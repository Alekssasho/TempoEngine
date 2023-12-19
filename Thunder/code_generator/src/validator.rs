use crate::structures::Structure;

fn is_fundamental_type(ttype: &str) -> bool {
    match ttype {
        "f32" => true,
        _ => false,
    }
}

pub fn verify_all_structs_are_existing(
    struct_to_test: &Structure,
    all_structures: &[Structure],
) -> bool {
    struct_to_test.members.iter().all(|member| {
        is_fundamental_type(&member.ttype)
            || (all_structures
                .into_iter()
                .find(|structure| structure.name == member.ttype))
            .is_some()
    })
}
