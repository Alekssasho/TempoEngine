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

pub type ResourceBox<T> = Box<dyn Resource<ReturnValue = T>>;

pub async fn compile_async<T>(resource: ResourceBox<T>, options: CompilerOptions) -> T {
    let async_compiler = std::sync::Arc::new(AsyncCompiler { options });

    resource.compile(async_compiler).await
}
