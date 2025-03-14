use std::sync::Weak;

use crate::{compiler::AsyncCompiler, scene::Scene};
use itertools::izip;

use super::Resource;
#[derive(Debug)]
pub struct MeshResource {
    scene: Weak<Scene>,
    mesh_index: usize,
}

impl MeshResource {
    pub fn new(scene: Weak<Scene>, mesh_index: usize) -> Self {
        Self { scene, mesh_index }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone, Default)]
pub struct VertexLayout {
    pub position: math::Vec3,
    pub normal: math::Vec3,
    pub uv: math::Vec2,
}

#[derive(Debug)]
pub struct PrimitiveMeshData {
    pub meshlets: Vec<meshopt::Meshlet>,
    pub vertices: Vec<VertexLayout>,
    pub meshlet_indices: Vec<u8>,
    pub whole_mesh_indices: Vec<u32>,
    pub simplyfied_mesh_indices: Vec<u32>,
    pub material_index: usize,
}

impl PrimitiveMeshData {
    pub fn get_vertices_float_slice(&self) -> &[f32] {
        unsafe { self.vertices.as_slice().align_to::<f32>().1 }
    }
}

#[derive(Debug)]
pub struct MeshData {
    pub primitive_meshes: Vec<PrimitiveMeshData>,
}

#[async_trait]
impl Resource for MeshResource {
    type ReturnValue = MeshData;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Self::ReturnValue {
        let scene = self.scene.upgrade().unwrap();

        // Extract Mesh data from GLTF
        let position_counts = scene
            .gltf
            .mesh_positions_count_per_primitive(self.mesh_index);

        let primitive_count = scene.gltf.mesh_primitive_count(self.mesh_index);
        let mut primitive_meshes = Vec::with_capacity(primitive_count);
        // TODO: Every primitive mesh, could be a seperate task instead of doing it inline
        for prim in 0..primitive_count {
            let mut vertices = Vec::new();
            let mut indices = if let Some(indices) = scene.gltf.mesh_indices(self.mesh_index, prim)
            {
                indices
            } else {
                (0..position_counts[prim] as u32).collect()
            };

            // Tempest is LH system and GLTF is RH, so we need to change the winding of the triangles
            // to use the proper positive rotation
            assert!(indices.len() % 3 == 0);
            for i in 0..(indices.len() / 3) {
                let index = indices[i * 3 + 1];
                indices[i * 3 + 1] = indices[i * 3 + 2];
                indices[i * 3 + 2] = index;
            }

            let positions = scene.gltf.mesh_positions(self.mesh_index, prim).unwrap();
            let normals = scene.gltf.mesh_normals(self.mesh_index, prim).unwrap();
            let uvs = scene.gltf.mesh_uvs(self.mesh_index, prim).unwrap();

            vertices.reserve(positions.len());
            for (position, normal, uv) in izip!(positions, normals, uvs) {
                // GLTF uses RH coordinate system in which +Z is front, +Y is up, and +X is left, according to specs
                // Tempest uses LH coordinate system with +Z is front, +Y is up nad +X is right, so we need to invert only X coordinate
                vertices.push(VertexLayout {
                    position: math::vec3(-position.x, position.y, position.z),
                    normal: math::vec3(-normal.x, normal.y, normal.z),
                    uv,
                });
            }

            let (_, remap_table) =
                meshopt::generate_vertex_remap(vertices.as_slice(), Some(indices.as_slice()));
            indices = meshopt::remap_index_buffer(
                Some(indices.as_slice()),
                vertices.len(),
                remap_table.as_slice(),
            );
            vertices = meshopt::remap_vertex_buffer(
                vertices.as_slice(),
                vertices.len(),
                remap_table.as_slice(),
            );

            meshopt::optimize_vertex_cache_in_place(indices.as_slice(), vertices.len());
            meshopt::optimize_vertex_fetch_in_place(
                indices.as_mut_slice(),
                vertices.as_mut_slice(),
            );

            let vertex_adapter = meshopt::VertexDataAdapter::new(
                unsafe { vertices.as_slice().align_to::<u8>().1 },
                std::mem::size_of::<VertexLayout>(),
                0,
            )
            .unwrap();

            // TODO: find better values for max vertices/triangles
            let (meshlets, meshlet_vertices, meshlet_indices) =
                meshopt::build_meshlets(indices.as_slice(), &vertex_adapter, 128, 128, 0.0);

            // Currently every meshlet have position and count inside the meshlet_vertices/meshlet_indices arrays. meshlet_vertices has indices inside the vertices array
            // For now lets remove the indirection
            vertices = {
                let mut ordered_vertices = Vec::<VertexLayout>::new();
                ordered_vertices.reserve(meshlet_vertices.len());
                for vertex_index in meshlet_vertices {
                    ordered_vertices.push(vertices[vertex_index as usize]);
                }
                ordered_vertices
            };

            // Prepare whole mesh indices
            let whole_mesh_indices = {
                let mut indices = Vec::new();
                indices.reserve(meshlet_indices.len());
                for meshlet in &meshlets {
                    for index in &meshlet_indices[(meshlet.triangle_offset as usize)
                        ..(meshlet.triangle_offset + (meshlet.triangle_count * 3)) as usize]
                    {
                        indices.push(*index as u32 + meshlet.vertex_offset);
                    }
                }
                indices
            };

            let vertex_adapter = meshopt::VertexDataAdapter::new(
                unsafe { vertices.as_slice().align_to::<u8>().1 },
                std::mem::size_of::<VertexLayout>(),
                0,
            )
            .unwrap();
            let simplyfied_mesh_indices = meshopt::simplify_sloppy(
                whole_mesh_indices.as_slice(),
                &vertex_adapter,
                256.min(whole_mesh_indices.len()),
                1.0,
            );
            primitive_meshes.push(PrimitiveMeshData {
                meshlets,
                vertices,
                meshlet_indices,
                whole_mesh_indices,
                simplyfied_mesh_indices,
                material_index: scene
                    .gltf
                    .mesh_material_index(self.mesh_index, prim)
                    .unwrap(),
            });
        }

        MeshData { primitive_meshes }
    }
}
