use std::{collections::HashMap, fs, path::Path};

use tera::{from_value, to_value, Result, Tera, Value};

use crate::structures::{Member, MemberAttribute, Structure, StructureAttribute};

fn type_for_member(args: &HashMap<String, Value>) -> Result<Value> {
    let member = from_value::<Member>(args.get("member").unwrap().clone()).unwrap();

    let mut type_name = &member.ttype;
    for attrib in &member.attributes {
        if let MemberAttribute::StorageType(val) = attrib {
            type_name = &val;
            break;
        }
    }
    Ok(to_value(type_name).unwrap())
}

fn needs_reflection(value: Option<&Value>, _: &[Value]) -> Result<bool> {
    let structure = from_value::<Structure>(value.unwrap().clone()).unwrap();

    for attrib in &structure.attributes {
        match attrib {
            StructureAttribute::NoReflection => return Ok(false),
            _ => (),
        }
    }
    Ok(true)
}

fn is_inherit(value: Option<&Value>, _: &[Value]) -> Result<bool> {
    let structure = from_value::<Structure>(value.unwrap().clone()).unwrap();

    for attrib in &structure.attributes {
        match attrib {
            StructureAttribute::Inherit => return Ok(true),
            _ => (),
        }
    }
    Ok(false)
}

pub fn generate_cpp_code(structures: &[Structure]) {
    let mut tera = Tera::default();

    let generated_code_destination = Path::new("./Tempest/Generated/");
    if !generated_code_destination.exists() {
        fs::create_dir_all(generated_code_destination).unwrap();
    }

    let templates_pattern = "./Thunder/code_generator/templates/*.jinja";
    let mut templates_names = vec![];

    for entry in glob::glob(templates_pattern).expect("Failed to read templates") {
        match entry {
            Err(err) => println!("{:?}", err),
            Ok(template_path) => {
                let template_name = template_path
                    .file_stem()
                    .unwrap()
                    .to_string_lossy()
                    .into_owned();
                templates_names.push(template_name);
                tera.add_template_file(template_path, Some(&templates_names.last().unwrap()))
                    .unwrap();
            }
        }
    }

    tera.register_function("type_for_member", type_for_member);

    tera.register_tester("needs_reflection", needs_reflection);
    tera.register_tester("is_inherit", is_inherit);

    let mut context = tera::Context::new();
    context.insert("structures", structures);

    for template in &templates_names {
        let generated_code = tera.render(template, &context).unwrap();
        let file_name = generated_code_destination.join(template);
        println!("Generating code for {:?}", file_name);
        if file_name.exists() {
            let current_contents = fs::read_to_string(&file_name).unwrap();
            if current_contents == generated_code {
                println!("Skipping as the code is the same");
                continue;
            }
        }

        fs::write(file_name, generated_code).unwrap();
    }
}
