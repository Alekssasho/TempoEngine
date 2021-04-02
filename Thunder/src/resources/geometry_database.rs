use std::sync::Weak;

use crate::scene::Scene;
use data_definition_generated::flatbuffer_derive::{FlatbufferSerialize, FlatbufferSerializeRoot};

use crate::compiler::AsyncCompiler;

use super::Resource;
#[derive(Debug)]
pub struct GeometryDatabaseResource {
    scene: Weak<Scene>,
}

impl GeometryDatabaseResource {
    pub fn new(scene: Weak<Scene>) -> Self {
        Self { scene }
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
        for mesh in &scene.meshes {
            let indices_counts = scene.gltf.mesh_indices_count_per_primitive(*mesh);
            let position_counts = scene.gltf.mesh_positions_count_per_primitive(*mesh);
            mappings.push(MeshMapping {
                index: *mesh as u32,
                vertex_offset: current_offset,
                vertex_count: indices_counts.iter().sum::<usize>() as u32,
            });

            // Fill up the vertex buffer
            let mut indices = Vec::new();
            for prim in 0..scene.gltf.mesh_primitive_count(*mesh) {
                let mut prim_indices = if let Some(indices) = scene.gltf.mesh_indices(*mesh, prim) {
                    indices
                } else {
                    (0..position_counts[prim] as u32).collect()
                };

                let indices_current_count = vertex_buffer.len() as u32;
                let positions = scene.gltf.mesh_positions(*mesh, prim).unwrap();
                vertex_buffer.reserve(vertex_buffer.len() + prim_indices.len() * 3);
                for index in prim_indices.iter() {
                    let position = positions[*index as usize];
                    vertex_buffer.push(position[0]);
                    vertex_buffer.push(position[1]);
                    vertex_buffer.push(position[2]);
                }
                prim_indices = prim_indices
                    .into_iter()
                    .map(|index| index + indices_current_count)
                    .collect();
                indices.append(&mut prim_indices);
            }

            // 3 is 3 float and 4 is num bytes for float
            current_offset += (indices_counts[0] as u32) * 3 * 4;
        }

        let vertex_buffer_bytes = unsafe { (vertex_buffer[..].align_to::<u8>()).1 };

        let database = GeometryDatabase {
            vertex_buffer: vertex_buffer_bytes,
            mappings,
        };

        database.serialize_root()
    }
}
