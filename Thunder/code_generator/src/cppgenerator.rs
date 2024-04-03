use tera::Tera;

use crate::structures::Structure;

pub fn generate_cpp_code(structures: &[Structure]) -> String {
    let mut tera = Tera::default();
    tera.add_raw_template("cppcode_structure_definition", "
        struct {{structure.name}} {
            {% for member in structure.members -%}
            {{member.ttype}} {{member.name}};
            {% endfor -%}
            {%- if structure.is_component %}
            static constexpr const char* Name = \"{{structure.name}}\";
            {%- endif  %}
        };
    ").unwrap();
    tera.add_raw_template("cppcode_main_file", "
        {% for structure in structures -%}
            {% include \"cppcode_structure_definition\" %}
        {% endfor %}
    ").unwrap();

    let mut context = tera::Context::new();
    context.insert("structures", structures);

    tera.render("cppcode_main_file", &context).unwrap()
}
