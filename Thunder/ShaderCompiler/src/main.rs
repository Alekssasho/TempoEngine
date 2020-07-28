use std::fs::File;
use std::io::prelude::*;
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

    let result = hassle_rs::compile_hlsl(
        source_name,
        &shader_text,
        entry_point,
        target_profile,
        &[],
        &[],
    );
    match result {
        Ok(code) => Some(code),
        Err(err) => panic!("Error {}", err),
    }
}

#[allow(dead_code)]
fn write_bytecode_to_header(source: Vec<u8>, output_file: PathBuf, shader_type: ShaderType) {
    let mut file = File::create(output_file).expect("Cannot create output file!");
    let mut contents = String::with_capacity(1024);
    let variable_name = match shader_type {
        ShaderType::VertexShader => "g_VertexShaderMain[]",
        ShaderType::PixelShader => "g_PixelShaderMain[]",
    };
    contents.push_str("const unsigned char ");
    contents.push_str(variable_name);
    contents.push_str(" = {\n");
    for byte in source {
        contents.push_str(&byte.to_string());
        contents.push_str(",");
    }
    contents.pop(); // remove the last comma
    contents.push_str("\n};\n");
    file.write_all(contents.as_bytes())
        .expect("Cannot write into file");
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

    let mut shader_library_builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
    let mut shaders_vector = Vec::with_capacity(input_shaders.len() * 2);
    for shader in input_shaders {
        {
            let compiled_vertex_shader =
                compile_hlsl_source_file(shader.to_str().unwrap(), ShaderType::VertexShader);
            let name = format!("{}-VS", shader.file_stem().unwrap().to_str().unwrap());
            let vertex_shader_name = shader_library_builder.create_string(&name);
            let vertex_code = shader_library_builder.create_vector(&compiled_vertex_shader.unwrap()[..]);
            let vertex_shader = data_definition_generated::Shader::create(&mut shader_library_builder, &data_definition_generated::ShaderArgs{
                Name: Some(vertex_shader_name),
                Type: data_definition_generated::ShaderType::Vertex,
                Code: Some(vertex_code),
            });
            shaders_vector.push((name, vertex_shader));

            // let mut output_file = shader.clone();
            // output_file.set_file_name(format!("{}-VS", output_file.file_stem().unwrap().to_str().unwrap()));
            // output_file.set_extension("h");
            // println!("Outputing vertex shader code to {:?}", output_file);
            // write_bytecode_to_header(compiled_vertex_shader.unwrap(), output_file, ShaderType::VertexShader);
        }

        {
            let compiled_pixel_shader =
                compile_hlsl_source_file(shader.to_str().unwrap(), ShaderType::PixelShader);

            let name = format!("{}-PS", shader.file_stem().unwrap().to_str().unwrap());
            let pixel_shader_name = shader_library_builder.create_string(&name);
            let pixel_code = shader_library_builder.create_vector(&compiled_pixel_shader.unwrap()[..]);
            let pixel_shader = data_definition_generated::Shader::create(&mut shader_library_builder, &data_definition_generated::ShaderArgs{
                Name: Some(pixel_shader_name),
                Type: data_definition_generated::ShaderType::Pixel,
                Code: Some(pixel_code),
            });
            shaders_vector.push((name, pixel_shader));
            // let mut output_file = shader.clone();
            // output_file.set_file_name(format!("{}-PS", output_file.file_stem().unwrap().to_str().unwrap()));
            // output_file.set_extension("h");
            // println!("Outputing pixel shader code to {:?}", output_file);
            // write_bytecode_to_header(compiled_vertex_shader.unwrap(), output_file, ShaderType::PixelShader);
        }
    }

    shaders_vector.sort_unstable_by(|lhs, rhs| lhs.0.cmp(&rhs.0));
    let sorted_vector: Vec<flatbuffers::WIPOffset<data_definition_generated::Shader>> = shaders_vector.into_iter().map(|x| x.1).collect();

    let shaders = shader_library_builder.create_vector(&sorted_vector[..]);
    let root_shader_library = data_definition_generated::ShaderLibrary::create(&mut shader_library_builder, &data_definition_generated::ShaderLibraryArgs{
        Shaders: Some(shaders)
    });
    shader_library_builder.finish(root_shader_library, Some(data_definition_generated::SHADER_LIBRARY_IDENTIFIER));
    let data = shader_library_builder.finished_data();

    let mut output_file_path = opt.output_folder;
    output_file_path.push("ShaderLibrary");
    output_file_path.set_extension(data_definition_generated::SHADER_LIBRARY_EXTENSION);
    let mut output_file = File::create(output_file_path).expect("Cannot create output file");
    output_file.write_all(data).expect("Cannot write output data");
}
