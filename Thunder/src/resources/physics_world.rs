use crate::{compiler::AsyncCompiler, scene::Scene};
use gltf::json::extensions::scene::tempest_extension;
use physics_handler::*;
use std::{
    collections::HashMap,
    sync::{Arc, Weak},
};

use super::{mesh::MeshData, Resource};
#[derive(Debug)]
pub struct PhysicsWorldResource {
    scene: Weak<Scene>,
    meshes: Arc<Vec<MeshData>>,
    node_to_entity_map: HashMap<usize, u64>,
}

impl PhysicsWorldResource {
    pub fn new(
        scene: Weak<Scene>,
        meshes: Arc<Vec<MeshData>>,
        node_to_entity_map: HashMap<usize, u64>,
    ) -> Self {
        Self {
            scene,
            meshes,
            node_to_entity_map,
        }
    }
}

#[async_trait]
impl Resource for PhysicsWorldResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let mut physics = PhysicsHandler::new();

        let scene = self.scene.upgrade().unwrap();

        scene.walk_root_nodes(|gltf, node_index, world_transform| -> Option<()> {
            if gltf.node_mesh_index(node_index).is_none() {
                return None;
            }

            if let Some(physics_body) = gltf.tempest_extension(node_index).physics_body {
                let trs = math::TRS::new(world_transform);
                let px_transform = PxTransform::from_translation_rotation(
                    &PxVec3::new(trs.translate.x, trs.translate.y, trs.translate.z),
                    &PxQuat::new(trs.rotate.i, trs.rotate.j, trs.rotate.k, trs.rotate.w),
                );
                match physics_body.collision_shape.type_ {
                    gltf::json::validation::Checked::Valid(tempest_extension::Type::Mesh) => {
                        // Find proper index
                        let mesh_index = gltf.node_mesh_index(node_index).unwrap();
                        let position_index = scene
                            .meshes
                            .iter()
                            .position(|&index| index == mesh_index)
                            .unwrap();
                        let mesh_data = &self.meshes[position_index];

                        let mut mesh = physics
                            .create_triangle_mesh(
                                mesh_data.vertices.as_slice(),
                                mesh_data.indices.as_slice(),
                            )
                            .unwrap();
                        let geometry = physics.create_mesh_geometry(
                            PxVec3::new(trs.scale.x, trs.scale.y, trs.scale.z),
                            &mut mesh,
                        );
                        assert!(physics_body.dynamic == false);
                        physics.add_actor(
                            false, // This should be always false as meshes cannot be dynamic in PhysX
                            px_transform,
                            &geometry,
                            *self.node_to_entity_map.get(&node_index).unwrap(),
                        );
                    }
                    gltf::json::validation::Checked::Valid(tempest_extension::Type::Sphere) => {
                        let geometry = physics.create_sphere_geometry(
                            physics_body.collision_shape.radius.unwrap_or(1.0),
                        );
                        physics.add_actor(
                            physics_body.dynamic,
                            px_transform,
                            &geometry,
                            *self.node_to_entity_map.get(&node_index).unwrap(),
                        );
                    }
                    gltf::json::validation::Checked::Invalid => {
                        panic!("There should be valid collision shape")
                    }
                };
            }
            None
        });

        physics.serialize()
    }
}
