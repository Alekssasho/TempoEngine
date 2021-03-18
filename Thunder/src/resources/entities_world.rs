use std::sync::Weak;

use components::glm::identity;
use components::*;
use flecs_rs::*;
use gltf_loader::{Node, Scene, TRS};

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

fn export_node(flecs_state: &FlecsState, node: &Node, parent_transform: &glm::Mat4x4) {
    let world_transform = parent_transform * node.local_transform();
    if let Some(mesh_index) = node.mesh_index() {
        EntitiesWorldResource::create_mesh_entity(
            flecs_state,
            &node.name(),
            TRS::new(world_transform),
            mesh_index,
            node.is_boids(),
        );
    }

    for child in &node.children() {
        export_node(flecs_state, child, &world_transform);
    }
}

#[async_trait]
impl Resource for EntitiesWorldResource {
    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let flecs_state = FlecsState::new();

        let scene = self.scene.upgrade().unwrap();
        // TODO: Support only a single scene inside the document
        assert!(scene.num_scenes() == 1);
        let root_nodes = scene.gather_root_nodes(0);

        for node in root_nodes {
            export_node(&flecs_state, &node, &identity())
        }

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}
