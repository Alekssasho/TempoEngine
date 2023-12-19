mod parser;
mod structures;
mod validator;

static TEST_STRING: &str = "
    struct Vector {
        x : f32;
        y: f32;
        z: f32;
    }
    struct TransformComponent {
        position: Vector;
    }
";

fn main() {
    let (_, structures) = parser::parse_input(TEST_STRING).unwrap();
    let result = structures.iter().all(|struct_to_test| {
        validator::verify_all_structs_are_existing(struct_to_test, &structures)
    });
    if result {
        println!("Code is valid");
    } else {
        println!("Error");
    }
    //println!("{:#?}", structures);
}
