use crate::{compiler::AsyncCompiler, scene::Scene};
use data_definition_generated::PhysicsShapeFilter;
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

    fn create_convex_geometry(
        mesh_data: &MeshData,
        scale: &math::Vec3,
        physics: &mut PhysicsHandler,
    ) -> physics_handler::PxConvexMeshGeometry {
        // Meshes consist of seperate primitive meshes, with different rendering materials
        // For Physics we need only a single mesh (for now) so just concatenate it back together
        let (vertices, vertices_count) = {
            let mut vertices = Vec::new();

            let mut vertices_current_offset = 0;
            for primitive_mesh in &mesh_data.primitive_meshes {
                vertices.reserve(
                    std::mem::size_of::<crate::resources::mesh::VertexLayout>()
                        / std::mem::size_of::<f32>()
                        * primitive_mesh.simplyfied_mesh_indices.len(),
                );
                // vertices
                //     .extend_from_slice(primitive_mesh.get_vertices_float_slice());
                for index in &primitive_mesh.simplyfied_mesh_indices {
                    let vertex = primitive_mesh.vertices[*index as usize];
                    vertices.push(vertex.position.x);
                    vertices.push(vertex.position.y);
                    vertices.push(vertex.position.z);
                }
                vertices_current_offset += primitive_mesh.simplyfied_mesh_indices.len() as u32;
            }
            (vertices, vertices_current_offset)
        };

        let mut mesh = physics
            .create_convex_mesh(
                vertices.as_slice(),
                vertices_count as usize,
                std::mem::size_of::<f32>() * 3,
            )
            .unwrap();
        let geometry =
            physics.create_convex_geometry(PxVec3::new(scale.x, scale.y, scale.z), &mut mesh);
        geometry
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

            let trs = math::TRS::new(world_transform);
            let px_transform = PxTransform::from_translation_rotation(
                &PxVec3::new(trs.translate.x, trs.translate.y, trs.translate.z),
                &PxQuat::new(trs.rotate.x, trs.rotate.y, trs.rotate.z, trs.rotate.w),
            );

            if gltf.is_car(node_index) {
                const NUM_WHEELS: usize = 4;
                let node_name = gltf.node_name(node_index);
                // Don't reorder the names as they should correspond to physx::PxVehicleDrive4WWheelOrder, and chassis should be the last
                let possible_node_names = [
                    node_name.to_owned() + "_FrontLeftWheel",
                    node_name.to_owned() + "_FrontRightWheel",
                    node_name.to_owned() + "_RearLeftWheel",
                    node_name.to_owned() + "_RearRightWheel",
                    node_name.to_owned() + "_Chassis",
                ];
                let mut shapes = [None; 5];
                let mut chassis_position = Default::default();
                let mut wheels_positions: [math::Vec3; NUM_WHEELS] = Default::default();
                let mut wheel_radius = 0.0;
                let mut wheel_width = 0.0;
                for child in gltf.node_children(node_index) {
                    let child_node_name = gltf.node_name(child);
                    let index_of_car_component = possible_node_names
                        .iter()
                        .position(|possible_name| possible_name == child_node_name)
                        .unwrap();
                    let mesh_index = gltf.node_mesh_index(child).unwrap();
                    let position_index = scene
                        .meshes
                        .iter()
                        .position(|&index| index == mesh_index)
                        .unwrap();
                    let mesh_data = &self.meshes[position_index];
                    let child_geometry = PhysicsWorldResource::create_convex_geometry(
                        mesh_data,
                        &math::vec3(1.0, 1.0, 1.0),
                        &mut physics,
                    );
                    let child_transform = math::TRS::new(&(*world_transform * gltf.node_transform(child)));
                    match index_of_car_component {
                        0..=3 => {
                            wheels_positions[index_of_car_component] = child_transform.translate;
                            let bounds = unsafe { physics_handler::physx_sys::PxConvexMesh_getLocalBounds(child_geometry.convexMesh) };
                            // TODO: Checking for wheel geometry is the same
                            wheel_radius = unsafe{ physics_handler::physx_sys::PxBounds3_getDimensions(&bounds).y };
                            wheel_width = unsafe{ physics_handler::physx_sys::PxBounds3_getExtents_1(&bounds).x };
                        }
                        4 => {
                            chassis_position = child_transform.translate;
                        },
                        _ => unreachable!()
                    };
                    shapes[index_of_car_component] = Some(child_geometry);
                }

                assert!(shapes.iter().all(|opt| opt.is_some()));

                let shapes = shapes.iter().map(|geometry| {
                    physics.create_shape(&geometry.unwrap(), PhysicsShapeFilter::NonDrivableSurface as u32)
                }).collect::<Vec<_>>();

                let mut car_actor = physics.create_actor();
                let scale = 50.0;
                let chassis_mass_properties = unsafe {
                    let properties = physics_handler::physx_sys::PxRigidBodyExt_computeMassPropertiesFromShapes_mut(&shapes[4].as_ptr(), 1);

                    // TODO: in c++ we scale the inertia tensor as well, however we don't have operator mul for that
                    physics_handler::physx_sys::PxMassProperties_new_1(properties.mass * scale, &properties.inertiaTensor, &properties.centerOfMass)
                };
                for mut shape in shapes {
                    car_actor.attach_shape(&mut shape);
                }
                car_actor.set_mass(chassis_mass_properties.mass);
                car_actor.set_mass_space_inertia_tensor(
                    &PxVec3::from(unsafe {
                        physics_handler::physx_sys::phys_PxDiagonalize(
                            &chassis_mass_properties.inertiaTensor,
                            &mut physics_handler::physx_sys::PxQuat_new_1(0)
                        )}
                    )
                );
                car_actor.set_c_mass_local_pose(&physics_handler::PxTransform::from_translation(&physics_handler::PxVec3::from(chassis_mass_properties.centerOfMass)));

                let mut wheel_center_actor_offsets : [physics_handler::physx_sys::PxVec3; NUM_WHEELS] = unsafe{ [physics_handler::physx_sys::PxVec3_new(); NUM_WHEELS] };
                let mut wheels_data = unsafe{ [physics_handler::physx_sys::PxVehicleWheelData_new(); NUM_WHEELS] };
                let mut tires = unsafe{ [physics_handler::physx_sys::PxVehicleTireData_new(); NUM_WHEELS] };

                let wheel_mass = 20.0;
                let wheel_moi = 0.5 * wheel_mass * wheel_radius * wheel_radius;

                for i in 0..NUM_WHEELS {
                    let offset = wheels_positions[i] - chassis_position;
                    wheel_center_actor_offsets[i] = unsafe { physics_handler::physx_sys::PxVec3_new_3(offset.x, offset.y, offset.z) };

                    wheels_data[i].mMass = wheel_mass;
                    wheels_data[i].mMOI = wheel_moi;
                    wheels_data[i].mRadius = wheel_radius;
                    wheels_data[i].mWidth = wheel_width;

                    tires[i].mType = 0;
                }

                wheels_data[physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_LEFT as usize].mMaxHandBrakeTorque = 4000.0;
                wheels_data[physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_RIGHT as usize].mMaxHandBrakeTorque = 4000.0;
                wheels_data[physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_LEFT as usize].mMaxSteer = std::f32::consts::PI * 0.3333;
                wheels_data[physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_RIGHT as usize].mMaxSteer = std::f32::consts::PI * 0.3333;

                let mut suspensions = unsafe{ [physics_handler::physx_sys::PxVehicleSuspensionData_new(); NUM_WHEELS] };
                let mut suspension_sprung_masses = [0.0; NUM_WHEELS];
                unsafe{
                    physics_handler::physx_sys::phys_PxVehicleComputeSprungMasses(
                        NUM_WHEELS as u32,
                        wheel_center_actor_offsets.as_ptr(),
                        &chassis_mass_properties.centerOfMass,
                        chassis_mass_properties.mass,
                        1,
                        suspension_sprung_masses.as_mut_ptr());
                }

                let camber_angle_at_rest = 0.0;
                let camber_angle_at_max_droop = 0.01;
                let camber_angle_at_max_compression = -0.01;
                for i in 0..NUM_WHEELS {
                    suspensions[i].mMaxCompression = 0.3;
                    suspensions[i].mMaxDroop = 0.1;
                    suspensions[i].mSpringStrength = 35000.0;
                    suspensions[i].mSpringDamperRate = 4500.0;
                    suspensions[i].mSprungMass = suspension_sprung_masses[i];
                }
                for i in (0..NUM_WHEELS).step_by(2) {
                    suspensions[i + 0].mCamberAtRest = camber_angle_at_rest;
                    suspensions[i + 1].mCamberAtRest = -camber_angle_at_rest;
                    suspensions[i + 0].mCamberAtMaxDroop = camber_angle_at_max_droop;
                    suspensions[i + 1].mCamberAtMaxDroop = -camber_angle_at_max_droop;
                    suspensions[i + 0].mCamberAtMaxCompression = camber_angle_at_max_compression;
                    suspensions[i + 1].mCamberAtMaxCompression = -camber_angle_at_max_compression;
                }

                let wheels_simulation_data = unsafe{ physics_handler::physx_sys::PxVehicleWheelsSimData_allocate_mut(NUM_WHEELS as u32) };
                return Some(()); // Break recursion in the GLTF, as we have already produced what we need
            }

            if let Some(physics_body) = gltf.tempest_extension(node_index).physics_body {
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

                        // Meshes consist of seperate primitive meshes, with different rendering materials
                        // For Physics we need only a single mesh (for now) so just concatenate it back together\
                        let (vertices, indices, vertices_count) = {
                            let mut vertices = Vec::new();
                            let mut indices = Vec::new();

                            let mut vertices_current_offset = 0;
                            for primitive_mesh in &mesh_data.primitive_meshes {
                                vertices
                                    .extend_from_slice(primitive_mesh.get_vertices_float_slice());
                                indices.extend(
                                    primitive_mesh
                                        .whole_mesh_indices
                                        .iter()
                                        .map(|index| index + vertices_current_offset),
                                );
                                vertices_current_offset += primitive_mesh.vertices.len() as u32;
                            }
                            (vertices, indices, vertices_current_offset)
                        };

                        let mut mesh = physics
                            .create_triangle_mesh(
                                vertices.as_slice(),
                                vertices_count as usize,
                                std::mem::size_of::<crate::resources::mesh::VertexLayout>(),
                                indices.as_slice(),
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
                            PhysicsShapeFilter::DrivableSurface as u32,
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
                            PhysicsShapeFilter::NonDrivableSurface as u32,
                        );
                    }
                    gltf::json::validation::Checked::Valid(tempest_extension::Type::Convex) => {
                        // Find proper index
                        let mesh_index = gltf.node_mesh_index(node_index).unwrap();
                        let position_index = scene
                            .meshes
                            .iter()
                            .position(|&index| index == mesh_index)
                            .unwrap();
                        let mesh_data = &self.meshes[position_index];
                        let geometry = PhysicsWorldResource::create_convex_geometry(
                            mesh_data,
                            &trs.scale,
                            &mut physics,
                        );
                        physics.add_actor(
                            physics_body.dynamic,
                            px_transform,
                            &geometry,
                            *self.node_to_entity_map.get(&node_index).unwrap(),
                            PhysicsShapeFilter::DrivableSurface as u32,
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
