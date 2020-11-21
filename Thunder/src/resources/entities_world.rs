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
        transform: glm_mat4x4,
        mesh_index: u32,
    ) -> (ecs_entity_t, CString) {
        let transform = Components::Transform(Tempest_Components_Transform { Matrix: transform });
        let static_mesh =
            Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
        state.create_entity(name, &[transform, static_mesh], &[Tags::Boids])
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
                    &node.name(),
                    node.transform(),
                    mesh_index,
                    //node.is_boids()
                ));
            }
        }

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}
