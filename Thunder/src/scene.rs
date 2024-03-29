use std::{collections::HashSet, path::PathBuf};

use itertools::Itertools;

#[derive(Debug)]
pub struct GltfData {
    document: gltf::Document,
    buffers: Vec<gltf::buffer::Data>,
    pub images: Vec<gltf::image::Data>,
}

#[derive(Debug)]
pub struct Scene {
    pub gltf: GltfData,

    pub camera: data_definition_generated::Camera,
    pub root_nodes: Vec<usize>,
    pub meshes: Vec<usize>,
    pub materials: Vec<usize>,
    pub car_node_indices: Vec<usize>,
}

// Constructor
impl Scene {
    pub fn new(filename: &PathBuf) -> Scene {
        match gltf::import(filename) {
            Ok((document, buffers, images)) => {
                let gltf = GltfData {
                    document,
                    buffers,
                    images,
                };
                // TODO: Support only a single scene inside the document
                let root_nodes = gltf.gather_root_node_indices(0);
                let camera = Scene::extract_camera_from_scene(&gltf, &root_nodes);
                let (meshes, materials, car_node_indices) =
                    gltf.gather_mesh_and_materials_and_car_indices(&root_nodes);

                Scene {
                    gltf,
                    camera,
                    root_nodes,
                    meshes,
                    materials,
                    car_node_indices,
                }
            }
            Err(error) => {
                panic!("Error loading file: {}", error);
            }
        }
    }
}

impl GltfData {
    fn gather_root_node_indices(&self, scene_index: usize) -> Vec<usize> {
        self.document
            .scenes()
            .nth(scene_index)
            .unwrap()
            .nodes()
            .map(|node| node.index())
            .collect()
    }

    pub fn gather_mesh_and_materials_and_car_indices(
        &self,
        root_nodes: &[usize],
    ) -> (Vec<usize>, Vec<usize>, Vec<usize>) {
        let mut meshes = HashSet::new();
        let mut materials = HashSet::new();
        let mut car_nodes = HashSet::new();
        let mut node_stack = Vec::new();

        node_stack.extend_from_slice(root_nodes);

        while !node_stack.is_empty() {
            if let Some(node) = self.document.nodes().nth(*node_stack.first().unwrap()) {
                for child in node.children() {
                    node_stack.push(child.index());
                }

                if let Some(mesh) = node.mesh() {
                    meshes.insert(mesh.index());
                    for primitive in mesh.primitives() {
                        if let Some(material_index) = primitive.material().index() {
                            materials.insert(material_index);
                        }
                    }
                }
                if let Some(tempest_extension) = node.tempest_extension() {
                    if tempest_extension.is_car.unwrap_or(false) {
                        car_nodes.insert(node.index());
                    }
                }
            }
            node_stack.remove(0);
        }

        (
            meshes.drain().collect(),
            materials.drain().sorted().collect(),
            car_nodes.drain().collect(),
        )
    }

    pub fn node_transform(&self, index: usize) -> math::Mat4 {
        let mat = self
            .document
            .nodes()
            .nth(index)
            .unwrap()
            .transform()
            .matrix();
        math::Mat4::from_cols_array_2d(&mat)
    }

    fn node(&self, index: usize) -> gltf::Node {
        self.document.nodes().nth(index).unwrap()
    }

    pub fn node_mesh_index(&self, index: usize) -> Option<usize> {
        self.node(index).mesh().and_then(|mesh| Some(mesh.index()))
    }

    pub fn node_name(&self, index: usize) -> &str {
        self.node(index)
            .name()
            .map_or(Some("Unnamed node"), |str| Some(str))
            .unwrap()
    }

    pub fn node_children(&self, index: usize) -> Vec<usize> {
        self.node(index)
            .children()
            .map(|child| child.index())
            .collect()
    }

    pub fn is_car(&self, index: usize) -> bool {
        self.node(index)
            .tempest_extension()
            .map_or(false, |extension| extension.is_car.unwrap_or(false))
    }

    // TODO: This is not efficient as it does a copy
    pub fn tempest_extension(
        &self,
        index: usize,
    ) -> gltf::json::extensions::scene::tempest_extension::TempestNodeExtension {
        self.node(index).tempest_extension().unwrap().clone()
    }

    pub fn light(&self, index: usize) -> Option<gltf::khr_lights_punctual::Light> {
        self.node(index).light()
    }

    pub fn material(&self, index: usize) -> gltf::Material {
        self.document.materials().nth(index).unwrap()
    }
}

// Mesh functions
impl GltfData {
    fn mesh(&self, index: usize) -> gltf::Mesh {
        self.document.meshes().nth(index).unwrap()
    }

    pub fn _mesh_indices_count_per_primitive(&self, index: usize) -> Vec<usize> {
        self.mesh(index)
            .primitives()
            .map(|prim| prim.indices().unwrap().count())
            .collect()
    }

    pub fn mesh_primitive_count(&self, index: usize) -> usize {
        self.mesh(index).primitives().count()
    }

    pub fn mesh_positions_count_per_primitive(&self, index: usize) -> Vec<usize> {
        self.mesh(index)
            .primitives()
            .map(|prim| prim.get(&gltf::Semantic::Positions).unwrap().count())
            .collect()
    }

    pub fn mesh_indices(&self, mesh_index: usize, prim_index: usize) -> Option<Vec<u32>> {
        let primitive = self.mesh(mesh_index).primitives().nth(prim_index).unwrap();
        let reader = primitive.reader(|buffer| Some(&self.buffers[buffer.index()]));
        reader
            .read_indices()
            .and_then(|iter| Some(iter.into_u32().collect()))
    }

    pub fn mesh_positions(&self, mesh_index: usize, prim_index: usize) -> Option<Vec<math::Vec3>> {
        let primitive = self.mesh(mesh_index).primitives().nth(prim_index).unwrap();
        let reader = primitive.reader(|buffer| Some(&self.buffers[buffer.index()]));
        reader.read_positions().and_then(|iter| {
            Some(
                iter.map(|vertex| math::vec3(vertex[0], vertex[1], vertex[2]))
                    .collect(),
            )
        })
    }

    pub fn mesh_normals(&self, mesh_index: usize, prim_index: usize) -> Option<Vec<math::Vec3>> {
        let primitive = self.mesh(mesh_index).primitives().nth(prim_index).unwrap();
        let reader = primitive.reader(|buffer| Some(&self.buffers[buffer.index()]));
        reader.read_normals().and_then(|iter| {
            Some(
                iter.map(|normal| math::vec3(normal[0], normal[1], normal[2]))
                    .collect(),
            )
        })
    }

    pub fn mesh_uvs(&self, mesh_index: usize, prim_index: usize) -> Option<Vec<math::Vec2>> {
        let primitive = self.mesh(mesh_index).primitives().nth(prim_index).unwrap();
        let reader = primitive.reader(|buffer| Some(&self.buffers[buffer.index()]));
        reader.read_tex_coords(0).and_then(|tex_coord_reader| {
            Some(match tex_coord_reader {
                gltf::mesh::util::ReadTexCoords::U8(iter) => iter
                    .map(|tex_coord| {
                        math::vec2(tex_coord[0] as f32 / 255.0, tex_coord[1] as f32 / 255.0)
                    })
                    .collect(),
                gltf::mesh::util::ReadTexCoords::U16(iter) => iter
                    .map(|tex_coord| {
                        math::vec2(tex_coord[0] as f32 / 65535.0, tex_coord[1] as f32 / 65535.0)
                    })
                    .collect(),
                gltf::mesh::util::ReadTexCoords::F32(iter) => iter
                    .map(|tex_coord| math::vec2(tex_coord[0], tex_coord[1]))
                    .collect(),
            })
        })
    }

    pub fn mesh_material_index(&self, mesh_index: usize, prim_index: usize) -> Option<usize> {
        let primitive = self.mesh(mesh_index).primitives().nth(prim_index).unwrap();
        primitive.material().index()
    }
}

// Static methods for scene walking
impl Scene {
    pub fn walk_nodes<T, F>(
        gltf: &GltfData,
        node_index: usize,
        parent_transform: &math::Mat4,
        walk_function: &mut F,
    ) -> Option<T>
    where
        F: FnMut(&GltfData, usize, &math::Mat4) -> Option<T>,
    {
        let world_transform = *parent_transform * gltf.node_transform(node_index);

        let return_value = walk_function(gltf, node_index, &world_transform);
        if return_value.is_some() {
            return return_value;
        }

        for child in gltf.node(node_index).children().map(|node| node.index()) {
            let data = Scene::walk_nodes(gltf, child, &world_transform, walk_function);
            if data.is_some() {
                return data;
            }
        }

        None
    }

    fn extract_camera_from_scene(
        gltf: &GltfData,
        root_nodes: &[usize],
    ) -> data_definition_generated::Camera {
        for node_index in root_nodes {
            if let Some((camera, transform)) = Scene::walk_nodes(
                gltf,
                *node_index,
                &math::identity_matrix(),
                &mut |gltf, node_index, world_transform| {
                    if let Some(camera) = gltf.node(node_index).camera() {
                        if let gltf::camera::Projection::Perspective(data) = camera.projection() {
                            return Some((
                                (
                                    data.yfov(),
                                    data.aspect_ratio().unwrap_or(16.0 / 9.0),
                                    data.znear(),
                                    data.zfar().unwrap_or(1000.0),
                                ),
                                *world_transform,
                            ));
                        }
                    }
                    None
                },
            ) {
                let trs = math::TRS::new(&transform);

                // TRS is in Tempest LH system, so we use Tempest oriented Up and Forward directions
                let rotated_up = trs.rotate * math::vec3(0.0, 1.0, 0.0);
                let rotated_forward = trs.rotate * math::vec3(0.0, 0.0, -1.0);

                return data_definition_generated::Camera::new(
                    camera.0,
                    camera.2,
                    camera.3,
                    camera.1,
                    &data_definition_generated::Vec3::new(
                        trs.translate.x,
                        trs.translate.y,
                        trs.translate.z,
                    ),
                    &data_definition_generated::Vec3::new(
                        rotated_forward.x,
                        rotated_forward.y,
                        rotated_forward.z,
                    ),
                    &data_definition_generated::Vec3::new(rotated_up.x, rotated_up.y, rotated_up.z),
                );
            }
        }

        // Default Camera
        data_definition_generated::Camera::new(
            1.0,
            0.1,
            1000.0,
            16.0 / 9.0,
            &data_definition_generated::Vec3::new(0.0, 0.0, 0.0),
            &data_definition_generated::Vec3::new(0.0, 0.0, 0.0),
            &data_definition_generated::Vec3::new(0.0, 0.0, 0.0),
        )
    }

    pub fn walk_root_nodes<T, F>(&self, mut walk_function: F) -> Option<T>
    where
        F: FnMut(&GltfData, usize, &math::Mat4) -> Option<T>,
    {
        for node in &self.root_nodes {
            let return_value = Scene::walk_nodes(
                &self.gltf,
                *node,
                &math::identity_matrix(),
                &mut walk_function,
            );
            if return_value.is_some() {
                return return_value;
            }
        }
        None
    }
}
