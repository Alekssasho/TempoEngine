use std::sync::Weak;

use components::*;
use flecs_rs::*;
use gltf_loader::{Scene, TRS};

use crate::compiler::AsyncCompiler;

use super::Resource;
#[derive(Debug)]
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

#[async_trait]
impl Resource for EntitiesWorldResource {
    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
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
