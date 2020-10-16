use super::compiler::CompiledResources;
use super::compiler::CompilerGraph;
use super::compiler::ResourceBox;

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
