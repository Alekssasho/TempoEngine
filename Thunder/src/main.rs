use components::*;
use data_definition_generated::{Level, LevelArgs, LEVEL_EXTENSION, LEVEL_IDENTIFIER};
use flecs_rs::*;
use std::io::prelude::*;
use std::path::PathBuf;
use structopt::StructOpt;
use std::ffi::CString;

#[optick_attr::profile]
fn create_entity(state: &FlecsState, name: &str, pos: glm_vec3, color: glm_vec4) -> (ecs_entity_t, CString) {
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

#[optick_attr::profile]
fn prepare_entities() -> (FlecsState, Vec<(ecs_entity_t, CString)>) {
    let flecs_state = FlecsState::new();
    let mut entity_names = Vec::new();
    // create_entity(
    //     &flecs_state,
    //     "Green Rect",
    //     glm::vec3(-1.0, -1.0, 0.0),
    //     glm::vec4(1.0, 1.0, 0.0, 1.0),
    // );
    // create_entity(
    //     &flecs_state,
    //     "Blue Rect",
    //     glm::vec3(0.9, 0.9, 0.0),
    //     glm::vec4(0.0, 0.0, 1.0, 1.0),
    // );
    for i in 0..50 {
        let name = format!("Rect {}", i);
        entity_names.push(create_entity(
            &flecs_state,
            &name,
            glm::vec3(-0.8 + ((i as f32) * 0.03), -1.0, 0.0),
            glm::vec4(0.0, 0.0, 1.0, 1.0),
        ));
    }

    (flecs_state, entity_names)
}

#[derive(StructOpt)]
struct CommandLineOptions {
    /// Output folder to put generated level
    #[structopt(short, long, parse(from_os_str))]
    output_folder: PathBuf,
}

#[optick_attr::capture("Thunder")]
fn main() {
    optick::event!();
    let opt = CommandLineOptions::from_args();
    let entities_world = prepare_entities();

    let mut entities = Vec::<u8>::new();
    entities_world.0.write_to_buffer(&mut entities);

    let name = "Rects";

    let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
    {
        optick::event!("Prepare flatbuffer for level");
        let level_name_offset = builder.create_string(name);
        let entities_vector_offset = builder.create_vector(&entities[..]);
        let root_level = Level::create(
            &mut builder,
            &LevelArgs {
                name: Some(level_name_offset),
                entities: Some(entities_vector_offset),
            },
        );
        builder.finish(root_level, Some(LEVEL_IDENTIFIER));
    }

    // Write the output data
    {
        optick::event!("Write to output");
        let data = builder.finished_data();
        let mut output_file_path = opt.output_folder;
        output_file_path.push(format!("Level_{}", name));
        output_file_path.set_extension(LEVEL_EXTENSION);
        let mut output_file =
            std::fs::File::create(output_file_path).expect("Cannot create output file");
        output_file
            .write_all(data)
            .expect("Cannot write output data");
    }
}
