use super::Resource;
use crate::compiler::AsyncCompiler;
use data_definition_generated::flatbuffer_derive::FlatbufferSerializeRoot;
use tokio::io::AsyncReadExt;
#[derive(Debug)]
pub struct AudioDatabaseResource {}

#[derive(FlatbufferSerializeRoot)]
struct AudioDatabase {
    #[store_vector_direct]
    background_music: Vec<u8>,
}

#[async_trait]
impl Resource for AudioDatabaseResource {
    type ReturnValue = Vec<u8>;

    #[instrument]
    async fn compile(&self, _compiled: std::sync::Arc<AsyncCompiler>) -> Vec<u8> {
        let background_music_filename = "file_example_oog_48.ogg";
        let mut file = tokio::fs::File::open(background_music_filename)
            .await
            .expect("Cannot find background music file");
        let mut contents = Vec::new();
        file.read_to_end(&mut contents)
            .await
            .expect("cannot read file to the end");

        let database = AudioDatabase {
            background_music: contents,
        };

        database.serialize_root()
    }
}
