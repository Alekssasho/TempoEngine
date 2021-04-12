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
    vertex_offset: u32,
    vertex_count: u32,
}
#[derive(FlatbufferSerializeRoot)]
struct GeometryDatabase<'a> {
    #[offset]
    vertex_buffer: &'a [u8],
    #[offset]
    mappings: Vec<MeshMapping>,
}

#[async_trait]
impl Resource for GeometryDatabaseResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let mut vertex_buffer = Vec::<f32>::new();
        let scene = self.scene.upgrade().unwrap();

        let mut mappings = Vec::new();
        let mut current_offset = 0;
        for (mesh_index, mesh_data) in scene.meshes.iter().zip(self.meshes.iter()) {
            mappings.push(MeshMapping {
                index: *mesh_index as u32,
                vertex_offset: current_offset, // In bytes
                vertex_count: mesh_data.indices.len() as u32,
            });

            // 3 is 3 float and 4 is num bytes for float
            current_offset += mesh_data.indices.len() as u32 * 3 * 4;

            vertex_buffer.extend(&mesh_data.vertices);
        }

        let vertex_buffer_bytes = unsafe { (vertex_buffer[..].align_to::<u8>()).1 };

        let database = GeometryDatabase {
            vertex_buffer: vertex_buffer_bytes,
            mappings,
        };

        database.serialize_root()
    }
}
