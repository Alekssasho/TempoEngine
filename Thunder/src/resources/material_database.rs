use std::{sync::Weak, u32};

use data_definition_generated::{ColorSpace, TextureFormat};

use crate::scene::Scene;

use crate::compiler::AsyncCompiler;

use super::{texture::TextureRequest, Resource};
#[derive(Debug)]
pub struct MaterialDatabaseResource {
    scene: Weak<Scene>,
}

impl MaterialDatabaseResource {
    pub fn new(scene: Weak<Scene>) -> Self {
        Self { scene }
    }
}

#[async_trait]
impl Resource for MaterialDatabaseResource {
    type ReturnValue = (
        Vec<TextureRequest>,
        Vec<data_definition_generated::Material>,
    );

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Self::ReturnValue {
        let mut materials = Vec::new();
        let mut texture_requests = Vec::new();

        let scene = self.scene.upgrade().unwrap();
        materials.reserve(scene.materials.len());
        for material_index in &scene.materials {
            let gltf_material = scene.gltf.material(*material_index);
            // For now we only support textures
            let texture_index = if let Some(texture) =
                gltf_material.pbr_metallic_roughness().base_color_texture()
            {
                let texture_index = texture.texture().source().index();
                texture_requests.push(TextureRequest {
                    texture_index,
                    color_space: ColorSpace::sRGB,
                    format: TextureFormat::BC1_RGB,
                });
                (texture_requests.len() - 1) as u32
            } else {
                u32::MAX
            };
            let texture_color = gltf_material.pbr_metallic_roughness().base_color_factor();

            materials.push(data_definition_generated::Material::new(
                &data_definition_generated::Color::new(
                    texture_color[0],
                    texture_color[1],
                    texture_color[2],
                    texture_color[3],
                ),
                texture_index,
            ));
        }

        (texture_requests, materials)
    }
}
