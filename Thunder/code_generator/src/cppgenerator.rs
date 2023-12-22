use tera::Tera;

use crate::structures::{Structure, Attribute};

fn generate_cpp_structure(structure: &Structure, builder: &mut string_builder::Builder) {
    builder.append("struct ");
    builder.append(structure.name.as_str());
    builder.append(" {\n");
    for member in structure.members.iter() {
        builder.append("    ");
        builder.append(member.ttype.as_str());
        builder.append(" ");
        builder.append(member.name.as_str());
        builder.append(";\n");
    }
    if structure.attributes.iter().find(|att| match att {
        Attribute::Component => true,
        _ => false,
    }).is_some() {
        builder.append("    static constexpr const char* Name = \"");
        builder.append(structure.name.as_str());
        builder.append("\";\n")
    }
    builder.append("};\n");
}

pub fn generate_cpp_code(structures: &[Structure]) -> String {
    // let mut builder = string_builder::Builder::new(1024);

    // for structure in structures {
    //     generate_cpp_structure(structure, &mut builder);
    // }

    // builder.string().unwrap()


    let mut tera = Tera::default();
    tera.add_raw_template("cppcode", "
        {% for structure in structures -%}
        struct {{structure.name}} {
            {% for member in structure.members -%}
            {{member.ttype}} {{member.name}};
            {% endfor -%}
            {%- if structure is component %}
            static constexpr const char* Name = \"{{structure.name}}\";
            {%- endif  %}
        };
        {% endfor %}
    ").unwrap();

    tera.register_tester("component", |structure: Option<&tera::Value>, _args: &[tera::Value]| {
        if let Some(obj) = structure.unwrap().as_object() {
            if let Some(attributes) = obj["attributes"].as_array() {
                if attributes.iter().find(|att| att.is_string() && att.as_str().unwrap() == "Component").is_some() {
                    return Ok(true);
                }
            }
        }
        Ok(false)
    });

    let mut context = tera::Context::new();
    context.insert("structures", structures);

    tera.render("cppcode", &context).unwrap()
}