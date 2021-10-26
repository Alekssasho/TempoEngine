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
            let pbr_data = gltf_material.pbr_metallic_roughness();

            let base_texture_index = if let Some(texture) = pbr_data.base_color_texture() {
                let texture_index = texture.texture().source().index();
                texture_requests.push(TextureRequest {
                    texture_index,
                    color_space: ColorSpace::sRGB, // Color data is sRGB encoded in GLTF
                    format: TextureFormat::BC1_RGB,
                });
                (texture_requests.len() - 1) as u32
            } else {
                u32::MAX
            };

            let metallic_roughness_texture_index =
                if let Some(texture) = pbr_data.metallic_roughness_texture() {
                    let texture_index = texture.texture().source().index();
                    texture_requests.push(TextureRequest {
                        texture_index,
                        color_space: ColorSpace::Linear, // This are non color values so use Linear
                        format: TextureFormat::BC1_RGB, // TODO: Is this okay to be compressed with BC1 ?
                    });
                    (texture_requests.len() - 1) as u32
                } else {
                    u32::MAX
                };

            let base_texture_color_factor = pbr_data.base_color_factor();
            let metallic_factor = pbr_data.metallic_factor();
            let roughness_factor = pbr_data.roughness_factor();

            materials.push(data_definition_generated::Material::new(
                &data_definition_generated::Color::new(
                    base_texture_color_factor[0],
                    base_texture_color_factor[1],
                    base_texture_color_factor[2],
                    base_texture_color_factor[3],
                ),
                metallic_factor,
                roughness_factor,
                base_texture_index,
                metallic_roughness_texture_index,
            ));
        }

        (texture_requests, materials)
    }
}
