use std::sync::Weak;

use basis_universal::{Compressor, CompressorParams, TranscodeParameters, Transcoder};
use data_definition_generated::{ColorSpace, TextureData, TextureFormat};

use crate::{compiler::AsyncCompiler, scene::Scene};

use super::Resource;

#[derive(Debug, Clone, Copy)]
pub struct TextureRequest {
    pub texture_index: usize,
    pub color_space: ColorSpace,
    pub format: TextureFormat,
}

#[derive(Debug)]
pub struct TextureResource {
    scene: Weak<Scene>,
    request: TextureRequest,
}

impl TextureResource {
    pub fn new(scene: Weak<Scene>, request: TextureRequest) -> Self {
        Self { scene, request }
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

        let image_data = &scene.gltf.images[self.request.texture_index];
        let channel_count = match image_data.format {
            gltf::image::Format::R8 | gltf::image::Format::R16 => 1,
            gltf::image::Format::R8G8 | gltf::image::Format::R16G16 => 2,
            gltf::image::Format::R8G8B8
            | gltf::image::Format::B8G8R8
            | gltf::image::Format::R16G16B16 => 3,
            gltf::image::Format::R8G8B8A8
            | gltf::image::Format::B8G8R8A8
            | gltf::image::Format::R16G16B16A16 => 4,
        };

        let mut compressor_params = CompressorParams::new();
        compressor_params.set_generate_mipmaps(false);
        compressor_params.set_basis_format(basis_universal::BasisTextureFormat::UASTC4x4);
        compressor_params.set_uastc_quality_level(basis_universal::UASTC_QUALITY_DEFAULT);
        compressor_params.set_print_status_to_stdout(false);

        compressor_params.set_color_space( match self.request.color_space {
            ColorSpace::Linear => basis_universal::ColorSpace::Linear,
            ColorSpace::sRGB => basis_universal::ColorSpace::Srgb,
        });

        let mut compressor_image = compressor_params.source_image_mut(0);
        compressor_image.init(
            image_data.pixels.as_slice(),
            image_data.width,
            image_data.height,
            channel_count,
        );

        let mut compressor = Compressor::default();
        unsafe {
            compressor.init(&compressor_params);
            compressor.process().unwrap();
        }

        let basis_file = compressor.basis_file();

        let mut transcoder = Transcoder::new();
        transcoder.prepare_transcoding(basis_file).unwrap();
        let result = transcoder
            .transcode_image_level(
                basis_file,
                match self.request.format {
                    TextureFormat::RGBA8 => basis_universal::TranscoderTextureFormat::RGBA32,
                    TextureFormat::BC1_RGB => basis_universal::TranscoderTextureFormat::BC1_RGB,
                    TextureFormat::BC7_RGBA => basis_universal::TranscoderTextureFormat::BC7_RGBA,
                } ,
                TranscodeParameters {
                    image_index: 0,
                    level_index: 0,
                    ..Default::default()
                },
            )
            .unwrap();

        let image_description = transcoder
            .image_level_description(basis_file, 0, 0)
            .unwrap();

        CompiledTextureData {
            data: result,
            texture_info: TextureData::new(
                image_description.original_width,
                image_description.original_height,
                self.request.format,
                self.request.color_space,
            ),
        }
    }
}
