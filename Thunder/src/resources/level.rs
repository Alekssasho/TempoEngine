use super::*;
use crate::resources::audio_database::AudioDatabaseResource;
use crate::resources::entities_world::EntitiesWorldResource;
use crate::resources::geometry_database::GeometryDatabaseResource;
use crate::resources::mesh::MeshResource;
use crate::resources::texture_database::TextureDatabaseResource;

use data_definition_generated::{
    flatbuffer_derive::FlatbufferSerializeRoot, AUDIO_DATABASE_EXTENSION,
    GEOMETRY_DATABASE_EXTENSION, TEXTURE_DATABASE_EXTENSION,
};
use mesh::MeshData;
use physics_world::PhysicsWorldResource;

use std::sync::Arc;

use crate::scene::Scene;
use crate::ttrace;

#[derive(Debug)]
pub struct LevelResource {
    name: String,
}

impl LevelResource {
    pub fn new(name: String) -> Self {
        Self { name }
    }
}

#[derive(FlatbufferSerializeRoot)]
struct Level<'a> {
    #[store_offset]
    name: &'a str,
    #[store_vector_direct]
    entities: &'a [u8],
    #[store_vector_direct]
    physics_world: &'a [u8],
    #[store_offset]
    geometry_database_file: String,
    #[store_offset]
    texture_database_file: String,
    #[store_offset]
    audio_database_file: String,
    camera: Option<&'a data_definition_generated::Camera>,
}

#[async_trait]
impl Resource for LevelResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, compiler: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let mut level_scene_file_name = compiler.options.input_folder.clone();
        level_scene_file_name.push(&self.name);
        level_scene_file_name.set_extension("gltf");
        let scene = {
            ttrace!("Load Scene");
            Arc::new(Scene::new(&level_scene_file_name))
        };
        // First step is extract all meshes
        let mut mesh_futures = Vec::new();
        for mesh in &scene.meshes {
            let mesh_resource = MeshResource::new(Arc::downgrade(&scene), *mesh);
            let mesh_compiler = compiler.clone();
            mesh_futures.push(tokio::spawn(async move {
                mesh_resource.compile(mesh_compiler).await
            }));
        }

        let gathered_meshes = Arc::new(
            futures::future::join_all(mesh_futures)
                .await
                .into_iter()
                .map(Result::unwrap)
                .collect::<Vec<MeshData>>(),
        );
        assert!(gathered_meshes.len() == scene.meshes.len());

        // Second step prepare EntitiesWorld and GeometryDatabase which both require the extracted meshes
        let entities_resource_data = EntitiesWorldResource::new(Arc::downgrade(&scene));
        let geometry_database_data =
            GeometryDatabaseResource::new(Arc::downgrade(&scene), gathered_meshes.clone());
        let texture_database_data = TextureDatabaseResource::new(Arc::downgrade(&scene));
        let audio_database_data = AudioDatabaseResource {};

        let geometry_compiler = compiler.clone();
        let entities_compiler = compiler.clone();
        let texture_compiler = compiler.clone();
        let audio_compiler = compiler.clone();

        let entities_future =
            tokio::spawn(async move { entities_resource_data.compile(entities_compiler).await });
        let geometry_future =
            tokio::spawn(async move { geometry_database_data.compile(geometry_compiler).await });
        let texture_future =
            tokio::spawn(async move { texture_database_data.compile(texture_compiler).await });
        let audio_future =
            tokio::spawn(async move { audio_database_data.compile(audio_compiler).await });

        // We need to wait for entities world, in order to have entity id we need for later patching
        // then we cook the physics world which needs the ids. This is async to geometry/audio database
        let (entities_resource_compiled_data, node_to_entity_map) = entities_future.await.unwrap();

        let physics_resource_data = PhysicsWorldResource::new(
            Arc::downgrade(&scene),
            gathered_meshes.clone(),
            node_to_entity_map,
        );
        let physics_compiler = compiler.clone();
        let physics_future =
            tokio::spawn(async move { physics_resource_data.compile(physics_compiler).await });

        // After we have done the needed work wait for the async task to finish
        let (
            geometry_database_compiled_data,
            texture_database_compiled_data,
            audio_database_compiled_data,
            physics_resource_compiled_data,
        ) = tokio::try_join!(
            geometry_future,
            texture_future,
            audio_future,
            physics_future,
        )
        .unwrap();

        // Write geometry database to a file
        let mut output_file_path = compiler.options.output_folder.clone();
        output_file_path.push(&self.name);
        output_file_path.set_extension(GEOMETRY_DATABASE_EXTENSION);
        let write_handle_geometry = tokio::spawn(async move {
            write_resource_to_file(geometry_database_compiled_data.as_slice(), output_file_path)
                .await
        });

        // Generate geometry database filename to write in resource
        let geometry_database_name = format!("{}.{}", self.name, GEOMETRY_DATABASE_EXTENSION);

        // Write audio database to file
        let mut output_file_path = compiler.options.output_folder.clone();
        output_file_path.push(&self.name);
        output_file_path.set_extension(AUDIO_DATABASE_EXTENSION);
        let write_handle_audio = tokio::spawn(async move {
            write_resource_to_file(audio_database_compiled_data.as_slice(), output_file_path).await
        });

        // Generate audio database filename to write in resource
        let audio_database_name = format!("{}.{}", self.name, AUDIO_DATABASE_EXTENSION);

        // Write texture database to file
        let mut output_file_path = compiler.options.output_folder.clone();
        output_file_path.push(&self.name);
        output_file_path.set_extension(TEXTURE_DATABASE_EXTENSION);
        let write_handle_texture = tokio::spawn(async move {
            write_resource_to_file(texture_database_compiled_data.as_slice(), output_file_path)
                .await
        });

        // Generate audio database filename to write in resource
        let texture_database_name = format!("{}.{}", self.name, TEXTURE_DATABASE_EXTENSION);

        let level = Level {
            name: &self.name,
            entities: entities_resource_compiled_data.as_slice(),
            physics_world: physics_resource_compiled_data.as_slice(),
            geometry_database_file: geometry_database_name,
            texture_database_file: texture_database_name,
            audio_database_file: audio_database_name,
            camera: Some(&scene.camera),
        };

        let result = level.serialize_root();

        // Wait for the write task to finish as well
        write_handle_geometry.await.expect("Write task failed");
        write_handle_texture.await.expect("Write task failed");
        write_handle_audio.await.expect("Write task failed");

        result
    }
}
