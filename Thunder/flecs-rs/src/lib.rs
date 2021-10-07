#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/flecs-binding.rs"));

#[macro_use]
extern crate flecs_rs_derive;

use components::*;
use std::{
    collections::HashMap,
    ffi::{c_void, CStr, CString},
    sync::Arc,
};

fn cstr(input: &str) -> CString {
    CString::new(input).expect("CString::new failed")
}
pub struct FlecsState {
    world: *mut ecs_world_t,
    component_entities: Vec<(ecs_entity_t, u64)>,
    tag_entities: Vec<ecs_entity_t>,

    // Bulk API
    next_entity_id: u64,
    archetypes: HashMap<String, Vec<Vec<Components>>>,
}

impl Drop for FlecsState {
    fn drop(&mut self) {
        // unsafe {
        //     ecs_fini(self.world);
        // }
    }
}

fn register_tag(world: *mut ecs_world_t, name: &[u8]) -> ecs_entity_t {
    unsafe {
        // ecs_new_entity(
        //     world,
        //     0,
        //     CStr::from_bytes_with_nul(name).unwrap().as_ptr(),
        //     std::ptr::null(),
        // )
        0
    }
}

fn register_component<T>(world: *mut ecs_world_t, name: &[u8]) -> (ecs_entity_t, u64) {
    unsafe {
        (
            // ecs_new_component(
            //     world,
            //     0,
            //     CStr::from_bytes_with_nul(name).unwrap().as_ptr(),
            //     std::mem::size_of::<T>() as u64,
            //     std::mem::align_of::<T>() as u64,
            // ),
            1,
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
    CameraController(Tempest_Components_CameraController),
    VehicleController(Tempest_Components_VehicleController),
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
            //let world = ecs_init();
            let world = std::ptr::null_mut();
            // Register all components / tags
            let component_entities = Components::register_components(world);
            let tag_entities = Tags::register_tags(world);

            FlecsState {
                world,
                component_entities,
                tag_entities,

                next_entity_id: 0,
                archetypes: Default::default(),
            }
        }
    }

    pub fn create_entity(
        &mut self,
        name: &str,
        components: Vec<Components>,
        tags: &[Tags],
    ) -> ecs_entity_t {
        // TODO: for now we rely on the order of the components to be the same
        // Construct Type
        let archetype_name = components
            .iter()
            .map(|comp| comp.get_name())
            .chain(tags.iter().map(|tag| tag.get_name()))
            .collect::<Vec<&str>>()
            .join(",");

        if !self.archetypes.contains_key(&archetype_name) {
            let mut components_arrays = vec![];
            components_arrays.resize_with(components.len(), || vec![]);
            self.archetypes
                .insert(archetype_name.clone(), components_arrays);
        }

        let components_arrays = self.archetypes.get_mut(&archetype_name).unwrap();
        for (component_data, component_array) in
            components.into_iter().zip(components_arrays.iter_mut())
        {
            component_array.push(component_data);
        }

        let result = self.next_entity_id;
        self.next_entity_id = self.next_entity_id + 1;
        result
    }

    pub fn write_to_buffer<W: std::io::Write>(&self, writer: &mut W) {
        // Write number of archetypes
        writer.write_all(&u32::to_ne_bytes(self.archetypes.len() as u32)).unwrap();

        for (name, components_array) in self.archetypes.iter() {
            write!(writer, "{}\0", name).unwrap();
            // Number of components
            writer.write_all(&u32::to_ne_bytes(components_array.first().unwrap().len() as u32)).unwrap();
            for array in components_array {
                for component in array {
                    let (ptr, size) = component.get_pointer();
                    unsafe {
                        writer
                            .write_all(std::slice::from_raw_parts(ptr as *const u8, size))
                            .unwrap();
                    }
                }
            }
        }

        writer.flush().expect("Cannot flush");

        // unsafe {
        //     let mut reader = ecs_reader_init(self.world);
        //     const buffer_size: i32 = 256;
        //     let mut buff = [0u8; buffer_size as usize];
        //     let mut read = 1;
        //     while read > 0 {
        //         read = ecs_reader_read(
        //             &mut buff as *mut _ as *mut ::std::os::raw::c_char,
        //             buffer_size,
        //             &mut reader,
        //         );
        //         writer.write_all(&buff).expect("Cannot write into writer");
        //     }
        //     writer.flush().expect("Cannot flush");
        // }
    }
}
