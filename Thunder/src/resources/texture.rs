use std::sync::Weak;

use data_definition_generated::{TextureData, TextureFormat};

use crate::{compiler::AsyncCompiler, scene::Scene};

use super::Resource;
#[derive(Debug)]
pub struct TextureResource {
    scene: Weak<Scene>,
    texture_index: usize,
}

impl TextureResource {
    pub fn new(scene: Weak<Scene>, texture_index: usize) -> Self {
        Self {
            scene,
            texture_index,
        }
    }
}

#[derive(Debug)]
pub struct CompiledTextureData {
    pub data: Vec<u8>,
    pub texture_info: TextureData,
}

#[async_trait]
impl Resource for TextureResource {
    type ReturnValue = CompiledTextureData;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Self::ReturnValue {
        let scene = self.scene.upgrade().unwrap();

        // // Extract Mesh data from GLTF
        // let position_counts = scene
        //     .gltf
        //     .mesh_positions_count_per_primitive(self.mesh_index);

        // let primitive_count = scene.gltf.mesh_primitive_count(self.mesh_index);
        // let mut primitive_meshes = Vec::with_capacity(primitive_count);
        // // TODO: Every primitive mesh, could be a seperate task instead of doing it inline
        // for prim in 0..primitive_count {
        //     let mut vertices = Vec::new();
        //     let mut indices = if let Some(indices) = scene.gltf.mesh_indices(self.mesh_index, prim)
        //     {
        //         indices
        //     } else {
        //         (0..position_counts[prim] as u32).collect()
        //     };

        //     let positions = scene.gltf.mesh_positions(self.mesh_index, prim).unwrap();
        //     let normals = scene.gltf.mesh_normals(self.mesh_index, prim).unwrap();

        //     vertices.reserve(positions.len());
        //     for (position, normal) in positions.into_iter().zip(normals.into_iter()) {
        //         vertices.push(VertexLayout { position, normal });
        //     }

        //     let (_, remap_table) =
        //         meshopt::generate_vertex_remap(vertices.as_slice(), Some(indices.as_slice()));
        //     indices = meshopt::remap_index_buffer(
        //         Some(indices.as_slice()),
        //         vertices.len(),
        //         remap_table.as_slice(),
        //     );
        //     vertices = meshopt::remap_vertex_buffer(
        //         vertices.as_slice(),
        //         vertices.len(),
        //         remap_table.as_slice(),
        //     );

        //     meshopt::optimize_vertex_cache_in_place(indices.as_slice(), vertices.len());
        //     meshopt::optimize_vertex_fetch_in_place(
        //         indices.as_mut_slice(),
        //         vertices.as_mut_slice(),
        //     );

        //     let vertex_adapter = meshopt::VertexDataAdapter::new(
        //         unsafe { vertices.as_slice().align_to::<u8>().1 },
        //         std::mem::size_of::<VertexLayout>(),
        //         0,
        //     )
        //     .unwrap();

        //     // TODO: find better values for max vertices/triangles
        //     let (meshlets, meshlet_vertices, meshlet_indices) =
        //         meshopt::build_meshlets(indices.as_slice(), &vertex_adapter, 128, 128, 0.0);

        //     // Currently every meshlet have position and count inside the meshlet_vertices/meshlet_indices arrays. meshlet_vertices has indices inside the vertices array
        //     // For now lets remove the indirection
        //     vertices = {
        //         let mut ordered_vertices = Vec::<VertexLayout>::new();
        //         ordered_vertices.reserve(meshlet_vertices.len());
        //         for vertex_index in meshlet_vertices {
        //             ordered_vertices.push(vertices[vertex_index as usize]);
        //         }
        //         ordered_vertices
        //     };

        //     // Prepare whole mesh indices
        //     let whole_mesh_indices = {
        //         let mut indices = Vec::new();
        //         indices.reserve(meshlet_indices.len());
        //         for meshlet in &meshlets {
        //             for index in &meshlet_indices[(meshlet.triangle_offset as usize)
        //                 ..(meshlet.triangle_offset + (meshlet.triangle_count * 3)) as usize]
        //             {
        //                 indices.push(*index as u32 + meshlet.vertex_offset);
        //             }
        //         }
        //         indices
        //     };
        //     primitive_meshes.push(PrimitiveMeshData {
        //         meshlets,
        //         vertices,
        //         meshlet_indices,
        //         whole_mesh_indices,
        //     });
        // }

        // MeshData { primitive_meshes }
        CompiledTextureData {
            data: Vec::new(),
            texture_info: TextureData::new(1, 1, TextureFormat::RGBA8),
        }
    }
}
