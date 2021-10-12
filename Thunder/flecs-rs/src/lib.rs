#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/flecs-binding.rs"));

#[macro_use]
extern crate flecs_rs_derive;

use components::*;
use std::collections::HashMap;

pub struct FlecsState {
    // Bulk API
    next_entity_id: u64,
    archetypes: HashMap<String, Vec<Vec<Components>>>,
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
    pub fn new() -> Self {
        FlecsState {
            next_entity_id: 0,
            archetypes: Default::default(),
        }
    }

    pub fn create_entity(
        &mut self,
        _name: &str, // TODO: Find a way to add names
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
        writer
            .write_all(&u32::to_ne_bytes(self.archetypes.len() as u32))
            .unwrap();

        for (name, components_array) in self.archetypes.iter() {
            write!(writer, "{}\0", name).unwrap();
            // Number of components
            writer
                .write_all(&u32::to_ne_bytes(
                    components_array.first().unwrap().len() as u32
                ))
                .unwrap();
            for array in components_array {
                for component in array {
                    let (ptr, size) = component.get_pointer_and_size();
                    unsafe {
                        writer
                            .write_all(std::slice::from_raw_parts(ptr, size))
                            .unwrap();
                    }
                }
            }
        }

        writer.flush().expect("Cannot flush");
    }
}
