use super::*;
use data_definition_generated::{GEOMETRY_DATABASE_EXTENSION, GEOMETRY_DATABASE_IDENTIFIER, GeometryDatabase, GeometryDatabaseArgs, LEVEL_IDENTIFIER, Level, LevelArgs, MeshMapping, MeshMappingArgs};
use flecs_rs::*;
use std::{ffi::CString, io::Write, path::PathBuf};
pub struct LevelResource {
    name: String,
    entities: ResourceId,
    // TODO: Remove me, when we start reading this from a file
    prepare_entitites: fn() -> EntitiesWorldResource,
    geometry_database_id: ResourceId,
}

impl LevelResource {
    pub fn new(name: String, prepare_entitites: fn() -> EntitiesWorldResource) -> Self {
        Self {
            name,
            entities: INVALID_RESOURCE,
            prepare_entitites,
            geometry_database_id: INVALID_RESOURCE,
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

        self.geometry_database_id = compiler.get_next_resource_id();
        vec![
            (self.entities, Some(Box::new(entities_resource_data))),
            (
                self.geometry_database_id,
                Some(Box::new(GeometryDatabaseResource {})),
            ),
        ]
    }

    fn compile(&self, compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);

        let level_name_offset = builder.create_string(&self.name);

        let geometry_database_compiled_data = compiled_dependencies.get_resource_data(self.geometry_database_id);
        let mut output_file_path = compiled_dependencies.options.output_folder.clone();
        output_file_path.push(&self.name);
        output_file_path.set_extension(GEOMETRY_DATABASE_EXTENSION);
        write_resource_to_file(geometry_database_compiled_data.as_slice(), output_file_path);
        let geometry_database_name = format!("{}.{}", self.name, GEOMETRY_DATABASE_EXTENSION);
        let geometry_database_name_offset = builder.create_string(&geometry_database_name);

        let entities_data = compiled_dependencies.get_resource_data(self.entities);
        let entities_vector_offset = builder.create_vector(entities_data.as_slice());

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

struct GeometryDatabaseResource {}

impl Resource for GeometryDatabaseResource {
    fn extract_dependencies(
        &mut self,
        _compiler: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        vec![]
    }

    fn compile(&self, _compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
        let vertex_buffer = vec![
            0.0f32, -0.5f32, 0.0f32, 0.5f32, 0.0f32, 0.0f32, -0.5f32, 0.0f32, 0.0f32, 1.0f32,
            -0.5f32, 1.0f32, 0.5f32, 0.0f32, 1.0f32, -0.5f32, 0.0f32, 1.0f32,
        ];

        let vertex_buffer_bytes = unsafe { (&vertex_buffer[..].align_to::<u8>()).1 };

        let mappings = vec![
            MeshMapping::create(
                &mut builder,
                &MeshMappingArgs {
                    index: 0,
                    vertex_offset: 0,
                    vertex_count: 3,
                },
            ),
            MeshMapping::create(
                &mut builder,
                &MeshMappingArgs {
                    index: 1,
                    vertex_offset: 36,
                    vertex_count: 3,
                },
            ),
        ];

        let mappings_offset = builder.create_vector(&mappings[..]);

        let vertex_buffer_offset = builder.create_vector(vertex_buffer_bytes);
        let root_level = GeometryDatabase::create(
            &mut builder,
            &GeometryDatabaseArgs {
                vertex_buffer: Some(vertex_buffer_offset),
                mappings: Some(mappings_offset),
            },
        );
        builder.finish(root_level, Some(GEOMETRY_DATABASE_IDENTIFIER));
        Vec::from(builder.finished_data())
    }
}
