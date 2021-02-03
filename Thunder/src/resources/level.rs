use super::*;
use crate::resources::audio_database::AudioDatabaseResource;
use crate::resources::entities_world::EntitiesWorldResource;
use crate::resources::geometry_database::GeometryDatabaseResource;

use data_definition_generated::{
    flatbuffer_derive::FlatbufferSerializeRoot, AUDIO_DATABASE_EXTENSION,
    GEOMETRY_DATABASE_EXTENSION,
};

use gltf_loader::Scene;
use std::sync::Arc;

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

#[async_trait]
impl Resource for LevelResource {
    #[instrument]
    async fn compile(&self, compiler: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let mut level_scene_file_name = compiler.options.input_folder.clone();
        level_scene_file_name.push(&self.name);
        level_scene_file_name.set_extension("gltf");
        let scene = {
            ttrace!("Load Scene");
            Some(Arc::new(Scene::new(level_scene_file_name)))
        };

        let entities_resource_data =
            EntitiesWorldResource::new(Arc::downgrade(scene.as_ref().unwrap()));
        let geometry_database_data =
            GeometryDatabaseResource::new(Arc::downgrade(scene.as_ref().unwrap()));
        let audio_database_data = AudioDatabaseResource {};

        let geometry_compiler = compiler.clone();
        let entities_compiler = compiler.clone();
        let audio_compiler = compiler.clone();

        let geometry_future =
            tokio::spawn(async move { geometry_database_data.compile(geometry_compiler).await });
        let entities_future =
            tokio::spawn(async move { entities_resource_data.compile(entities_compiler).await });
        let audio_future =
            tokio::spawn(async move { audio_database_data.compile(audio_compiler).await });

        let (
            geometry_database_compiled_data,
            entities_resource_compiled_data,
            audio_database_compiled_data,
        ) = tokio::try_join!(geometry_future, entities_future, audio_future).unwrap();

        // Write geometry database to a file
        //let geometry_database_compiled_data = geometry_database_data.compile_async(compiler).await;
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

        #[derive(FlatbufferSerializeRoot)]
        struct Level<'a> {
            #[offset]
            name: &'a str,
            #[offset]
            entities: &'a [u8],
            #[offset]
            geometry_database_file: String,
            #[offset]
            audio_database_file: String,
        }

        let level = Level {
            name: &self.name,
            entities: entities_resource_compiled_data.as_slice(),
            geometry_database_file: geometry_database_name,
            audio_database_file: audio_database_name,
        };

        let result = level.serialize_root();

        // Wait for the write task to finish as well
        write_handle_geometry.await.expect("Write task failed");
        write_handle_audio.await.expect("Write task failed");

        result
    }
}
