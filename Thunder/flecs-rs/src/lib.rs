#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/flecs-binding.rs"));

#[macro_use]
extern crate flecs_rs_derive;

use components::*;
use std::ffi::{c_void, CStr, CString};

fn cstr(input: &str) -> CString {
    CString::new(input).expect("CString::new failed")
}

pub struct FlecsState {
    world: *mut ecs_world_t,
    component_entities: Vec<(ecs_entity_t, u64)>,
    tag_entities: Vec<ecs_entity_t>,
}

impl Drop for FlecsState {
    fn drop(&mut self) {
        unsafe {
            ecs_fini(self.world);
        }
    }
}

fn register_tag(world: *mut ecs_world_t, name: &[u8]) -> ecs_entity_t {
    unsafe {
        ecs_new_entity(
            world,
            0,
            CStr::from_bytes_with_nul(name).unwrap().as_ptr(),
            std::ptr::null(),
        )
    }
}

fn register_component<T>(world: *mut ecs_world_t, name: &[u8]) -> (ecs_entity_t, u64) {
    unsafe {
        (
            ecs_new_component(
                world,
                0,
                CStr::from_bytes_with_nul(name).unwrap().as_ptr(),
                std::mem::size_of::<T>() as u64,
                std::mem::align_of::<T>() as u64,
            ),
            std::mem::size_of::<T>() as u64,
        )
    }
}

#[derive(Components)]
pub enum Components {
    Transform(Tempest_Components_Transform),
    Rect(Tempest_Components_Rect),
    StaticMesh(Tempest_Components_StaticMesh),
    DynamicPhysicsActor(Tempest_Components_DynamicPhysicsActor),
    LightColorInfo(Tempest_Components_LightColorInfo),
    CarPhysicsPart(Tempest_Components_CarPhysicsPart),
}

#[derive(Tags)]
pub enum Tags {
    Boids,
    DirectionalLight,
}

impl FlecsState {
    fn get_component_entity(&self, component: &Components) -> (ecs_entity_t, u64) {
        self.component_entities[component.get_index()]
    }

    fn get_tag_entity(&self, tag: &Tags) -> ecs_entity_t {
        self.tag_entities[tag.get_index()]
    }

    pub fn new() -> Self {
        unsafe {
            let world = ecs_init();
            // Register all components / tags
            let component_entities = Components::register_components(world);
            let tag_entities = Tags::register_tags(world);

            FlecsState {
                world,
                component_entities,
                tag_entities,
            }
        }
    }

    pub fn create_entity(
        &self,
        name: &str,
        components: &[Components],
        tags: &[Tags],
    ) -> ecs_entity_t {
        unsafe {
            let name = cstr(name);
            let sep = cstr(".");
            let entity = ecs_new_w_type(self.world, std::ptr::null());

            // Set name
            ecs_add_path_w_sep(
                self.world,
                entity,
                0,
                name.as_ptr(),
                sep.as_ptr(),
                std::ptr::null(),
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

            for tag in tags {
                ecs_add_entity(self.world, entity, self.get_tag_entity(&tag));
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
