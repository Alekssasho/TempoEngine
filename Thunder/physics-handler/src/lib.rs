use physx::{
    cooking::{PxCooking, PxCookingParams, TriangleMeshCookingResult},
    foundation::DefaultAllocator,
    prelude::*,
    traits::Class,
};

pub use physx::math::{PxQuat, PxTransform, PxVec3};
use physx_sys::{PxBoundedData_new, PxCollection, PxMeshFlag::eFLIPNORMALS, PxMeshGeometryFlags, PxMeshScale_new_2, PxSerializationRegistry};

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
struct OnCollision;
impl CollisionCallback for OnCollision {
    fn on_collision(
        &mut self,
        _header: &physx_sys::PxContactPairHeader,
        _pairs: &[physx_sys::PxContactPair],
    ) {
    }
}
struct OnTrigger;
impl TriggerCallback for OnTrigger {
    fn on_trigger(&mut self, _pairs: &[physx_sys::PxTriggerPair]) {}
}

struct OnConstraintBreak;
impl ConstraintBreakCallback for OnConstraintBreak {
    fn on_constraint_break(&mut self, _constraints: &[physx_sys::PxConstraintInfo]) {}
}
struct OnWakeSleep;
impl WakeSleepCallback<PxArticulationLink, PxRigidStatic, PxRigidDynamic> for OnWakeSleep {
    fn on_wake_sleep(
        &mut self,
        _actors: &[&physx::actor::ActorMap<PxArticulationLink, PxRigidStatic, PxRigidDynamic>],
        _is_waking: bool,
    ) {
    }
}

struct OnAdvance;
impl AdvanceCallback<PxArticulationLink, PxRigidDynamic> for OnAdvance {
    fn on_advance(
        &self,
        _actors: &[&physx::rigid_body::RigidBodyMap<PxArticulationLink, PxRigidDynamic>],
        _transforms: &[PxTransform],
    ) {
    }
}

struct RawPtrHolder {
    physx_collection: *mut PxCollection,
    physx_serialize_registry: *mut PxSerializationRegistry,
}

impl Drop for RawPtrHolder {
    fn drop(&mut self) {
        unsafe {
            physx_sys::PxCollection_release_mut(self.physx_collection);
            physx_sys::PxSerializationRegistry_release_mut(self.physx_serialize_registry);
        }
    }
}

// This needs to be ordered as such, as Rust by defaults drop members
// top to bottom as opposed as in C++ where it is bottom to top
pub struct PhysicsHandler {
    default_material: Owner<PxMaterial>,
    physx_scene: Owner<PxScene>,
    physx_cooking: Owner<PxCooking>,
    raw_ptr_holder: RawPtrHolder,
    physx_foundation: PhysicsFoundation<DefaultAllocator, PxShape>,
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
        indices: &[u32]
    ) -> Option<Owner<physx::triangle_mesh::TriangleMesh>> {
        // TODO: Currently all of the meshes are not indexed so ingore that for the moment
        let mut points_data = unsafe { PxBoundedData_new() };
        points_data.count = (vertices.len() / 3) as u32;
        points_data.data = vertices.as_ptr() as *const std::ffi::c_void;
        points_data.stride = (std::mem::size_of::<f32>() * 3) as u32;

        let mut desc = physx::cooking::PxTriangleMeshDesc::new();
        desc.obj.points = points_data;

        //let triangles: Vec<u32> = (0..points_data.count).collect();
        let mut triangles_data = unsafe { PxBoundedData_new() };
        triangles_data.count = (indices.len() / 3) as u32;
        triangles_data.data = indices.as_ptr() as *const std::ffi::c_void;
        triangles_data.stride = (std::mem::size_of::<u32>() * 3) as u32;

        desc.obj.triangles = triangles_data;

        //desc.obj.flags.mBits = eFLIPNORMALS as u16;

        if let TriangleMeshCookingResult::Success(mesh) = self
            .physx_cooking
            .create_triangle_mesh(self.physx_foundation.physics_mut(), &desc)
        {
            Some(mesh)
        } else {
            None
        }
    }

    pub fn create_mesh_geometry(&self, scale: PxVec3, mesh: &mut Owner<physx::triangle_mesh::TriangleMesh>) -> impl Geometry {
        let sys_vec: physx_sys::PxVec3 = scale.into();
        let mesh_scale = unsafe { PxMeshScale_new_2(&sys_vec as *const physx_sys::PxVec3) };
        PxTriangleMeshGeometry::new(
            mesh.as_mut(),
            &mesh_scale,
            PxMeshGeometryFlags { mBits: 0 },
        )
    }

    pub fn create_sphere_geometry(&self, radius: f32) -> impl Geometry {
        PxSphereGeometry::new(radius)
    }

    pub fn add_actor(
        &mut self,
        is_dynamic: bool,
        transform: PxTransform,
        geometry: &impl Geometry,
        id: u64,
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