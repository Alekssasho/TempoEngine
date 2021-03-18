use super::*;
use crate::resources::audio_database::AudioDatabaseResource;
use crate::resources::entities_world::EntitiesWorldResource;
use crate::resources::geometry_database::GeometryDatabaseResource;

use data_definition_generated::{
    flatbuffer_derive::FlatbufferSerializeRoot, Camera, Vec3, AUDIO_DATABASE_EXTENSION,
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
    camera: Option<&'a Camera>,
}

fn walk_nodes(
    node: &gltf_loader::Node,
    parent_transform: &components::glm::Mat4x4,
) -> Option<(gltf_loader::PerspectiveCamera, components::glm::Mat4x4)> {
    let world_transform = parent_transform * node.local_transform();

    if let Some(camera) = node.camera() {
        return Some((camera, world_transform));
    }

    for child in &node.children() {
        let data = walk_nodes(child, &world_transform);
        if data.is_some() {
            return data;
        }
    }

    None
}

fn extract_camera_from_scene(scene: &Scene) -> Camera {
    let root_nodes = scene.gather_root_nodes(0);
    for node in root_nodes {
        if let Some((camera, transform)) = walk_nodes(&node, &components::glm::identity()) {
            let trs = gltf_loader::TRS::new(transform);

            let rotated_up = components::glm::quat_cross_vec(&trs.rotate, &components::glm::vec3(0.0, 1.0, 0.0));
            let rotated_forward = components::glm::quat_cross_vec(&trs.rotate, &components::glm::vec3(0.0, 0.0, 1.0));

            return Camera::new(
                camera.yfov,
                camera.znear,
                camera.zfar,
                camera.aspect_ratio,
                &Vec3::new(trs.translate.x, trs.translate.y, trs.translate.z),
                &Vec3::new(rotated_forward.x, rotated_forward.y, rotated_forward.z),
                &Vec3::new(rotated_up.x, rotated_up.y, rotated_up.z),
            );
        }
    }

    // Default Camera
    Camera::new(
        1.0,
        0.1,
        1000.0,
        16.0 / 9.0,
        &Vec3::new(0.0, 0.0, 0.0),
        &Vec3::new(0.0, 0.0, 0.0),
        &Vec3::new(0.0, 0.0, 0.0),
    )
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
            Arc::new(Scene::new(level_scene_file_name))
        };

        let entities_resource_data =
            EntitiesWorldResource::new(Arc::downgrade(&scene));
        let geometry_database_data =
            GeometryDatabaseResource::new(Arc::downgrade(&scene));
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

        // We need to do some work for the level, so do it here, after we have spawned the async tasks and before waiting for them
        let level_camera = extract_camera_from_scene(&scene);

        // After we have done the needed work wait for the async task to finish
        let (
            geometry_database_compiled_data,
            entities_resource_compiled_data,
            audio_database_compiled_data,
        ) = tokio::try_join!(geometry_future, entities_future, audio_future).unwrap();

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

        let level = Level {
            name: &self.name,
            entities: entities_resource_compiled_data.as_slice(),
            geometry_database_file: geometry_database_name,
            audio_database_file: audio_database_name,
            camera: Some(&level_camera),
        };

        let result = level.serialize_root();

        // Wait for the write task to finish as well
        write_handle_geometry.await.expect("Write task failed");
        write_handle_audio.await.expect("Write task failed");

        result
    }
}
