use std::rc::Weak;

use data_definition_generated::{
    flatbuffers, GeometryDatabase, GeometryDatabaseArgs, MeshMapping, MeshMappingArgs,
    GEOMETRY_DATABASE_IDENTIFIER,
};
use gltf_loader::Scene;

use crate::compiler::{CompiledResources, CompilerGraph, ResourceBox};

use super::{Resource, ResourceId};

pub struct GeometryDatabaseResource {
    scene: Weak<Scene>,
}

impl GeometryDatabaseResource {
    pub fn new(scene: Weak<Scene>) -> Self {
        Self { scene }
    }
}

impl Resource for GeometryDatabaseResource {
    fn extract_dependencies(
        &mut self,
        _compiler: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        vec![]
    }

    fn compile(&self, _compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
        let mut vertex_buffer = Vec::<f32>::new();
        let scene = self.scene.upgrade().unwrap();
        let meshes = scene.gather_meshes();

        let mut mappings = Vec::new();
        let mut current_offset = 0;
        for mesh in meshes {
            assert!(mesh.primitive_count() == 1);
            let indices_counts = mesh.indices_counts();
            let position_counts = mesh.position_counts();
            mappings.push(MeshMapping::create(
                &mut builder,
                &MeshMappingArgs {
                    index: mesh.index(),
                    vertex_offset: current_offset,
                    vertex_count: indices_counts[0] as u32,
                },
            ));

            // Fill up the vertex buffer
            let indices = if let Some(indices) = mesh.indices(0) {
                indices
            } else {
                (0..position_counts[0] as u32).collect()
            };

            let positions = mesh.positions(0).unwrap();
            vertex_buffer.reserve(indices.len() * 3);
            for index in indices {
                let position = positions[index as usize];
                vertex_buffer.push(position[0]);
                vertex_buffer.push(position[1]);
                vertex_buffer.push(position[2]);
            }

            // 3 is 3 float and 4 is num bytes for float
            current_offset += (indices_counts[0] as u32) * 3 * 4;
        }

        let mappings_offset = builder.create_vector(&mappings[..]);
        let vertex_buffer_bytes = unsafe { (&vertex_buffer[..].align_to::<u8>()).1 };
        let vertex_buffer_offset = builder.create_vector(vertex_buffer_bytes);
        let root_level = GeometryDatabase::create(
            &mut builder,
            &GeometryDatabaseArgs {
                vertex_buffer: Some(vertex_buffer_offset),
                mappings: Some(mappings_offset),
            },
        );
        builder.finish(root_level, Some(GEOMETRY_DATABASE_IDENTIFIER));
        Vec::from(builder.finished_data())
    }
}
