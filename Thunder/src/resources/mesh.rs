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
#[derive(Debug)]
pub struct MeshData {
    pub vertices: Vec<f32>,
    pub indices: Vec<u32>,
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
        for prim in 0..scene.gltf.mesh_primitive_count(self.mesh_index) {
            let mut prim_indices =
            if let Some(indices) = scene.gltf.mesh_indices(self.mesh_index, prim) {
                indices
            } else {
                (0..position_counts[prim] as u32).collect()
            };
            
            // Vertices array is of floats, 3 of which are a vertex
            //let indices_current_count = (orig_vertices.len() / 3) as u32;
            let positions = scene.gltf.mesh_positions(self.mesh_index, prim).unwrap();
            vertices.reserve(vertices.len() + prim_indices.len() * 3);
            for index in prim_indices.iter() {
                let position = positions[*index as usize];
                vertices.push(position[0]);
                vertices.push(position[1]);
                vertices.push(position[2]);
            }
        }
        let indices = (0..(vertices.len()/3) as u32).collect();
        
        MeshData { vertices, indices }
    }
}