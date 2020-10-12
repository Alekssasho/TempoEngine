use components::*;
use data_definition_generated::{
    GeometryDatabase, GeometryDatabaseArgs, Level, LevelArgs, MeshMapping, MeshMappingArgs,
    GEOMETRY_DATABASE_EXTENSION, GEOMETRY_DATABASE_IDENTIFIER, LEVEL_EXTENSION, LEVEL_IDENTIFIER,
};
use flecs_rs::*;
use std::ffi::CString;
use std::io::prelude::*;
use std::path::PathBuf;
use structopt::StructOpt;

mod compiler;
mod resources;
use resources::Resource;

fn create_level() -> Box<resources::level::LevelResource> {
    let mut result = resources::level::LevelResource {
        name: "Rects".to_string(),
    };

    Box::new(result)
}

#[optick_attr::profile]
fn create_entity(
    state: &FlecsState,
    name: &str,
    pos: glm_vec3,
    color: glm_vec4,
) -> (ecs_entity_t, CString) {
    let transform = Components::Transform(Tempest_Components_Transform {
        Position: pos,
        Heading: glm::vec3(1.0, 0.0, 0.0),
    });
    let rect = Components::Rect(Tempest_Components_Rect {
        width: 0.02f32,
        height: 0.02f32,
        color,
    });
    state.create_entity(name, &[transform, rect])
}

fn create_mesh_entity(
    state: &FlecsState,
    name: &str,
    pos: glm_vec3,
    mesh_index: u32,
) -> (ecs_entity_t, CString) {
    let transform = Components::Transform(Tempest_Components_Transform {
        Position: pos,
        Heading: glm::vec3(1.0, 0.0, 0.0),
    });
    let static_mesh = Components::StaticMesh(Tempest_Components_StaticMesh { Mesh: mesh_index });
    state.create_entity(name, &[transform, static_mesh])
}

#[optick_attr::profile]
fn prepare_entities() -> (FlecsState, Vec<(ecs_entity_t, CString)>) {
    let flecs_state = FlecsState::new();
    let mut entity_names = Vec::new();
    // Old boids example
    // for i in 0..50 {
    //     let name = format!("Rect {}", i);
    //     entity_names.push(create_entity(
    //         &flecs_state,
    //         &name,
    //         glm::vec3(-0.8 + ((i as f32) * 0.03), -1.0, 0.0),
    //         glm::vec4(0.0, 0.0, 1.0, 1.0),
    //     ));
    // }

    // Mesh example
    entity_names.push(create_mesh_entity(
        &flecs_state,
        "Mesh",
        glm::vec3(0.0, 0.0, 0.0),
        0,
    ));

    entity_names.push(create_mesh_entity(
        &flecs_state,
        "Mesh 2",
        glm::vec3(0.0, 0.0, 0.0),
        1,
    ));

    (flecs_state, entity_names)
}

fn geometry_database_export(filename: &str, opt: &CommandLineOptions) -> () {
    let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
    {
        optick::event!("Prepare flatbuffer for level");
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
    }

    // Write the output data
    {
        optick::event!("Write to geometry output");
        let data = builder.finished_data();
        let mut output_file_path = opt.output_folder.clone();
        output_file_path.push(filename);
        output_file_path.set_extension(GEOMETRY_DATABASE_EXTENSION);
        let mut output_file =
            std::fs::File::create(output_file_path).expect("Cannot create output file");
        output_file
            .write_all(data)
            .expect("Cannot write output data");
    }
}

#[derive(StructOpt)]
struct CommandLineOptions {
    /// Output folder to put generated level
    #[structopt(short, long, parse(from_os_str))]
    output_folder: PathBuf,
}

#[optick_attr::capture("Thunder")]
fn main() {
    let opt = CommandLineOptions::from_args();

    let mut registry = resources::ResourceRegistry::new();

    // TODO: Read this level from some source file
    let level = create_level();

    registry.add_resource(level.get_id(), level);

    let mut compiler = compiler::ResourceCompiler {};
    compiler.compile(&registry, opt.output_folder);

    // let entities_world = prepare_entities();

    // let mut entities = Vec::<u8>::new();
    // entities_world.0.write_to_buffer(&mut entities);

    // let name = "Rects";
    // let geometry_database_name = format!("{}.{}", name, GEOMETRY_DATABASE_EXTENSION);

    // let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
    // {
    //     optick::event!("Prepare flatbuffer for level");
    //     let level_name_offset = builder.create_string(name);
    //     let geometry_database_name_offset = builder.create_string(&geometry_database_name);
    //     let entities_vector_offset = builder.create_vector(&entities[..]);
    //     let root_level = Level::create(
    //         &mut builder,
    //         &LevelArgs {
    //             name: Some(level_name_offset),
    //             entities: Some(entities_vector_offset),
    //             geometry_database_file: Some(geometry_database_name_offset),
    //         },
    //     );
    //     builder.finish(root_level, Some(LEVEL_IDENTIFIER));
    // }

    // // Write the output data
    // {
    //     optick::event!("Write to output");
    //     let data = builder.finished_data();
    //     let mut output_file_path = opt.output_folder.clone();
    //     output_file_path.push(format!("Level_{}", name));
    //     output_file_path.set_extension(LEVEL_EXTENSION);
    //     let mut output_file =
    //         std::fs::File::create(output_file_path).expect("Cannot create output file");
    //     output_file
    //         .write_all(data)
    //         .expect("Cannot write output data");
    // }

    // geometry_database_export(name, &opt);
}
