#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/flecs-binding.rs"));

use components::{Tempest_Components_Transform, Tempest_Components_Transform_Name};
use std::ffi::{c_void, CString};
use std::fs::File;
use std::io::prelude::*;

fn cstr(input: &str) -> CString {
    CString::new(input).expect("CString::new failed")
}

pub unsafe fn test_flecs() {
    let world = ecs_init();
    let component_name = cstr(&std::str::from_utf8(Tempest_Components_Transform_Name).unwrap());
    let component_entity = ecs_new_component(
        world,
        0,
        component_name.as_ptr(),
        std::mem::size_of::<Tempest_Components_Transform>() as u64,
        std::mem::align_of::<Tempest_Components_Transform>() as u64,
    );

    let e1_name = cstr("E1");
    let e1 = ecs_new_entity(world, 0, e1_name.as_ptr(), component_name.as_ptr());
    let e1_data = Tempest_Components_Transform {
        Position: components::glm::vec3(1.0f32, 2.0f32, 3.0f32),
    };
    ecs_set_ptr_w_entity(
        world,
        e1,
        component_entity,
        std::mem::size_of::<Tempest_Components_Transform>() as u64,
        &e1_data as *const _ as *const c_void,
    );

    let e2_name = cstr("E2");
    let e2 = ecs_new_entity(world, 0, e2_name.as_ptr(), component_name.as_ptr());
    let e2_data = Tempest_Components_Transform {
        Position: components::glm::vec3(101.0f32, 105.2f32, 108.7f32),
    };
    ecs_set_ptr_w_entity(
        world,
        e2,
        component_entity,
        std::mem::size_of::<Tempest_Components_Transform>() as u64,
        &e2_data as *const _ as *const c_void,
    );

    let mut reader = ecs_reader_init(world);
    let mut file = File::create("flecs-test-save-file.flecs").unwrap();
    let mut buff = [0u8; 256usize];
    let mut read = 1;
    while read > 0 {
        read = ecs_reader_read(
            &mut buff as *mut _ as *mut ::std::os::raw::c_char,
            256u64,
            &mut reader,
        );
        file.write(&buff).expect("Cannot write into file");
    }
    file.flush().expect("Cannot flush");
}
