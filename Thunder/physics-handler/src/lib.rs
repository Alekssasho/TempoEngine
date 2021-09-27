use physx::{
    cooking::{ConvexMeshCookingResult, PxCooking, PxCookingParams, TriangleMeshCookingResult},
    foundation::DefaultAllocator,
    prelude::*,
};

pub use physx::geometry::PxConvexMeshGeometry;
pub use physx::rigid_actor::RigidActor;
pub use physx::rigid_body::RigidBody;
pub use physx::traits::Class;
pub use physx_sys;
pub use physx::owner::Owner;
pub use physx::convex_mesh::ConvexMesh;

pub use physx::math::{PxQuat, PxTransform, PxVec3};
use physx_sys::{
    PxBoundedData_new, PxCollection, PxConvexFlag, PxConvexMeshGeometryFlags, PxMeshGeometryFlags,
    PxMeshScale_new_2, PxSerializationRegistry,
};

type PxMaterial = physx::material::PxMaterial<()>;
type PxShape = physx::shape::PxShape<(), PxMaterial>;
type PxArticulationLink = physx::articulation_link::PxArticulationLink<(), PxShape>;
type PxRigidStatic = physx::rigid_static::PxRigidStatic<(), PxShape>;
type PxRigidDynamic = physx::rigid_dynamic::PxRigidDynamic<(), PxShape>;
type PxArticulation = physx::articulation::PxArticulation<(), PxArticulationLink>;
type PxArticulationReducedCoordinate =
    physx::articulation_reduced_coordinate::PxArticulationReducedCoordinate<(), PxArticulationLink>;
type PxScene = physx::scene::PxScene<
    (),
    PxArticulationLink,
    PxRigidStatic,
    PxRigidDynamic,
    PxArticulation,
    PxArticulationReducedCoordinate,
    OnCollision,
    OnTrigger,
    OnConstraintBreak,
    OnWakeSleep,
    OnAdvance,
>;

/// Next up, the simulation event callbacks need to be defined, and possibly an
/// allocator callback as well.
pub struct OnCollision;
impl CollisionCallback for OnCollision {
    fn on_collision(
        &mut self,
        _header: &physx_sys::PxContactPairHeader,
        _pairs: &[physx_sys::PxContactPair],
    ) {
    }
}
pub struct OnTrigger;
impl TriggerCallback for OnTrigger {
    fn on_trigger(&mut self, _pairs: &[physx_sys::PxTriggerPair]) {}
}

pub struct OnConstraintBreak;
impl ConstraintBreakCallback for OnConstraintBreak {
    fn on_constraint_break(&mut self, _constraints: &[physx_sys::PxConstraintInfo]) {}
}
pub struct OnWakeSleep;
impl WakeSleepCallback<PxArticulationLink, PxRigidStatic, PxRigidDynamic> for OnWakeSleep {
    fn on_wake_sleep(
        &mut self,
        _actors: &[&physx::actor::ActorMap<PxArticulationLink, PxRigidStatic, PxRigidDynamic>],
        _is_waking: bool,
    ) {
    }
}

pub struct OnAdvance;
impl AdvanceCallback<PxArticulationLink, PxRigidDynamic> for OnAdvance {
    fn on_advance(
        &self,
        _actors: &[&physx::rigid_body::RigidBodyMap<PxArticulationLink, PxRigidDynamic>],
        _transforms: &[PxTransform],
    ) {
    }
}

pub struct RawPtrHolder {
    pub physx_collection: *mut PxCollection,
    pub physx_serialize_registry: *mut PxSerializationRegistry,
}

impl Drop for RawPtrHolder {
    fn drop(&mut self) {
        unsafe {
            physx_sys::phys_PxCloseVehicleSDK(self.physx_serialize_registry);
            physx_sys::PxCollection_release_mut(self.physx_collection);
            physx_sys::PxSerializationRegistry_release_mut(self.physx_serialize_registry);
        }
    }
}

// This needs to be ordered as such, as Rust by defaults drop members
// top to bottom as opposed as in C++ where it is bottom to top
pub struct PhysicsHandler {
    default_material: Owner<PxMaterial>,
    pub physx_scene: Owner<PxScene>,
    physx_cooking: Owner<PxCooking>,
    pub raw_ptr_holder: RawPtrHolder,
    pub physx_foundation: PhysicsFoundation<DefaultAllocator, PxShape>,
}

impl PhysicsHandler {
    pub fn new() -> PhysicsHandler {
        let mut physics = PhysicsFoundation::<_, PxShape>::default();
        let scene: Owner<PxScene> = physics.create(SceneDescriptor::new(())).unwrap();

        let mut cooking_params = PxCookingParams::new(physics.physics()).unwrap();
        cooking_params.obj.meshPreprocessParams.mBits =
            physx_sys::PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES;
        // let mut desc = unsafe{ physx_sys::PxMidphaseDesc_new() };
        // unsafe{
        //     physx_sys::PxMidphaseDesc_setToDefault_mut(&mut desc, physx_sys::PxMeshMidPhase::eBVH34);
        // }
        // cooking_params.obj.midphaseDesc = desc;
        let cooking =
            physx::cooking::PxCooking::new(physics.foundation_mut(), &cooking_params).unwrap();

        let default_material = physics.create_material(0.5, 0.5, 0.6, ()).unwrap();

        let collection = unsafe { physx_sys::phys_PxCreateCollection() };
        let registry = unsafe {
            physx_sys::PxSerialization_createSerializationRegistry_mut(
                physics.physics_mut().as_mut_ptr(),
            )
        };

        unsafe { physx_sys::phys_PxInitVehicleSDK(physics.physics_mut().as_mut_ptr(), registry) };

        PhysicsHandler {
            physx_foundation: physics,
            raw_ptr_holder: RawPtrHolder {
                physx_collection: collection,
                physx_serialize_registry: registry,
            },
            physx_cooking: cooking,
            physx_scene: scene,
            default_material,
        }
    }

    pub fn create_triangle_mesh(
        &mut self,
        vertices: &[f32],
        vertices_count: usize,
        vertices_stride: usize,
        indices: &[u32],
    ) -> Option<Owner<physx::triangle_mesh::TriangleMesh>> {
        // TODO: Currently all of the meshes are not indexed so ingore that for the moment
        let mut points_data = unsafe { PxBoundedData_new() };
        points_data.count = vertices_count as u32;
        points_data.data = vertices.as_ptr() as *const std::ffi::c_void;
        points_data.stride = vertices_stride as u32;

        let mut desc = physx::cooking::PxTriangleMeshDesc::new();
        desc.obj.points = points_data;

        //let triangles: Vec<u32> = (0..points_data.count).collect();
        let mut triangles_data = unsafe { PxBoundedData_new() };
        triangles_data.count = (indices.len() / 3) as u32;
        triangles_data.data = indices.as_ptr() as *const std::ffi::c_void;
        triangles_data.stride = (std::mem::size_of::<u32>() * 3) as u32;

        desc.obj.triangles = triangles_data;

        //desc.obj.flags.mBits = physx_sys::PxMeshFlag::eFLIPNORMALS as u16;

        if let TriangleMeshCookingResult::Success(mesh) = self
            .physx_cooking
            .create_triangle_mesh(self.physx_foundation.physics_mut(), &desc)
        {
            Some(mesh)
        } else {
            None
        }
    }

    pub fn create_convex_mesh(
        &mut self,
        vertices: &[f32],
        vertices_count: usize,
        vertices_stride: usize,
    ) -> Option<Owner<physx::convex_mesh::ConvexMesh>> {
        // TODO: Currently all of the meshes are not indexed so ingore that for the moment
        let mut points_data = unsafe { PxBoundedData_new() };
        points_data.count = vertices_count as u32;
        points_data.data = vertices.as_ptr() as *const std::ffi::c_void;
        points_data.stride = vertices_stride as u32;

        let mut desc = physx::cooking::PxConvexMeshDesc::new();
        desc.obj.points = points_data;
        desc.obj.flags.mBits = PxConvexFlag::eCOMPUTE_CONVEX as u16;
        desc.obj.vertexLimit = 256;

        let result = self
            .physx_cooking
            .create_convex_mesh(self.physx_foundation.physics_mut(), &desc);
        if let ConvexMeshCookingResult::Success(mesh) = result {
            Some(mesh)
        } else {
            match result {
                ConvexMeshCookingResult::ZeroAreaTestFailed => {
                    println!("Error creating convex mesh: ZeroAreaTestFailed")
                }
                ConvexMeshCookingResult::PolygonsLimitReached => {
                    println!("Error creating convex mesh: PolygonsLimitReached")
                }
                ConvexMeshCookingResult::Failure => println!("Error creating convex mesh: Failure"),
                ConvexMeshCookingResult::InvalidDescriptor => {
                    println!("Error creating convex mesh: InvalidDescriptor")
                }
                _ => {}
            }
            None
        }
    }

    pub fn create_mesh_geometry(
        &self,
        scale: PxVec3,
        mesh: &mut Owner<physx::triangle_mesh::TriangleMesh>,
    ) -> impl Geometry {
        let sys_vec: physx_sys::PxVec3 = scale.into();
        let mesh_scale = unsafe { PxMeshScale_new_2(&sys_vec as *const physx_sys::PxVec3) };
        PxTriangleMeshGeometry::new(mesh.as_mut(), &mesh_scale, PxMeshGeometryFlags { mBits: 0 })
    }

    pub fn create_convex_geometry(
        &self,
        scale: PxVec3,
        mesh: &mut Owner<physx::convex_mesh::ConvexMesh>,
    ) -> PxConvexMeshGeometry {
        let sys_vec: physx_sys::PxVec3 = scale.into();
        let mesh_scale = unsafe { PxMeshScale_new_2(&sys_vec as *const physx_sys::PxVec3) };
        PxConvexMeshGeometry::new(
            mesh.as_mut(),
            &mesh_scale,
            PxConvexMeshGeometryFlags { mBits: 0 },
        )
    }

    pub fn create_sphere_geometry(&self, radius: f32) -> impl Geometry {
        PxSphereGeometry::new(radius)
    }

    pub fn create_actor(&mut self) -> Owner<PxRigidDynamic> {
        self.physx_foundation
            .create_dynamic(&PxTransform::default(), ())
            .unwrap()
    }

    pub fn create_shape(
        &mut self,
        geometry: &impl Geometry,
        shape_filter_data: u32,
    ) -> Owner<PxShape> {
        let mut shape = self
            .physx_foundation
            .create_shape(
                geometry,
                &mut [self.default_material.as_mut()],
                true,
                ShapeFlag::Visualization | ShapeFlag::SceneQueryShape | ShapeFlag::SimulationShape,
                (),
            )
            .unwrap();

        let mut data = unsafe { physx_sys::PxFilterData_new_1() };
        unsafe {
            physx_sys::PxFilterData_setToDefault_mut(&mut data);
        }
        data.word3 = shape_filter_data;

        unsafe {
            physx_sys::PxShape_setQueryFilterData_mut(
                shape.as_mut_ptr(),
                &data as *const physx_sys::PxFilterData,
            )
        }
        shape
    }

    pub fn add_actor(
        &mut self,
        is_dynamic: bool,
        transform: PxTransform,
        geometry: &impl Geometry,
        id: u64,
        static_shape_filter_data: u32,
    ) {
        if is_dynamic {
            let mut actor = self
                .physx_foundation
                .create_rigid_dynamic(
                    transform,
                    geometry,
                    self.default_material.as_mut(),
                    10.0,
                    PxTransform::default(),
                    (),
                )
                .unwrap();

            unsafe {
                physx_sys::PxCollection_add_mut(
                    self.raw_ptr_holder.physx_collection,
                    actor.as_mut_ptr(),
                    id as usize,
                );
                physx_sys::PxSerialization_complete_mut(
                    self.raw_ptr_holder.physx_collection,
                    self.raw_ptr_holder.physx_serialize_registry,
                    std::ptr::null(),
                    false,
                );
            }

            self.physx_scene.add_dynamic_actor(actor);
        } else {
            let mut actor = self
                .physx_foundation
                .create_rigid_static(
                    transform,
                    geometry,
                    self.default_material.as_mut(),
                    PxTransform::default(),
                    (),
                )
                .unwrap();

            let mut shapes = actor.get_shapes_mut();
            assert!(shapes.len() == 1);

            let mut data = unsafe { physx_sys::PxFilterData_new_1() };
            unsafe {
                physx_sys::PxFilterData_setToDefault_mut(&mut data);
            }
            data.word3 = static_shape_filter_data;

            unsafe {
                physx_sys::PxShape_setQueryFilterData_mut(
                    shapes[0].as_mut_ptr(),
                    &data as *const physx_sys::PxFilterData,
                )
            }

            unsafe {
                physx_sys::PxCollection_add_mut(
                    self.raw_ptr_holder.physx_collection,
                    actor.as_mut_ptr(),
                    id as usize,
                );
                physx_sys::PxSerialization_complete_mut(
                    self.raw_ptr_holder.physx_collection,
                    self.raw_ptr_holder.physx_serialize_registry,
                    std::ptr::null(),
                    false,
                );
            }

            self.physx_scene.add_static_actor(actor);
        }
    }

    pub fn serialize(&mut self) -> Vec<u8> {
        unsafe {
            let output_stream = physx_sys::PxDefaultMemoryOutputStream_new_alloc(
                physx_sys::get_default_allocator() as *mut physx_sys::PxAllocatorCallback,
            );

            physx_sys::PxSerialization_serializeCollectionToBinaryDeterministic_mut(
                output_stream as *mut physx_sys::PxOutputStream,
                self.raw_ptr_holder.physx_collection,
                self.raw_ptr_holder.physx_serialize_registry,
                std::ptr::null(),
                false,
            );

            let size = physx_sys::PxDefaultMemoryOutputStream_getSize(output_stream);
            let data = physx_sys::PxDefaultMemoryOutputStream_getData(output_stream);
            let slice = std::slice::from_raw_parts(data, size as usize);
            let serialzed_vector = slice.to_owned();

            physx_sys::PxDefaultMemoryOutputStream_delete(output_stream);
            serialzed_vector
        }
    }
}
