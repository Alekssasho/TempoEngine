use std::sync::{Arc, Weak};

use crate::scene::Scene;
use data_definition_generated::{flatbuffer_derive::FlatbufferSerializeRoot, TextureMapping};

use crate::compiler::AsyncCompiler;

use super::{
    texture::{CompiledTextureData, TextureResource},
    Resource,
};
#[derive(Debug)]
pub struct TextureDatabaseResource {
    scene: Weak<Scene>,
}

impl TextureDatabaseResource {
    pub fn new(scene: Weak<Scene>) -> Self {
        Self { scene }
    }
}

#[derive(FlatbufferSerializeRoot)]
struct TextureDatabase {
    #[store_vector_direct]
    texture_data_buffer: Vec<u8>,
    #[store_vector_direct]
    mappings: Vec<data_definition_generated::TextureMapping>,
}

#[async_trait]
impl Resource for TextureDatabaseResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, compiler: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let scene = self.scene.upgrade().unwrap();
        // We can start preparing textures, as those are not dependant on other things (for now)
        let mut texture_futures = Vec::new();
        for texture in &scene.textures {
            let texture_resource = TextureResource::new(Arc::downgrade(&scene), *texture);
            let texture_compiler = compiler.clone(); // TODO: Make this global, and just write it once
            texture_futures.push(tokio::spawn(async move {
                texture_resource.compile(texture_compiler).await
            }));
        }

        // Wait for the texture to become ready
        let gathered_textures = futures::future::join_all(texture_futures)
            .await
            .into_iter()
            .map(Result::unwrap)
            .collect::<Vec<CompiledTextureData>>();
        assert!(gathered_textures.len() == scene.textures.len());

        let mut texture_data_buffer = Vec::<u8>::new();
        let mut mappings = Vec::new();

        for (index, texture) in scene.textures.iter().zip(gathered_textures.into_iter()) {
            mappings.push(TextureMapping::new(
                *index as u32,
                texture_data_buffer.len() as u32,
                texture.data.len() as u32,
                &texture.texture_info,
            ));
            texture_data_buffer.extend(texture.data.iter());
        }

        let database = TextureDatabase {
            texture_data_buffer,
            mappings,
        };

        database.serialize_root()
    }
}
