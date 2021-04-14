use std::error::Error;

fn build_shader(path_to_create: &str) -> Result<(), Box<dyn Error>> {
    spirv_builder::SpirvBuilder::new(path_to_create).build()?;
    Ok(())
}

fn main() -> Result<(), Box<dyn Error>> {
    build_shader("../shaders/mesh_shader")?;
    Ok(())
}