use super::*;
use crate::resources::geometry_database::GeometryDatabaseResource;
use components::*;
use data_definition_generated::{Level, LevelArgs, GEOMETRY_DATABASE_EXTENSION, LEVEL_IDENTIFIER};
use flecs_rs::*;
use gltf_loader::Scene;
use std::{
    ffi::CString,
    io::Write,
    path::PathBuf,
    rc::{Rc, Weak},
};
pub struct LevelResource {
    name: String,
    entities: ResourceId,
    geometry_database_id: ResourceId,
    scene: Option<Rc<Scene>>,
}

impl LevelResource {
    pub fn new(name: String) -> Self {
        Self {
            name,
            entities: INVALID_RESOURCE,
            geometry_database_id: INVALID_RESOURCE,
            scene: None,
        }
    }
}

impl Resource for LevelResource {
    fn extract_dependencies<'a>(
        &'a mut self,
        compiler: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        let mut level_scene_file_name = compiler.options.input_folder.clone();
        level_scene_file_name.push(&self.name);
        level_scene_file_name.set_extension("gltf");
        self.scene = Some(Rc::new(Scene::new(level_scene_file_name)));

        self.entities = compiler.get_next_resource_id();
        self.geometry_database_id = compiler.get_next_resource_id();

        let entities_resource_data =
            EntitiesWorldResource::new(Rc::downgrade(self.scene.as_ref().unwrap()));
        let geometry_database_data =
            GeometryDatabaseResource::new(Rc::downgrade(self.scene.as_ref().unwrap()));

        vec![
            (self.entities, Some(Box::new(entities_resource_data))),
            (
                self.geometry_database_id,
                Some(Box::new(geometry_database_data)),
            ),
        ]
    }

    fn compile(&self, compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);

        let level_name_offset = builder.create_string(&self.name);

        let geometry_database_compiled_data =
            compiled_dependencies.get_resource_data(self.geometry_database_id);
        let mut output_file_path = compiled_dependencies.options.output_folder.clone();
        output_file_path.push(&self.name);
        output_file_path.set_extension(GEOMETRY_DATABASE_EXTENSION);
        write_resource_to_file(geometry_database_compiled_data, output_file_path);
        let geometry_database_name = format!("{}.{}", self.name, GEOMETRY_DATABASE_EXTENSION);
        let geometry_database_name_offset = builder.create_string(&geometry_database_name);

        let entities_data = compiled_dependencies.get_resource_data(self.entities);
        let entities_vector_offset = builder.create_vector(entities_data);

        let root_level = Level::create(
            &mut builder,
            &LevelArgs {
                name: Some(level_name_offset),
                entities: Some(entities_vector_offset),
                geometry_database_file: Some(geometry_database_name_offset),
            },
        );
        builder.finish(root_level, Some(LEVEL_IDENTIFIER));

        Vec::from(builder.finished_data())
    }
}

pub struct EntitiesWorldResource {
    scene: Weak<Scene>,
}

impl EntitiesWorldResource {
    pub fn new(scene: Weak<Scene>) -> Self {
        Self {
            scene,
        }
    }
}

impl Resource for EntitiesWorldResource {
    fn extract_dependencies(
        &mut self,
        _registry: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        // No dependancies for this resource
        vec![]
    }

    fn compile(&self, _compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let flecs_state = FlecsState::new();
        let mut entity_names = Vec::new();
        // //Old boids example
        // for i in 0..50 {
        //     let name = format!("Rect {}", i);
        //     entity_names.push(create_entity(
        //         &flecs_state,
        //         &name,
        //         glm::vec3(-0.8 + ((i as f32) * 0.03), -1.0, 0.0),
        //         glm::vec4(0.0, 0.0, 1.0, 1.0),
        //     ));
        // }

        // Mesh example
        entity_names.push(create_mesh_entity(
            &flecs_state,
            "Mesh",
            glm::vec3(0.0, 0.0, 0.0),
            0,
        ));

        // entity_names.push(create_mesh_entity(
        //     &flecs_state,
        //     "Mesh 2",
        //     glm::vec3(0.0, 0.0, 0.0),
        //     1,
        // ));

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}

#[allow(dead_code)]
fn create_entity(
    state: &FlecsState,
    name: &str,
    pos: glm_vec3,
    color: glm_vec4,
) -> (ecs_entity_t, CString) {
    let transform = Components::Transform(Tempest_Components_Transform {
        Position: pos,
        Heading: glm::vec3(1.0, 0.0, 0.0),
    });
    let rect = Components::Rect(Tempest_Components_Rect {
        width: 0.02f32,
        height: 0.02f32,
        color,
    });
    state.create_entity(name, &[transform, rect])
}

#[allow(dead_code)]
fn create_mesh_entity(
    state: &FlecsState,
    name: &str,
    pos: glm_vec3,
    mesh_index: u32,
) -> (ecs_entity_t, CString) {
    let transform = Components::Transform(Tempest_Components_Transform {
        Position: pos,
        Heading: glm::vec3(1.0, 0.0, 0.0),
    });
    let static_mesh = Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
    state.create_entity(name, &[transform, static_mesh])
}

pub fn write_resource_to_file(data_to_write: &[u8], path: PathBuf) {
    let mut output_file = std::fs::File::create(path).expect("Cannot create output file");
    output_file
        .write_all(data_to_write)
        .expect("Cannot write output data");
}
