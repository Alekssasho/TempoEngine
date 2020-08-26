#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/flecs-binding.rs"));

use components::*;
use std::ffi::{c_void, CStr, CString};

fn cstr(input: &str) -> CString {
    CString::new(input).expect("CString::new failed")
}

pub struct FlecsState {
    world: *mut ecs_world_t,
    component_entities: Vec<(ecs_entity_t, u64)>,
}

impl Drop for FlecsState {
    fn drop(&mut self) {
        unsafe {
            ecs_fini(self.world);
        }
    }
}

unsafe fn register_component_func<T>(world: *mut ecs_world_t, name: &[u8]) -> ecs_entity_t {
    ecs_new_component(
        world,
        0,
        CStr::from_bytes_with_nul(name).unwrap().as_ptr(),
        std::mem::size_of::<T>() as u64,
        std::mem::align_of::<T>() as u64,
    )
}

macro_rules! register_component {
    ($world:ident, $component_type:ty, $component_name:ident) => {
        (
            register_component_func::<$component_type>($world, $component_name),
            std::mem::size_of::<$component_type>() as u64,
        )
    };
}

pub enum Components {
    Transform(Tempest_Components_Transform),
    Rect(Tempest_Components_Rect),
}

impl Components {
    fn get_pointer(&self) -> *const c_void {
        match self {
            Components::Transform(d) => d as *const _ as *const c_void,
            Components::Rect(d) => d as *const _ as *const c_void,
        }
    }
}

fn get_component_name(component: &Components) -> String {
    match component {
        Components::Transform(_) => CStr::from_bytes_with_nul(Tempest_Components_Transform_Name)
            .unwrap()
            .to_string_lossy()
            .into_owned(),
        Components::Rect(_) => CStr::from_bytes_with_nul(Tempest_Components_Rect_Name)
            .unwrap()
            .to_string_lossy()
            .into_owned(),
    }
}

impl FlecsState {
    fn get_component_entity(&self, component: &Components) -> (ecs_entity_t, u64) {
        match component {
            Components::Transform(_) => self.component_entities[0],
            Components::Rect(_) => self.component_entities[1],
        }
    }

    pub fn new() -> Self {
        unsafe {
            let world = ecs_init();
            let mut component_entities = Vec::new();
            // Register all components
            component_entities.push(register_component!(
                world,
                Tempest_Components_Transform,
                Tempest_Components_Transform_Name
            ));
            component_entities.push(register_component!(
                world,
                Tempest_Components_Rect,
                Tempest_Components_Rect_Name
            ));

            FlecsState {
                world,
                component_entities,
            }
        }
    }

    pub fn create_entity(&self, name: &str, components: &[Components]) -> ecs_entity_t {
        unsafe {
            let mut component_signature = String::new();
            for component in components {
                component_signature.push_str(&get_component_name(&component));
                component_signature.push(',')
            }
            component_signature.pop();

            let name = cstr(name);
            let component_signature_cstr = cstr(&component_signature);
            let entity = ecs_new_entity(
                self.world,
                0,
                name.as_ptr(),
                component_signature_cstr.as_ptr(),
            );

            for component in components {
                let (component_entity, size) = self.get_component_entity(&component);
                ecs_set_ptr_w_entity(
                    self.world,
                    entity,
                    component_entity,
                    size,
                    component.get_pointer(),
                );
            }

            entity
        }
    }

    pub fn write_to_buffer<W: std::io::Write>(&self, writer: &mut W) {
        unsafe {
            let mut reader = ecs_reader_init(self.world);
            const buffer_size: i32 = 256;
            let mut buff = [0u8; buffer_size as usize];
            let mut read = 1;
            while read > 0 {
                read = ecs_reader_read(
                    &mut buff as *mut _ as *mut ::std::os::raw::c_char,
                    buffer_size,
                    &mut reader,
                );
                writer.write_all(&buff).expect("Cannot write into writer");
            }
            writer.flush().expect("Cannot flush");
        }
    }
}
