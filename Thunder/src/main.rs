use components::*;
use data_definition_generated::LEVEL_EXTENSION;
use flecs_rs::*;
use std::ffi::CString;
use std::path::PathBuf;
use structopt::StructOpt;

mod compiler;
mod resources;
use resources::{level::write_resource_to_file, Resource};

fn create_level(name: &str) -> Box<dyn Resource> {
    let level = resources::level::LevelResource::new(name.to_string(), prepare_entities);
    Box::new(level)
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

#[allow(dead_code)]
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
fn prepare_entities() -> resources::level::EntitiesWorldResource {
    let flecs_state = FlecsState::new();
    let mut entity_names = Vec::new();
    // //Old boids example
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

    resources::level::EntitiesWorldResource {
        flecs_state,
        entities_names: entity_names,
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

    let name = "Rects";
    // TODO: Read this level from some source file
    let level = create_level(name);
    let compiled_data = compiler::compile(
        level,
        compiler::CompilerOptions {
            output_folder: opt.output_folder.clone(),
        },
    );

    let mut output_file_path = opt.output_folder.clone();
    output_file_path.push(format!("Level_{}", name));
    output_file_path.set_extension(LEVEL_EXTENSION);
    write_resource_to_file(compiled_data.as_slice(), output_file_path);
}
