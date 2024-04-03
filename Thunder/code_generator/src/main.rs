use structures::{Structure, Attribute};

use crate::cppgenerator::generate_cpp_code;

mod parser;
mod structures;
mod validator;
mod cppgenerator;

static TEST_STRING: &str = "
    struct Vector {
        x : f32;
        y: f32;
        z: f32;
    }

    [Component]
    struct TransformComponent {
        position: Vector;
    }
";

fn prepare_for_generation(structures: &mut Vec<Structure>) {
    for mut structure in structures {
        structure.is_component = structure.attributes.iter().find(|att| match att {
            Attribute::Component => true,
            _ => false,
        }).is_some();
    }
}

fn main() {
    let (_, mut structures) = parser::parse_input(TEST_STRING).unwrap();

    assert_eq!(2, structures.len(), "Size of parsed structures");
    assert_eq!(structures[0].name, "Vector");
    assert_eq!(structures[1].name, "TransformComponent");
    assert_eq!(structures[1].attributes.len(), 1);

    let result = structures.iter().all(|struct_to_test| {
        validator::verify_all_structs_are_existing(struct_to_test, &structures)
    });
    if result {
        println!("Code is valid");
    } else {
        println!("Error");
    }
    //println!("{:#?}", structures);

    prepare_for_generation(&mut structures);
    let cpp_code = generate_cpp_code(&structures);
    println!("{}", cpp_code);
}
