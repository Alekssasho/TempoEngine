use std::{fs::File, io::Read};

//use structures::*;

use crate::cppgenerator::generate_cpp_code;

mod cppgenerator;
mod parser;
mod structures;
//mod validator;

fn main() {
    std::env::set_current_dir(
        std::env::current_exe()
            .unwrap()
            .parent()
            .unwrap()
            .parent()
            .unwrap()
            .parent()
            .unwrap(),
    )
    .unwrap();

    let mut component_file = File::open("./DataSchemas/Components.txt").unwrap();
    let mut contents = String::new();
    component_file.read_to_string(&mut contents).unwrap();

    let (_, structures) = parser::parse_input(&contents).unwrap();
    generate_cpp_code(&structures);
}
