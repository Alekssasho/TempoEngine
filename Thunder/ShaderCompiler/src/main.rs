use data_definition_generated::ShaderType;
use regex::Regex;
use std::fs::File;
use std::io::prelude::*;
use std::path::PathBuf;
use structopt::StructOpt;

fn compile_hlsl_source(source: &str, shader_type: ShaderType) -> Option<Vec<u8>> {
    let (target_profile, entry_point) = match shader_type {
        ShaderType::Vertex => ("vs_6_0", "VertexShaderMain"),
        ShaderType::Pixel => ("ps_6_0", "PixelShaderMain"),
    };

    let result = hassle_rs::compile_hlsl("Shader", source, entry_point, target_profile, &[], &[]);

    match result {
        Ok(code) => Some(code),
        Err(err) => panic!("Error {}", err),
    }
}

fn find_all_hlsl_files(input_folder: &PathBuf) -> Result<Vec<PathBuf>, std::io::Error> {
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

    // Prepare regexes for checking shader type
    let shader_type_regex = [
        (
            Regex::new(r"(?m)\bVertexShaderMain\b").unwrap(),
            ShaderType::Vertex,
            "VS",
        ),
        (
            Regex::new(r"(?m)\bPixelShaderMain\b").unwrap(),
            ShaderType::Pixel,
            "PS",
        ),
    ];

    let input_hlsl_files =
        find_all_hlsl_files(&opt.input_folder).expect("Cannot read input folder for shaders");
    let input_hlsl_strings: Vec<(String, PathBuf)> = input_hlsl_files
        .into_iter()
        .map(|path| {
            (
                std::fs::read_to_string(&path).expect("File name not found"),
                path,
            )
        })
        .collect();

    let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
    let mut shaders_offsets = Vec::with_capacity(input_hlsl_strings.len() * 2);
    for (hlsl_string, hlsl_file) in input_hlsl_strings {
        for (regex, shader_type, shader_extension) in shader_type_regex.iter() {
            if regex.is_match(&hlsl_string) {
                let compiled_shader = compile_hlsl_source(&hlsl_string, *shader_type);

                let name = format!(
                    "{}-{}",
                    hlsl_file.file_stem().unwrap().to_str().unwrap(),
                    shader_extension
                );
                let name_offset = builder.create_string(&name);
                let code_offset = builder.create_vector(&compiled_shader.unwrap()[..]);
                let shader_offset = data_definition_generated::Shader::create(
                    &mut builder,
                    &data_definition_generated::ShaderArgs {
                        name: Some(name_offset),
                        type_: *shader_type,
                        code: Some(code_offset),
                    },
                );
                shaders_offsets.push((name, shader_offset));
            }
        }
    }
    // We are reading the vectors with binary searches from engine side, so we need a sort here.
    shaders_offsets.sort_unstable_by(|lhs, rhs| lhs.0.cmp(&rhs.0));
    let sorted_vector: Vec<flatbuffers::WIPOffset<data_definition_generated::Shader>> =
        shaders_offsets.into_iter().map(|x| x.1).collect();

    let shaders_vector_offset = builder.create_vector(&sorted_vector[..]);
    let root_shader_library = data_definition_generated::ShaderLibrary::create(
        &mut builder,
        &data_definition_generated::ShaderLibraryArgs {
            shaders: Some(shaders_vector_offset),
        },
    );
    builder.finish(
        root_shader_library,
        Some(data_definition_generated::SHADER_LIBRARY_IDENTIFIER),
    );
    let data = builder.finished_data();

    // Write the output data
    let mut output_file_path = opt.output_folder;
    output_file_path.push("ShaderLibrary");
    output_file_path.set_extension(data_definition_generated::SHADER_LIBRARY_EXTENSION);
    let mut output_file = File::create(output_file_path).expect("Cannot create output file");
    output_file
        .write_all(data)
        .expect("Cannot write output data");
}
