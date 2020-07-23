extern crate hassle_rs;
use std::path::PathBuf;
use structopt::StructOpt;

enum ShaderType {
    VertexShader,
    PixelShader,
}

fn compile_hlsl_source_file(source_name: &str, shader_type: ShaderType) -> Option<Vec<u8>> {
    let shader_text = std::fs::read_to_string(source_name).expect("File name not found");

    let (target_profile, entry_point) = match shader_type {
        ShaderType::VertexShader => ("vs_6_0", "VertexShaderMain"),
        ShaderType::PixelShader => ("ps_6_0", "PixelShaderMain"),
    };

    if let Ok(code) = hassle_rs::compile_hlsl(source_name, &shader_text, entry_point, target_profile, &[], &[]) {
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

fn read_input_files(input_folder: &PathBuf) -> Result<Vec<PathBuf>, std::io::Error> {
    let mut result = Vec::new();
    for entry in std::fs::read_dir(input_folder)? {
        let entry = entry?;
        let path = entry.path();
        match path.extension() {
            Some(ext) if ext == "hlsl" => result.push(path),
            _ => (),
        }
    }
    Ok(result)
}

fn main() {
    let opt = CommandLineOptions::from_args();

    let input_shaders =
        read_input_files(&opt.input_folder).expect("Cannot read input folder for shaders");

    for shader in input_shaders {
        let compiled_vertex_shader = compile_hlsl_source_file(shader.to_str().unwrap(), ShaderType::VertexShader);
        let compiled_pixel_shader = compile_hlsl_source_file(shader.to_str().unwrap(), ShaderType::PixelShader);
    }
}
