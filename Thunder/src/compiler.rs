use std::path::PathBuf;

pub struct ResourceCompiler {}

impl ResourceCompiler {
    pub fn compile(
        &mut self,
        registry: &super::resources::ResourceRegistry,
        output_folder: PathBuf,
    ) {
        let resources = registry.compile_dependancies();
        // TODO: This should be multithreaded
        for resource in resources {
            resource.compile();
        }
    }
}
