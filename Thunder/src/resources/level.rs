use std::hash::{Hash, Hasher};

#[derive(Hash)]
pub struct LevelResource {
    pub name: String,
}

impl super::Resource for LevelResource {
    fn get_id(&self) -> super::ResourceId {
        let mut hasher = std::collections::hash_map::DefaultHasher::new();
        self.hash(&mut hasher);
        super::ResourceId(hasher.finish())
    }

    fn fill_dependancies(&self, registry: &mut super::ResourceRegistry) {
        todo!()
    }

    fn compile(&self) {
        todo!()
    }
}
