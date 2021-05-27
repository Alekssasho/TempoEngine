use std::sync::Weak;

use crate::{compiler::AsyncCompiler, scene::Scene};

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
#[derive(Debug, Copy, Clone, Default)]
pub struct VertexLayout {
    position: math::Vec3,
    normal: math::Vec3,
}

#[derive(Debug)]
pub struct MeshData {
    pub meshlets: Vec<meshopt::Meshlet>,
    pub vertices: Vec<VertexLayout>,
    pub meshlet_indices: Vec<u8>,
    pub whole_mesh_indices: Vec<u32>,
}

impl MeshData {
    pub fn get_vertices_float_slice(&self) -> &[f32] {
        unsafe { self.vertices.as_slice().align_to::<f32>().1 }
    }
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

        // Fill up the vertex buffer
        let mut vertices = Vec::new();
        let mut indices = Vec::new();
        for prim in 0..scene.gltf.mesh_primitive_count(self.mesh_index) {
            let prim_indices = if let Some(indices) = scene.gltf.mesh_indices(self.mesh_index, prim)
            {
                indices
            } else {
                (0..position_counts[prim] as u32).collect()
            };

            let positions = scene.gltf.mesh_positions(self.mesh_index, prim).unwrap();
            let normals = scene.gltf.mesh_normals(self.mesh_index, prim).unwrap();

            let current_vertex_offset = vertices.len() as u32;
            vertices.reserve(vertices.len() + prim_indices.len());
            for (position, normal) in positions.into_iter().zip(normals.into_iter()) {
                vertices.push(VertexLayout { position, normal });
            }

            indices.reserve(indices.len() + prim_indices.len());
            indices.extend(prim_indices.iter().map(|index| index + current_vertex_offset));
        }

        let (_, remap_table) = meshopt::generate_vertex_remap(vertices.as_slice(), Some(indices.as_slice()));
        indices = meshopt::remap_index_buffer(Some(indices.as_slice()), vertices.len(), remap_table.as_slice());
        vertices = meshopt::remap_vertex_buffer(vertices.as_slice(), vertices.len(), remap_table.as_slice());

        meshopt::optimize_vertex_cache_in_place(indices.as_slice(), vertices.len());
        meshopt::optimize_vertex_fetch_in_place(indices.as_mut_slice(), vertices.as_mut_slice());

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

        MeshData {
            meshlets,
            vertices,
            meshlet_indices,
            whole_mesh_indices,
        }
    }
}
