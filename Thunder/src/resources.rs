use std::{fmt::Debug, path::PathBuf};

use super::compiler::AsyncCompiler;

use tokio::io::AsyncWriteExt;

mod entities_world;
mod geometry_database;
pub mod level;

#[async_trait]
pub trait Resource {
    async fn compile(&self, compiler: std::sync::Arc<AsyncCompiler>) -> Vec<u8>;
}

impl Debug for dyn Resource {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "dyn resource")
    }
}

#[derive(Hash, Eq, PartialEq, Copy, Clone, Debug)]
pub struct ResourceId(pub usize);

#[allow(dead_code)]
pub const INVALID_RESOURCE: ResourceId = ResourceId(0);

#[instrument]
pub async fn write_resource_to_file(data_to_write: &[u8], path: PathBuf) {
    let mut output_file = tokio::fs::File::create(path)
        .await
        .expect("Cannot crete output file");
    output_file
        .write_all(data_to_write)
        .await
        .expect("Cannot write data");
}
