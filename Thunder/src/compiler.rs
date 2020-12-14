use super::resources::Resource;
use std::path::PathBuf;

#[derive(Clone, Debug)]
pub struct CompilerOptions {
    pub output_folder: PathBuf,
    pub input_folder: PathBuf,
}

#[derive(Debug)]
pub struct AsyncCompiler {
    pub options: CompilerOptions,
}

pub type ResourceBox = Box<dyn Resource>;

pub async fn compile_async(resource: ResourceBox, options: CompilerOptions) -> Vec<u8> {
    let async_compiler = std::sync::Arc::new(AsyncCompiler { options });

    resource.compile(async_compiler).await
}
