use std::{io::Write, path::PathBuf};

use super::compiler::CompiledResources;
use super::compiler::CompilerGraph;
use super::compiler::ResourceBox;

mod entities_world;
mod geometry_database;
pub mod level;

pub trait Resource {
    fn extract_dependencies(
        &mut self,
        compiler: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)>;

    fn compile(&self, compiled_dependencies: &CompiledResources) -> Vec<u8>;
}

#[derive(Hash, Eq, PartialEq, Copy, Clone)]
pub struct ResourceId(pub usize);

pub const INVALID_RESOURCE: ResourceId = ResourceId(0);

pub fn write_resource_to_file(data_to_write: &[u8], path: PathBuf) {
    let mut output_file = std::fs::File::create(path).expect("Cannot create output file");
    output_file
        .write_all(data_to_write)
        .expect("Cannot write output data");
}
