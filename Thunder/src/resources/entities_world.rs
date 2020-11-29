use std::rc::Weak;

use components::*;
use flecs_rs::*;
use gltf_loader::{Scene, TRS};

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
        transform: TRS,
        mesh_index: u32,
        is_boids: bool,
    ) -> ecs_entity_t {
        let transform = Components::Transform(Tempest_Components_Transform {
            Position: transform.translate,
            Scale: transform.scale,
            Rotation: transform.rotate,
        });
        let static_mesh =
            Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
        let tags = if is_boids { vec![Tags::Boids] } else { vec![] };
        state.create_entity(name, &[transform, static_mesh], &tags)
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

        let scene = self.scene.upgrade().unwrap();
        let nodes = scene.gather_nodes();

        for node in nodes {
            if let Some(mesh_index) = node.mesh_index() {
                EntitiesWorldResource::create_mesh_entity(
                    &flecs_state,
                    &node.name(),
                    node.transform(),
                    mesh_index,
                    node.is_boids(),
                );
            }
        }

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}
