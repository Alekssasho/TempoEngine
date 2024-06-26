use std::{collections::HashMap, sync::Weak};

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
        state: &mut FlecsState,
        name: &str,
        trs: TRS,
        mesh_index: u32,
        tags: Vec<Tags>,
        has_dynamic_physics: bool,
        has_car_physics: Option<u32>,
    ) -> TemporaryEntityId {
        let transform = Components::Transform(Tempest_Components_Transform {
            Position: trs.translate,
            Scale: trs.scale,
            Rotation: trs.rotate,
        });
        let static_mesh =
            Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
        let mut components = vec![transform, static_mesh];
        if has_dynamic_physics && has_car_physics.is_none() {
            components.push(Components::DynamicPhysicsActor(
                Tempest_Components_DynamicPhysicsActor {
                    Actor: std::ptr::null_mut(), // This will be patched on loading time
                },
            ))
        }

        if has_car_physics.is_some() {
            components.push(Components::CarPhysicsPart(
                // This will be patched on loading time
                Tempest_Components_CarPhysicsPart {
                    CarActor: 9999 as *mut _,
                    ShapeIndex: has_car_physics.unwrap(),
                },
            ))
        }

        state.create_entity(name, components, &tags)
    }
}

#[async_trait]
impl Resource for EntitiesWorldResource {
    // First is the compiled ecs state, the second is map from node_index to entity_id
    type ReturnValue = (Vec<u8>, HashMap<usize, usize>);

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Self::ReturnValue {
        let mut flecs_state = FlecsState::new();

        let scene = self.scene.upgrade().unwrap();
        let mut node_to_temporary_entity_map = HashMap::new();
        scene.walk_root_nodes(|gltf, node_index, world_transform| -> Option<()> {
            if gltf.is_car(node_index) {
                let node_name = gltf.node_name(node_index);
                let possible_node_names = [
                    node_name.to_owned() + "_FrontLeftWheel",
                    node_name.to_owned() + "_FrontRightWheel",
                    node_name.to_owned() + "_RearLeftWheel",
                    node_name.to_owned() + "_RearRightWheel",
                    node_name.to_owned() + "_Chassis",
                ];
                for child in gltf.node_children(node_index) {
                    let child_node_name = gltf.node_name(child);
                    let index_of_car_component = possible_node_names
                        .iter()
                        .position(|possible_name| possible_name == child_node_name)
                        .unwrap();
                    let mesh_index = gltf.node_mesh_index(child).unwrap();
                    let tags = Vec::new();
                    let entity_id = EntitiesWorldResource::create_mesh_entity(
                        &mut flecs_state,
                        &child_node_name,
                        TRS::new(&(*world_transform * gltf.node_transform(child))),
                        mesh_index as u32,
                        tags,
                        false,
                        Some(index_of_car_component as u32),
                    );
                    node_to_temporary_entity_map.insert(child, entity_id);
                }
                return Some(()); // This will break the recursion as we have already handled all the needed children
            } else if let Some(mesh_index) = gltf.node_mesh_index(node_index) {
                let tempest_extension = gltf.tempest_extension(node_index);
                let tags = Vec::new();
                let entity_id = EntitiesWorldResource::create_mesh_entity(
                    &mut flecs_state,
                    &gltf.node_name(node_index),
                    TRS::new(world_transform),
                    mesh_index as u32,
                    tags,
                    tempest_extension
                        .physics_body
                        .map_or(false, |physics_body| physics_body.dynamic),
                    None,
                );
                node_to_temporary_entity_map.insert(node_index, entity_id);
            } else if let Some(light_data) = gltf.light(node_index) {
                if let gltf::khr_lights_punctual::Kind::Directional = light_data.kind() {
                    // GLTF specify that lights shines upon -Z direction. Tempest is using +Z for that
                    // So prepend a single mirror matrix for the Z direction
                    let changed_shine_direction_world_transform =
                        *world_transform * math::Mat4::from_scale(math::vec3(1.0, 1.0, -1.0));
                    let trs = TRS::new(&changed_shine_direction_world_transform);
                    let transform = Components::Transform(Tempest_Components_Transform {
                        Position: trs.translate,
                        Scale: trs.scale,
                        Rotation: trs.rotate,
                    });
                    let light_color = light_data.color();
                    let light_color_info =
                        Components::LightColorInfo(Tempest_Components_LightColorInfo {
                            Color: math::vec3(light_color[0], light_color[1], light_color[2]),
                            Intensity: light_data.intensity(),
                        });

                    let entity_id = flecs_state.create_entity(
                        gltf.node_name(node_index),
                        vec![transform, light_color_info],
                        &[Tags::DirectionalLight],
                    );
                    node_to_temporary_entity_map.insert(node_index, entity_id);
                }
            }
            None
        });

        // // Add camera controller
        // flecs_state.create_entity("MainCamera", &[Components::CameraController(Tempest_Components_CameraController{
        //     CameraData: scene.camera,
        //     InputMapIndex: 0,
        // })], &[]);

        flecs_state.finish_adding_entities();

        let mut node_to_entity_map = HashMap::new();
        node_to_entity_map.extend(
            node_to_temporary_entity_map
                .into_iter()
                .map(|(key, value)| (key, flecs_state.create_stable_entity_ids(value))),
        );

        let mut binary_data = Vec::<u8>::new();
        flecs_state.write_to_buffer(&mut binary_data);

        (binary_data, node_to_entity_map)
    }
}
