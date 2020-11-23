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

unsafe fn register_tag(world: *mut ecs_world_t, name: &[u8]) -> ecs_entity_t {
    ecs_new_entity(
        world,
        0,
        CStr::from_bytes_with_nul(name).unwrap().as_ptr(),
        std::ptr::null(),
    )
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
}

impl Components {
    fn register_components(world: *mut ecs_world_t) -> Vec<(ecs_entity_t, u64)> {
        let mut component_entities = Vec::new();
        // Register all components
        component_entities.push(register_component::<Tempest_Components_Transform>(
            world,
            Tempest_Components_Transform_Name,
        ));
        component_entities.push(register_component::<Tempest_Components_Rect>(
            world,
            Tempest_Components_Rect_Name,
        ));
        component_entities.push(register_component::<Tempest_Components_StaticMesh>(
            world,
            Tempest_Components_StaticMesh_Name,
        ));
        component_entities
    }
}

pub enum Tags {
    Boids,
}

impl FlecsState {
    fn get_component_entity(&self, component: &Components) -> (ecs_entity_t, u64) {
        self.component_entities[component.get_index()]
    }

    fn get_tag_entity(&self, tag: &Tags) -> ecs_entity_t {
        match tag {
            Tags::Boids => self.tag_entities[0],
        }
    }

    #[optick_attr::profile]
    pub fn new() -> Self {
        unsafe {
            let world = {
                optick::event!("ecs_init");
                ecs_init()
            };
            let mut tag_entities = Vec::new();
            // Register all components
            let component_entities = Components::register_components(world);

            // Register all tags
            tag_entities.push(register_tag(world, Tempest_Tags_Boids_Name));

            FlecsState {
                world,
                component_entities,
                tag_entities,
            }
        }
    }

    #[optick_attr::profile]
    pub fn create_entity(
        &self,
        name: &str,
        components: &[Components],
        tags: &[Tags],
    ) -> (ecs_entity_t, CString) {
        unsafe {
            let name = cstr(name);
            let entity = ecs_new_entity(self.world, 0, name.as_ptr(), std::ptr::null());

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

            (entity, name)
        }
    }

    #[optick_attr::profile]
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
