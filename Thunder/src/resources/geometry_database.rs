use std::sync::{Arc, Weak};

use crate::scene::Scene;
use data_definition_generated::flatbuffer_derive::{FlatbufferSerialize, FlatbufferSerializeRoot};

use crate::compiler::AsyncCompiler;

use super::{mesh::MeshData, Resource};
#[derive(Debug)]
pub struct GeometryDatabaseResource {
    scene: Weak<Scene>,
    meshes: Arc<Vec<MeshData>>,
}

impl GeometryDatabaseResource {
    pub fn new(scene: Weak<Scene>, meshes: Arc<Vec<MeshData>>) -> Self {
        Self { scene, meshes }
    }
}
#[derive(FlatbufferSerialize)]
struct MeshMapping {
    index: u32,
    meshlets_offset: u32,
    meshlets_count: u32,
}
#[derive(FlatbufferSerializeRoot)]
struct GeometryDatabase<'a> {
    #[store_vector_direct]
    vertex_buffer: &'a [u8],
    #[store_vector_direct]
    meshlet_indices_buffer: Vec<u8>,
    #[store_vector_direct]
    meshlet_buffer: Vec<data_definition_generated::Meshlet>,
    #[store_vector_offsets]
    mappings: Vec<MeshMapping>,
}

#[async_trait]
impl Resource for GeometryDatabaseResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let mut vertex_buffer = Vec::<f32>::new();
        let mut meshlet_indices_buffer = Vec::<u8>::new();
        let mut meshlets = Vec::new();
        let mut mappings = Vec::new();

        let scene = self.scene.upgrade().unwrap();
        let mut current_vertex_buffer_offset = 0;
        let mut current_indices_buffer_offset = 0;
        let mut current_meshlet_buffer_offset = 0;
        for (mesh_index, mesh_data) in scene.meshes.iter().zip(self.meshes.iter()) {
            let meshlets_count = mesh_data.meshlets.len() as u32;
            // Fill up buffers
            mappings.push(MeshMapping {
                index: *mesh_index as u32,
                meshlets_offset: current_meshlet_buffer_offset,
                meshlets_count: meshlets_count,
            });

            meshlets.reserve(meshlets.len() + meshlets_count as usize);
            for meshlet in &mesh_data.meshlets {
                meshlets.push(data_definition_generated::Meshlet::new(
                    current_vertex_buffer_offset + meshlet.vertex_offset,
                    meshlet.vertex_count,
                    current_indices_buffer_offset + meshlet.triangle_offset,
                    meshlet.triangle_count,
                ));
            }

            vertex_buffer.extend(mesh_data.get_vertices_float_slice());

            meshlet_indices_buffer.extend(mesh_data.meshlet_indices.as_slice());

            // Update the current offsets
            current_meshlet_buffer_offset += mesh_data.meshlets.len() as u32;
            current_vertex_buffer_offset += mesh_data.vertices.len() as u32;
            current_indices_buffer_offset += mesh_data.meshlet_indices.len() as u32;
        }

        let vertex_buffer_bytes = unsafe { (vertex_buffer[..].align_to::<u8>()).1 };

        let database = GeometryDatabase {
            vertex_buffer: vertex_buffer_bytes,
            meshlet_indices_buffer,
            meshlet_buffer: meshlets,
            mappings,
        };

        database.serialize_root()
    }
}
