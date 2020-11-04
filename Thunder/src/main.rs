use data_definition_generated::LEVEL_EXTENSION;
use std::path::PathBuf;
use structopt::StructOpt;

mod compiler;
mod resources;

use resources::level::{write_resource_to_file, LevelResource};

#[derive(StructOpt)]
struct CommandLineOptions {
    /// Output folder to put generated level
    #[structopt(short, long, parse(from_os_str))]
    output_folder: PathBuf,

    /// Input folder to read level from
    #[structopt(short, long, parse(from_os_str))]
    input_folder: PathBuf,

    #[structopt(short, long)]
    level_name: String,
}

#[optick_attr::capture("Thunder")]
fn main() {
    let opt = CommandLineOptions::from_args();

    let level = Box::new(LevelResource::new(opt.level_name.to_string()));
    let compiled_data = compiler::compile(
        level,
        compiler::CompilerOptions {
            output_folder: opt.output_folder.clone(),
            input_folder: opt.input_folder.clone(),
        },
    );

    let mut output_file_path = opt.output_folder.clone();
    output_file_path.push(format!("Level_{}", opt.level_name));
    output_file_path.set_extension(LEVEL_EXTENSION);
    write_resource_to_file(compiled_data.as_slice(), output_file_path);
}
