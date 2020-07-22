extern crate hassle_rs;
use std::path::PathBuf;
use structopt::StructOpt;

enum ShaderType {
    VertexShader,
    PixelShader
}

fn compile_hlsl_source_file(source_file: &str, shader_type: ShaderType) -> Option<Vec<u8>> {
    let source = std::fs::read_to_string(source_file).expect("File name not found");

    let type_string = match shader_type {
        ShaderType::VertexShader => "vs_6_0",
        ShaderType::PixelShader => "ps_6_0"
    };

    if let Ok(code) = hassle_rs::compile_hlsl(source_file, &source, "main", type_string, &[], &[]) {
        Some(code)
    } else {
        None
    }
}

#[derive(StructOpt)]
struct CommandLineOptions {
    /// Input folder to take shader files from
    #[structopt(short, long, parse(from_os_str))]
    input_folder: PathBuf,

    /// Output folder to put compiled shaders
    #[structopt(short, long, parse(from_os_str))]
    output_folder: PathBuf,
}

fn main() {
    let opt = CommandLineOptions::from_args();
}