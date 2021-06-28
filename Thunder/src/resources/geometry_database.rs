use std::sync::{Arc, Weak};

use crate::scene::Scene;
use data_definition_generated::flatbuffer_derive::FlatbufferSerializeRoot;

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

#[derive(FlatbufferSerializeRoot)]
struct GeometryDatabase<'a> {
    #[store_vector_direct]
    vertex_buffer: &'a [u8],
    #[store_vector_direct]
    meshlet_indices_buffer: Vec<u8>,
    #[store_vector_direct]
    meshlet_buffer: Vec<data_definition_generated::Meshlet>,
    #[store_vector_direct]
    primitive_meshes: Vec<data_definition_generated::PrimitiveMeshData>,
    #[store_vector_direct]
    materials: Vec<data_definition_generated::Material>,
    #[store_vector_direct]
    mappings: Vec<data_definition_generated::MeshMapping>,
}

#[async_trait]
impl Resource for GeometryDatabaseResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let mut vertex_buffer = Vec::<f32>::new();
        let mut meshlet_indices_buffer = Vec::<u8>::new();
        let mut meshlets = Vec::new();
        let mut primitive_meshes = Vec::new();
        let mut materials = Vec::new();
        let mut mappings = Vec::new();

        // TODO: Proper materials
        materials.push(data_definition_generated::Material::new(
            &data_definition_generated::Color::new(0.0, 1.0, 1.0, 1.0),
        ));

        let scene = self.scene.upgrade().unwrap();
        let mut current_vertex_buffer_offset = 0;
        let mut current_indices_buffer_offset = 0;
        let mut current_meshlet_buffer_offset = 0;
        for (mesh_index, mesh_data) in scene.meshes.iter().zip(self.meshes.iter()) {
            mappings.push(data_definition_generated::MeshMapping::new(
                *mesh_index as u32,
                &data_definition_generated::MeshData::new(
                    primitive_meshes.len() as u32,
                    mesh_data.primitive_meshes.len() as u32,
                ),
            ));

            primitive_meshes.reserve(primitive_meshes.len() + mesh_data.primitive_meshes.len());
            for primitive_mesh in &mesh_data.primitive_meshes {
                let meshlets_count = primitive_mesh.meshlets.len() as u32;

                // Fill up buffers
                meshlets.reserve(meshlets.len() + meshlets_count as usize);
                for meshlet in &primitive_mesh.meshlets {
                    meshlets.push(data_definition_generated::Meshlet::new(
                        current_vertex_buffer_offset + meshlet.vertex_offset,
                        meshlet.vertex_count,
                        current_indices_buffer_offset + meshlet.triangle_offset,
                        meshlet.triangle_count,
                    ));
                }

                vertex_buffer.extend(primitive_mesh.get_vertices_float_slice());

                meshlet_indices_buffer.extend(primitive_mesh.meshlet_indices.as_slice());

                primitive_meshes.push(data_definition_generated::PrimitiveMeshData::new(
                    current_meshlet_buffer_offset,
                    meshlets_count,
                    0,
                ));

                // Update the current offsets
                current_meshlet_buffer_offset += primitive_mesh.meshlets.len() as u32;
                current_vertex_buffer_offset += primitive_mesh.vertices.len() as u32;
                current_indices_buffer_offset += primitive_mesh.meshlet_indices.len() as u32;
            }
        }

        let vertex_buffer_bytes = unsafe { (vertex_buffer[..].align_to::<u8>()).1 };

        let database = GeometryDatabase {
            vertex_buffer: vertex_buffer_bytes,
            meshlet_indices_buffer,
            meshlet_buffer: meshlets,
            primitive_meshes,
            materials,
            mappings,
        };

        database.serialize_root()
    }
}
