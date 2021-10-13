#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/flecs-binding.rs"));

#[macro_use]
extern crate flecs_rs_derive;

use components::*;

struct Archetype {
    name: String,
    num_components: usize,
    components_arrays: Vec<Vec<Components>>,
}

pub struct FlecsState {
    archetypes: Vec<Archetype>,
    archetype_num_entities_prefix_sum: Vec<usize>,
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

pub struct TemporaryEntityId {
    archetypeIndex: usize,
    entityIndex: usize,
}

impl FlecsState {
    pub fn new() -> Self {
        FlecsState {
            archetypes: Default::default(),
            archetype_num_entities_prefix_sum: Default::default(),
        }
    }

    pub fn create_entity(
        &mut self,
        _name: &str, // TODO: Find a way to add names
        components: Vec<Components>,
        tags: &[Tags],
    ) -> TemporaryEntityId {
        // TODO: for now we rely on the order of the components to be the same
        // Construct Type
        let archetype_name = components
            .iter()
            .map(|comp| comp.get_name())
            .chain(tags.iter().map(|tag| tag.get_name()))
            .collect::<Vec<&str>>()
            .join(",");

        let indexOfArchetype = {
            if let Some(index) = self
                .archetypes
                .iter()
                .position(|archetype| archetype.name == archetype_name)
            {
                index
            } else {
                let mut components_arrays = vec![];
                components_arrays.resize_with(components.len(), || vec![]);
                self.archetypes.push(Archetype {
                    name: archetype_name,
                    num_components: components.len() + tags.len(),
                    components_arrays,
                });
                self.archetypes.len() - 1
            }
        };

        let archetype = &mut self.archetypes[indexOfArchetype];
        for (component_data, component_array) in components
            .into_iter()
            .zip(archetype.components_arrays.iter_mut())
        {
            component_array.push(component_data);
        }

        TemporaryEntityId {
            archetypeIndex: indexOfArchetype,
            entityIndex: archetype.components_arrays.first().unwrap().len() - 1,
        }
    }

    pub fn finish_adding_entities(&mut self) {
        // This will check that we are have not called finish_adding_entities twice
        assert!(self.archetype_num_entities_prefix_sum.is_empty());

        self.archetype_num_entities_prefix_sum = self
            .archetypes
            .iter()
            .scan(0, |sum, archetype| {
                let old_sum = *sum;
                *sum += archetype.num_components;
                Some(old_sum)
            })
            .collect();
    }

    pub fn create_stable_entity_ids(&self, id: TemporaryEntityId) -> usize {
        // This will check that we are have called finish_adding_entities
        assert!(!self.archetype_num_entities_prefix_sum.is_empty());

        self.archetype_num_entities_prefix_sum[id.archetypeIndex] + id.entityIndex
    }

    pub fn write_to_buffer<W: std::io::Write>(&self, writer: &mut W) {
        // This will check that we are have called finish_adding_entities
        assert!(!self.archetype_num_entities_prefix_sum.is_empty());
        // Write number of archetypes
        writer
            .write_all(&u32::to_ne_bytes(self.archetypes.len() as u32))
            .unwrap();

        // Each archetype
        for archetype in self.archetypes.iter() {
            // Num entities
            writer
                .write_all(&u32::to_ne_bytes(
                    archetype.components_arrays.first().unwrap().len() as u32,
                ))
                .unwrap();
            // Number of components
            writer
                .write_all(&u32::to_ne_bytes(archetype.num_components as u32))
                .unwrap();

            // Now all the sizes of the components
            let mut component_sizes = archetype
                .components_arrays
                .iter()
                .map(|component_array| component_array.first().unwrap().get_pointer_and_size().1)
                .collect::<Vec<usize>>();
            // Fill with zeros for tags
            if component_sizes.len() < archetype.num_components {
                component_sizes.resize(archetype.num_components, 0);
            }
            // Write the component sizes
            for size in component_sizes {
                writer.write_all(&u32::to_ne_bytes(size as u32)).unwrap();
            }

            // Now follow up with name of the archetype and all the component arrays
            // In order to not read the name from C++ we can write its size before the string
            // and the use that to skip to the arrays
            writer
                .write_all(&u32::to_ne_bytes(
                    (archetype.name.len() + 1) as u32, // +1 for the terminating zero
                ))
                .unwrap();
            write!(writer, "{}\0", archetype.name).unwrap();
            for array in archetype.components_arrays.iter() {
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
