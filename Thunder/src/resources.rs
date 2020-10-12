use std::collections::HashMap;

pub mod level;

pub trait Resource {
    fn get_id(&self) -> ResourceId;

    fn fill_dependancies(&self, registry: &mut ResourceRegistry);

    // TODO: Add additional members when we know how we are going to compile stuff
    fn compile(&self);
}

#[derive(Hash, Eq, PartialEq, Copy, Clone)]
pub struct ResourceId(u64);

pub struct ResourceRegistry {
    resources: HashMap<ResourceId, Box<dyn Resource>>,
}

impl ResourceRegistry {
    pub fn new() -> Self {
        Self {
            resources: HashMap::new(),
        }
    }

    pub fn has_resource(&self, id: ResourceId) -> bool {
        self.resources.contains_key(&id)
    }

    pub fn add_resource(&mut self, id: ResourceId, resource: Box<dyn Resource>) {
        resource.fill_dependancies(self);
        self.resources.insert(id, resource);
    }

    // TODO: this should have a topological sort
    pub fn compile_dependancies(&self) -> Vec<&Box<dyn Resource>> {
        self.resources.values().collect::<Vec<&Box<dyn Resource>>>()
    }
}
