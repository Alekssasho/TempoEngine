use std::sync::Weak;

use components::*;
use flecs_rs::*;
use math::TRS;

use crate::{compiler::AsyncCompiler, scene::Scene};

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
        trs: TRS,
        mesh_index: u32,
        is_boids: bool,
    ) -> ecs_entity_t {
        let transform = Components::Transform(Tempest_Components_Transform {
            Position: trs.translate,
            Scale: trs.scale,
            Rotation: trs.rotate,
        });
        let static_mesh =
            Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
        let tags = if is_boids { vec![Tags::Boids] } else { vec![] };
        state.create_entity(name, &[transform, static_mesh], &tags)
    }
}

#[async_trait]
impl Resource for EntitiesWorldResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let flecs_state = FlecsState::new();

        let scene = self.scene.upgrade().unwrap();
        scene.walk_root_nodes(|gltf, node_index, world_transform| -> Option<()> {
            if let Some(mesh_index) = gltf.node_mesh_index(node_index) {
                EntitiesWorldResource::create_mesh_entity(
                    &flecs_state,
                    &gltf.node_name(node_index),
                    TRS::new(world_transform),
                    mesh_index as u32,
                    gltf.tempest_extension(node_index).boids,
                );
            }
            None
        });

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}
