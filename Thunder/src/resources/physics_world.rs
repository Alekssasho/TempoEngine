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

    fn create_convex_mesh(
        mesh_data: &MeshData,
        physics: &mut PhysicsHandler,
    ) -> physics_handler::Owner<physics_handler::ConvexMesh> {
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

        let mesh = physics
            .create_convex_mesh(
                vertices.as_slice(),
                vertices_count as usize,
                std::mem::size_of::<f32>() * 3,
            )
            .unwrap();
        mesh
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
                let mut shapes: [Option<(Owner<ConvexMesh>, PxConvexMeshGeometry)>; NUM_WHEELS + 1] = Default::default();
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
                    let mut convex_mesh = PhysicsWorldResource::create_convex_mesh(mesh_data, &mut physics);
                    let child_geometry =
                        physics.create_convex_geometry(PxVec3::new(trs.scale.x, trs.scale.y, trs.scale.z), &mut convex_mesh);
                    let child_transform = math::TRS::new(&(*world_transform * gltf.node_transform(child)));
                    match index_of_car_component {
                        0..=3 => {
                            wheels_positions[index_of_car_component] = child_transform.translate;
                            let bounds = unsafe { physics_handler::physx_sys::PxConvexMesh_getLocalBounds(child_geometry.convexMesh) };
                            // TODO: Checking for wheel geometry is the same
                            wheel_radius = unsafe{ physics_handler::physx_sys::PxBounds3_getExtents_1(&bounds).y };
                            wheel_width = unsafe{ physics_handler::physx_sys::PxBounds3_getDimensions(&bounds).x };
                        }
                        4 => {
                            chassis_position = child_transform.translate;
                        },
                        _ => unreachable!()
                    };
                    shapes[index_of_car_component] = Some((convex_mesh, child_geometry));
                }

                assert!(shapes.iter().all(|opt| opt.is_some()));

                let mut shapes = shapes.iter().map(|geometry| {
                    physics.create_shape(&geometry.as_ref().unwrap().1, PhysicsShapeFilter::NonDrivableSurface as u32)
                }).collect::<Vec<_>>();

                let mut car_actor = physics.create_actor();
                let scale = 50.0;
                let chassis_mass_properties = unsafe {
                    let properties = physics_handler::physx_sys::PxRigidBodyExt_computeMassPropertiesFromShapes_mut(&shapes[4].as_ptr(), 1);

                    let column0 = physics_handler::physx_sys::PxVec3_new_3(properties.inertiaTensor.column0.x * scale, properties.inertiaTensor.column0.y * scale, properties.inertiaTensor.column0.z * scale);
                    let column1 = physics_handler::physx_sys::PxVec3_new_3(properties.inertiaTensor.column1.x * scale, properties.inertiaTensor.column1.y * scale, properties.inertiaTensor.column1.z * scale);
                    let column2 = physics_handler::physx_sys::PxVec3_new_3(properties.inertiaTensor.column2.x * scale, properties.inertiaTensor.column2.y * scale, properties.inertiaTensor.column2.z * scale);

                    physics_handler::physx_sys::PxMassProperties_new_1(properties.mass * scale, &physics_handler::physx_sys::PxMat33_new_3(&column0, &column1, &column2), &properties.centerOfMass)
                };
                for mut shape in &mut shapes {
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

                let mut susp_travel_directions = unsafe{ [physics_handler::physx_sys::PxVec3_new(); NUM_WHEELS] };
                let mut wheel_centre_cm_offsets = unsafe{ [physics_handler::physx_sys::PxVec3_new(); NUM_WHEELS] };
                let mut susp_force_app_cm_offsets = unsafe{ [physics_handler::physx_sys::PxVec3_new(); NUM_WHEELS] };
                let mut tire_force_app_cm_offsets = unsafe{ [physics_handler::physx_sys::PxVec3_new(); NUM_WHEELS] };
                //Set the geometry data.
                for i in 0..NUM_WHEELS {
                    //Vertical suspension travel.
                    susp_travel_directions[i] = unsafe{ physics_handler::physx_sys::PxVec3_new_3(0.0, -1.0, 0.0) };

                    //Wheel center offset is offset from rigid body center of mass.
                    wheel_centre_cm_offsets[i] = unsafe{ physics_handler::physx_sys::PxVec3_new_3(
                        wheel_center_actor_offsets[i].x - chassis_mass_properties.centerOfMass.x,
                        wheel_center_actor_offsets[i].y - chassis_mass_properties.centerOfMass.y,
                        wheel_center_actor_offsets[i].z - chassis_mass_properties.centerOfMass.z) };

                    //Suspension force application point 0.3 metres below 
                    //rigid body center of mass.
                    susp_force_app_cm_offsets[i] = unsafe{ physics_handler::physx_sys::PxVec3_new_3(wheel_centre_cm_offsets[i].x, -0.3, wheel_centre_cm_offsets[i].z) };

                    //Tire force application point 0.3 metres below 
                    //rigid body center of mass.
                    tire_force_app_cm_offsets[i] = unsafe{ physics_handler::physx_sys::PxVec3_new_3(wheel_centre_cm_offsets[i].x, -0.3, wheel_centre_cm_offsets[i].z) };
                }

                let mut filter_data = unsafe { physx_sys::PxFilterData_new_1() };
                unsafe {
                    physx_sys::PxFilterData_setToDefault_mut(&mut filter_data);
                }
                filter_data.word3 = PhysicsShapeFilter::NonDrivableSurface as u32;

                let wheels_simulation_data = unsafe{ physics_handler::physx_sys::PxVehicleWheelsSimData_allocate_mut(NUM_WHEELS as u32) };
                for i in 0..NUM_WHEELS {
                    unsafe{
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setWheelData_mut(wheels_simulation_data, i as u32, &wheels_data[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setTireData_mut(wheels_simulation_data, i as u32, &tires[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setSuspensionData_mut(wheels_simulation_data, i as u32, &suspensions[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setSuspTravelDirection_mut(wheels_simulation_data, i as u32, &susp_travel_directions[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setWheelCentreOffset_mut(wheels_simulation_data, i as u32, &wheel_centre_cm_offsets[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setSuspForceAppPointOffset_mut(wheels_simulation_data, i as u32, &susp_force_app_cm_offsets[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setTireForceAppPointOffset_mut(wheels_simulation_data, i as u32, &tire_force_app_cm_offsets[i]);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setSceneQueryFilterData_mut(wheels_simulation_data, i as u32, &filter_data);
                        physics_handler::physx_sys::PxVehicleWheelsSimData_setWheelShapeMapping_mut(wheels_simulation_data, i as u32, i as i32);
                    }
                }

                unsafe {
                    let mut bar_front = physics_handler::physx_sys::PxVehicleAntiRollBarData_new();
                    bar_front.mWheel0 = physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_LEFT as u32;
                    bar_front.mWheel1 = physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_RIGHT as u32;
                    bar_front.mStiffness = 10000.0;
                    physics_handler::physx_sys::PxVehicleWheelsSimData_addAntiRollBarData_mut(wheels_simulation_data, &bar_front);

                    let mut bar_rear = physics_handler::physx_sys::PxVehicleAntiRollBarData_new();
                    bar_rear.mWheel0 = physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_LEFT as u32;
                    bar_rear.mWheel1 = physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_RIGHT as u32;
                    bar_rear.mStiffness = 10000.0;
                    physics_handler::physx_sys::PxVehicleWheelsSimData_addAntiRollBarData_mut(wheels_simulation_data, &bar_rear);
                }

                let mut drive_sim_data = unsafe{ physics_handler::physx_sys::PxVehicleDriveSimData4W_new() };
                unsafe {
                    let mut diff = physics_handler::physx_sys::PxVehicleDifferential4WData_new();
                    diff.mType = physics_handler::physx_sys::PxVehicleDifferential4WDataEnum::eDIFF_TYPE_LS_4WD as u32;
                    physics_handler::physx_sys::PxVehicleDriveSimData4W_setDiffData_mut(&mut drive_sim_data, &diff);

                    //Engine
                    let mut engine = physics_handler::physx_sys::PxVehicleEngineData_new();
                    engine.mPeakTorque = 500.0;
                    engine.mMaxOmega = 600.0;//approx 6000 rpm
                    physics_handler::physx_sys::PxVehicleDriveSimData_setEngineData_mut(&mut drive_sim_data as *mut physics_handler::physx_sys::PxVehicleDriveSimData4W as *mut physics_handler::physx_sys::PxVehicleDriveSimData, &engine);

                    //Gears
                    let mut  gears = physics_handler::physx_sys::PxVehicleGearsData_new();
                    gears.mSwitchTime = 0.5;
                    physics_handler::physx_sys::PxVehicleDriveSimData_setGearsData_mut(&mut drive_sim_data as *mut physics_handler::physx_sys::PxVehicleDriveSimData4W as *mut physics_handler::physx_sys::PxVehicleDriveSimData, &gears);

                    //Clutch
                    let mut clutch = physics_handler::physx_sys::PxVehicleClutchData_new();
                    clutch.mStrength = 10.0;
                    physics_handler::physx_sys::PxVehicleDriveSimData_setClutchData_mut(&mut drive_sim_data as *mut physics_handler::physx_sys::PxVehicleDriveSimData4W as *mut physics_handler::physx_sys::PxVehicleDriveSimData, &clutch);

                    //Ackermann steer accuracy
                    let mut ackermann = physics_handler::physx_sys::PxVehicleAckermannGeometryData_new();
                    ackermann.mAccuracy = 1.0;
                    ackermann.mAxleSeparation =
                        (*physics_handler::physx_sys::PxVehicleWheelsSimData_getWheelCentreOffset(wheels_simulation_data, physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_LEFT)).z -
                        (*physics_handler::physx_sys::PxVehicleWheelsSimData_getWheelCentreOffset(wheels_simulation_data, physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_LEFT)).z;
                    ackermann.mFrontWidth =
                        (*physics_handler::physx_sys::PxVehicleWheelsSimData_getWheelCentreOffset(wheels_simulation_data, physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_RIGHT)).x -
                        (*physics_handler::physx_sys::PxVehicleWheelsSimData_getWheelCentreOffset(wheels_simulation_data, physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eFRONT_LEFT)).x;
                    ackermann.mRearWidth =
                        (*physics_handler::physx_sys::PxVehicleWheelsSimData_getWheelCentreOffset(wheels_simulation_data, physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_RIGHT)).x -
                        (*physics_handler::physx_sys::PxVehicleWheelsSimData_getWheelCentreOffset(wheels_simulation_data, physics_handler::physx_sys::PxVehicleDrive4WWheelOrder::eREAR_LEFT)).x;
                    println!("axple separation: {}, front widht: {}, rear width: {}", ackermann.mAxleSeparation, ackermann.mFrontWidth, ackermann.mRearWidth);
                    physics_handler::physx_sys::PxVehicleDriveSimData4W_setAckermannGeometryData_mut(&mut drive_sim_data, &ackermann);
                }

                unsafe{
                    let vehicle_drive_4w = physics_handler::physx_sys::PxVehicleDrive4W_allocate_mut(NUM_WHEELS as u32);
                    physics_handler::physx_sys::PxVehicleDrive4W_setup_mut(
                        vehicle_drive_4w,
                        physics.physx_foundation.physics_mut().as_mut_ptr(),
                        car_actor.as_mut_ptr(),
                        wheels_simulation_data,
                        &drive_sim_data,
                        0);

                    physics_handler::physx_sys::PxVehicleWheelsSimData_free_mut(wheels_simulation_data);

                    physics_handler::physx_sys::PxRigidActor_setGlobalPose_mut(
                        physics_handler::physx_sys::PxVehicleWheels_getRigidDynamicActor_mut(vehicle_drive_4w as *mut physics_handler::physx_sys::PxVehicleWheels) as *mut physx_sys::PxRigidActor,
                        px_transform.as_ptr(),
                        true);

                    physics_handler::physx_sys::PxScene_addActor_mut(
                        physics.physx_scene.as_mut_ptr(),
                        physics_handler::physx_sys::PxVehicleWheels_getRigidDynamicActor_mut(vehicle_drive_4w as *mut physics_handler::physx_sys::PxVehicleWheels) as *mut physx_sys::PxActor,
                        std::ptr::null());

                    physics_handler::physx_sys::PxVehicleDrive4W_setToRestState_mut(vehicle_drive_4w);

                    physx_sys::PxCollection_add_mut(
                        physics.raw_ptr_holder.physx_collection,
                        vehicle_drive_4w as *mut physx_sys::PxBase,
                        9997,
                    );

                    physx_sys::PxCollection_add_mut(
                        physics.raw_ptr_holder.physx_collection,
                        physics_handler::physx_sys::PxVehicleWheels_getRigidDynamicActor_mut(vehicle_drive_4w as *mut physics_handler::physx_sys::PxVehicleWheels) as *mut physx_sys::PxBase,
                        9999,
                    );
                    physx_sys::PxSerialization_complete_mut(
                        physics.raw_ptr_holder.physx_collection,
                        physics.raw_ptr_holder.physx_serialize_registry,
                        std::ptr::null(),
                        false,
                    );
                }

                // Don't try to release those, as physx is crashing. Maybe needs much investigation what is wrong.
                std::mem::forget(shapes);
                std::mem::forget(car_actor);

                return Some(()); // Break recursion in the GLTF, as we have already produced what we need
            }

            if gltf.node_mesh_index(node_index).is_none() {
                return None;
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
                        let mut convex_mesh = PhysicsWorldResource::create_convex_mesh(
                            mesh_data,
                            &mut physics,
                        );
                        let geometry =
                            physics.create_convex_geometry(PxVec3::new(trs.scale.x, trs.scale.y, trs.scale.z), &mut convex_mesh);
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
