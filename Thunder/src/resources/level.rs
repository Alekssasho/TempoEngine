use super::*;
use crate::resources::entities_world::EntitiesWorldResource;
use crate::resources::geometry_database::GeometryDatabaseResource;

use data_definition_generated::{Level, LevelArgs, GEOMETRY_DATABASE_EXTENSION, LEVEL_IDENTIFIER};

use gltf_loader::Scene;
use std::rc::Rc;
pub struct LevelResource {
    name: String,
    entities: ResourceId,
    geometry_database_id: ResourceId,
    scene: Option<Rc<Scene>>,
}

impl LevelResource {
    pub fn new(name: String) -> Self {
        Self {
            name,
            entities: INVALID_RESOURCE,
            geometry_database_id: INVALID_RESOURCE,
            scene: None,
        }
    }
}

impl Resource for LevelResource {
    fn extract_dependencies<'a>(
        &'a mut self,
        compiler: &CompilerGraph,
    ) -> Vec<(ResourceId, Option<ResourceBox>)> {
        let mut level_scene_file_name = compiler.options.input_folder.clone();
        level_scene_file_name.push(&self.name);
        level_scene_file_name.set_extension("gltf");
        self.scene = Some(Rc::new(Scene::new(level_scene_file_name)));

        self.entities = compiler.get_next_resource_id();
        self.geometry_database_id = compiler.get_next_resource_id();

        let entities_resource_data =
            EntitiesWorldResource::new(Rc::downgrade(self.scene.as_ref().unwrap()));
        let geometry_database_data =
            GeometryDatabaseResource::new(Rc::downgrade(self.scene.as_ref().unwrap()));

        vec![
            (self.entities, Some(Box::new(entities_resource_data))),
            (
                self.geometry_database_id,
                Some(Box::new(geometry_database_data)),
            ),
        ]
    }

    fn compile(&self, compiled_dependencies: &CompiledResources) -> Vec<u8> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);

        let level_name_offset = builder.create_string(&self.name);

        let geometry_database_compiled_data =
            compiled_dependencies.get_resource_data(self.geometry_database_id);
        let mut output_file_path = compiled_dependencies.options.output_folder.clone();
        output_file_path.push(&self.name);
        output_file_path.set_extension(GEOMETRY_DATABASE_EXTENSION);
        write_resource_to_file(geometry_database_compiled_data, output_file_path);
        let geometry_database_name = format!("{}.{}", self.name, GEOMETRY_DATABASE_EXTENSION);
        let geometry_database_name_offset = builder.create_string(&geometry_database_name);

        let entities_data = compiled_dependencies.get_resource_data(self.entities);
        let entities_vector_offset = builder.create_vector(entities_data);

        let root_level = Level::create(
            &mut builder,
            &LevelArgs {
                name: Some(level_name_offset),
                entities: Some(entities_vector_offset),
                geometry_database_file: Some(geometry_database_name_offset),
            },
        );
        builder.finish(root_level, Some(LEVEL_IDENTIFIER));

        Vec::from(builder.finished_data())
    }
}
