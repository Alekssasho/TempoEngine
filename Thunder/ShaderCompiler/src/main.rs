use data_definition_generated::flatbuffer_derive::{FlatbufferSerialize, FlatbufferSerializeRoot};
use data_definition_generated::ShaderType;
use hassle_rs::{utils::HassleError, Dxc, DxcIncludeHandler};
use regex::Regex;
use std::fs::File;
use std::io::prelude::*;
use std::path::PathBuf;
use structopt::StructOpt;

fn compile_with_include_handler(
    source_name: &str,
    shader_text: &str,
    entry_point: &str,
    target_profile: &str,
    args: &[&str],
    defines: &[(&str, Option<&str>)],
    include_handler: &IncludeHandler,
) -> Result<Vec<u8>, HassleError> {
    let dxc = Dxc::new()?;

    let compiler = dxc.create_compiler()?;
    let library = dxc.create_library()?;

    let blob = library
        .create_blob_with_encoding_from_str(shader_text)
        .map_err(HassleError::Win32Error)?;


    let result = compiler.compile(
        &blob,
        source_name,
        entry_point,
        target_profile,
        args,
        Some(Box::new(include_handler.clone())),
        defines,
    );
    match result {
        Err(result) => {
            let error_blob = result
                .0
                .get_error_buffer()
                .map_err(HassleError::Win32Error)?;
            Err(HassleError::CompileError(
                library.get_blob_as_string(&error_blob),
            ))
        }
        Ok(result) => {
            let result_blob = result.get_result().map_err(HassleError::Win32Error)?;

            Ok(result_blob.to_vec())
        }
    }
}

fn compile_hlsl_source(
    source: &str,
    shader_type: ShaderType,
    include_handler: &IncludeHandler,
    opt: &CommandLineOptions,
) -> Option<Vec<u8>> {
    let (target_profile, entry_point) = match shader_type {
        ShaderType::Vertex => ("vs_6_6", "VertexShaderMain"),
        ShaderType::Pixel => ("ps_6_6", "PixelShaderMain"),
        ShaderType::Mesh => ("ms_6_6", "MeshShaderMain"),
        ShaderType::Amplify => ("as_6_6", "AmplifyShaderMain"),
        ShaderType::Hull => ("hs_6_6", "HullShaderMain"),
        ShaderType::Domain => ("ds_6_6", "DomainShaderMain"),
        ShaderType::Compute => ("cs_6_6", "ComputeShaderMain"),
        ShaderType::Geometry => ("gs_6_6", "GeometryShaderMain"),
    };

    let result = compile_with_include_handler(
        "Shader",
        source,
        entry_point,
        target_profile,
        if opt.debug_shaders { &["-O0", "-Zi"] } else { &[] },
        &[],
        include_handler,
    );

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

    /// Compile Shaders in Debug Mode
    #[structopt(short, long)]
    debug_shaders: bool,
}

#[derive(Clone)]
struct IncludeHandler {
    shader_folder: PathBuf,
}

impl DxcIncludeHandler for IncludeHandler {
    fn load_source(&self, filename: String) -> Option<String> {
        let mut fullpath = self.shader_folder.clone();
        fullpath.push(filename);
        match std::fs::File::open(fullpath) {
            Ok(mut f) => {
                let mut content = String::new();
                f.read_to_string(&mut content).unwrap();
                Some(content)
            }
            Err(_) => None,
        }
    }
}
#[derive(FlatbufferSerialize)]
struct Shader {
    #[store_offset]
    name: String,
    type_: ShaderType,
    #[store_vector_direct]
    code: Vec<u8>,
}

#[derive(FlatbufferSerializeRoot)]
struct ShaderLibrary {
    #[store_vector_offsets]
    shaders: Vec<Shader>,
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
        (
            Regex::new(r"(?m)\bMeshShaderMain\b").unwrap(),
            ShaderType::Mesh,
            "MS",
        ),
        (
            Regex::new(r"(?m)\bAmplifyShaderMain\b").unwrap(),
            ShaderType::Amplify,
            "AS",
        ),
        (
            Regex::new(r"(?m)\bHullShaderMain\b").unwrap(),
            ShaderType::Hull,
            "HS",
        ),
        (
            Regex::new(r"(?m)\bDomainShaderMain\b").unwrap(),
            ShaderType::Domain,
            "DS",
        ),
        (
            Regex::new(r"(?m)\bComputeShaderMain\b").unwrap(),
            ShaderType::Compute,
            "CS",
        ),
        (
            Regex::new(r"(?m)\bGeometryShaderMain\b").unwrap(),
            ShaderType::Geometry,
            "GS",
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

    let include_handler = IncludeHandler {
        shader_folder: opt.input_folder.clone(),
    };

    let mut shaders = Vec::with_capacity(input_hlsl_strings.len() * 2);
    for (hlsl_string, hlsl_file) in input_hlsl_strings {
        for (regex, shader_type, shader_extension) in shader_type_regex.iter() {
            if regex.is_match(&hlsl_string) {
                let compiled_shader =
                    compile_hlsl_source(&hlsl_string, *shader_type, &include_handler, &opt);

                let name = format!(
                    "{}-{}",
                    hlsl_file.file_stem().unwrap().to_str().unwrap(),
                    shader_extension
                );

                shaders.push(Shader {
                    name,
                    type_: *shader_type,
                    code: compiled_shader.unwrap(),
                });
            }
        }
    }
    // We are reading the vectors with binary searches from engine side, so we need a sort here.
    shaders.sort_unstable_by(|lhs, rhs| lhs.name.cmp(&rhs.name));

    let shader_library = ShaderLibrary { shaders };
    let data = shader_library.serialize_root();

    // Write the output data
    let mut output_file_path = opt.output_folder;
    output_file_path.push("ShaderLibrary");
    output_file_path.set_extension(data_definition_generated::SHADER_LIBRARY_EXTENSION);
    let mut output_file = File::create(output_file_path).expect("Cannot create output file");
    output_file
        .write_all(data.as_slice())
        .expect("Cannot write output data");
}
