use std::{ffi::CString, rc::Weak};

use components::*;
use flecs_rs::*;
use gltf_loader::Scene;

use crate::compiler::{CompiledResources, CompilerGraph, ResourceBox};

use super::{Resource, ResourceId};

pub struct EntitiesWorldResource {
    scene: Weak<Scene>,
}

impl EntitiesWorldResource {
    pub fn new(scene: Weak<Scene>) -> Self {
        Self { scene }
    }

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
        let static_mesh =
            Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
        state.create_entity(name, &[transform, static_mesh])
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

        let scene = self.scene.upgrade().unwrap();
        let nodes = scene.gather_nodes();

        for node in nodes {
            if let Some(mesh_index) = node.mesh_index() {
                entity_names.push(EntitiesWorldResource::create_mesh_entity(
                    &flecs_state,
                    "Mesh",
                    glm::vec3(0.0, 0.0, 0.0),
                    mesh_index,
                ));
            }
        }

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}
