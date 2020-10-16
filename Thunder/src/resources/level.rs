use super::*;
use data_definition_generated::{Level, LevelArgs, LEVEL_IDENTIFIER};
use flecs_rs::*;
use std::{ffi::CString, io::Write, path::PathBuf};
pub struct LevelResource {
    pub name: String,
    pub entities: ResourceId,
    // TODO: Remove me, when we start reading this from a file
    pub prepare_entitites: fn() -> EntitiesWorldResource,
}

impl LevelResource {
    pub fn new(name: String, prepare_entitites: fn() -> EntitiesWorldResource) -> Self {
        Self {
            name,
            entities: INVALID_RESOURCE,
            prepare_entitites,
        }
    }
}

impl Resource for LevelResource {
    fn extract_dependencies(
        &mut self,
        compiler: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        self.entities = compiler.get_next_resource_id();
        let entities_resource_data = (self.prepare_entitites)();
        vec![(self.entities, Some(Box::new(entities_resource_data)))]
    }

    fn compile(&self, compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
        let level_name_offset = builder.create_string(&self.name);
        //let geometry_database_name_offset = builder.create_string(&geometry_database_name);
        let geometry_database_name_offset = builder.create_string(&"");
        let entities_data = compiled_dependencies.get_resource_data(self.entities);
        let entities_vector_offset = builder.create_vector(&entities_data[..]);
        let root_level = Level::create(
            &mut builder,
            &LevelArgs {
                name: Some(level_name_offset),
                entities: Some(entities_vector_offset),
                geometry_database_file: Some(geometry_database_name_offset),
            },
        );
        builder.finish(root_level, Some(LEVEL_IDENTIFIER));

        Vec::from(builder.finished_data())
    }
}

pub struct EntitiesWorldResource {
    pub flecs_state: flecs_rs::FlecsState,
    pub entities_names: Vec<(ecs_entity_t, CString)>,
}

impl Resource for EntitiesWorldResource {
    fn extract_dependencies(
        &mut self,
        _registry: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        // No dependancies for this resource
        vec![]
    }

    fn compile(&self, _compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut binary_data = Vec::<u8>::new();
        self.flecs_state.write_to_buffer(&mut binary_data);
        binary_data
    }
}

pub fn write_resource_to_file(data_to_write: &[u8], path: PathBuf) {
    let mut output_file = std::fs::File::create(path).expect("Cannot create output file");
    output_file
        .write_all(data_to_write)
        .expect("Cannot write output data");
}
