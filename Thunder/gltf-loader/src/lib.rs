use std::path::Path;
#[derive(Debug)]
pub struct Scene {
    document: gltf::Document,
    buffers: Vec<gltf::buffer::Data>,
    _images: Vec<gltf::image::Data>,
}

impl Scene {
    pub fn new(path: impl AsRef<Path>) -> Self {
        if let Ok((document, buffers, images)) = gltf::import(path) {
            Scene {
                document,
                buffers,
                _images: images,
            }
        } else {
            panic!("Cannot find scene file to load");
        }
    }

    pub fn gather_meshes(&self) -> Vec<Mesh> {
        let mut meshes = Vec::new();
        let mut node_stack = Vec::new();

        for scene in self.document.scenes() {
            for node in scene.nodes() {
                node_stack.push(node.index());
            }
        }

        while !node_stack.is_empty() {
            if let Some(node) = self.document.nodes().nth(*node_stack.first().unwrap()) {
                for child in node.children() {
                    node_stack.push(child.index());
                }

                if let Some(mesh) = node.mesh() {
                    meshes.push(mesh.index());
                }
            }
            node_stack.remove(0);
        }

        meshes
            .into_iter()
            .map(|index| Mesh {
                scene: self,
                mesh: self.document.meshes().nth(index).unwrap(),
            })
            .collect()
    }

    pub fn gather_root_nodes(&self, scene_index: usize) -> Vec<Node> {
        self.document
            .scenes()
            .nth(scene_index)
            .unwrap()
            .nodes()
            .map(|node| Node { _scene: self, node })
            .collect()
    }

    pub fn num_scenes(&self) -> usize {
        self.document.scenes().len()
    }

    pub fn get_node(&self, node_index: usize) -> Node {
        self.document
            .nodes()
            .nth(node_index)
            .map(|node| Node { _scene: self, node })
            .unwrap()
    }
}

pub struct Node<'a> {
    _scene: &'a Scene,
    node: gltf::Node<'a>,
}

pub struct TRS {
    pub translate: nalgebra_glm::Vec3,
    pub rotate: nalgebra_glm::Quat,
    pub scale: nalgebra_glm::Vec3,
}

impl TRS {
    pub fn new(mat: nalgebra_glm::Mat4x4) -> TRS {
        let to_tempest = nalgebra_glm::scale(&nalgebra_glm::identity(), &nalgebra_glm::vec3(1.0, 1.0, -1.0));
        let mat = to_tempest * mat;
        let mut matrix_iter = mat.iter();
        let transform = gltf::scene::Transform::Matrix {
            matrix: [
                [
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                ],
                [
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                ],
                [
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                ],
                [
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                    *matrix_iter.next().unwrap(),
                ],
            ],
        };

        let (translate, rotate, scale) = transform.decomposed();
        // GLTF uses right handed system, and we need to invert Z to align
        // properly with what Tempest is using
        TRS {
            translate: nalgebra_glm::vec3(translate[0], translate[1], translate[2]),
            rotate: nalgebra_glm::quat(rotate[0], rotate[1], rotate[2], rotate[3]),
            scale: nalgebra_glm::vec3(scale[0], scale[1], scale[2]),
        }
    }
}

pub struct PerspectiveCamera {
    pub yfov: f32,
    pub aspect_ratio: f32,
    pub znear: f32,
    pub zfar: f32,
}

impl<'a> Node<'a> {
    pub fn mesh_index(&self) -> Option<u32> {
        self.node.mesh().and_then(|mesh| Some(mesh.index() as u32))
    }

    pub fn name(&self) -> String {
        self.node
            .name()
            .map_or(Some(format!("Node-{}", self.node.index())), |str| {
                Some(String::from(str))
            })
            .unwrap()
    }

    pub fn children(&self) -> Vec<Node> {
        self.node
            .children()
            .map(|child| Node {
                _scene: self._scene,
                node: child,
            })
            .collect()
    }

    pub fn local_transform(&self) -> nalgebra_glm::Mat4x4 {
        let mat = self.node.transform().matrix();
        nalgebra_glm::mat4x4(
            mat[0][0], mat[1][0], mat[2][0], mat[3][0],
            mat[0][1], mat[1][1], mat[2][1], mat[3][1],
            mat[0][2], mat[1][2], mat[2][2], mat[3][2],
            mat[0][3], mat[1][3], mat[2][3], mat[3][3],
        )
    }

    pub fn camera(&self) -> Option<PerspectiveCamera> {
        if let Some(camera) = self.node.camera() {
            if let gltf::camera::Projection::Perspective(data) = camera.projection() {
                return Some(PerspectiveCamera {
                    yfov: data.yfov(),
                    aspect_ratio: data.aspect_ratio().unwrap_or(16.0 / 9.0),
                    znear: data.znear(),
                    zfar: data.zfar().unwrap_or(1000.0),
                });
            }
        }
        None
    }

    pub fn is_boids(&self) -> bool {
        self.node.boids().unwrap_or(false)
    }
}

pub struct Mesh<'a> {
    scene: &'a Scene,
    mesh: gltf::Mesh<'a>,
}

impl<'a> Mesh<'a> {
    pub fn index(&self) -> u32 {
        self.mesh.index() as u32
    }

    pub fn primitive_count(&self) -> usize {
        self.mesh.primitives().count()
    }

    pub fn indices_counts(&self) -> Vec<usize> {
        self.mesh
            .primitives()
            .map(|prim| prim.indices().unwrap().count())
            .collect()
    }

    pub fn position_counts(&self) -> Vec<usize> {
        self.mesh
            .primitives()
            .map(|prim| prim.get(&gltf::Semantic::Positions).unwrap().count())
            .collect()
    }

    pub fn indices(&self, prim_index: usize) -> Option<Vec<u32>> {
        let primitive = self.mesh.primitives().nth(prim_index).unwrap();
        let reader = primitive.reader(|buffer| Some(&self.scene.buffers[buffer.index()]));
        reader
            .read_indices()
            .and_then(|iter| Some(iter.into_u32().collect()))
    }

    pub fn positions(&self, prim_index: usize) -> Option<Vec<nalgebra_glm::Vec3>> {
        let primitive = self.mesh.primitives().nth(prim_index).unwrap();
        let reader = primitive.reader(|buffer| Some(&self.scene.buffers[buffer.index()]));
        reader.read_positions().and_then(|iter| {
            Some(
                iter.map(|vertex| nalgebra_glm::vec3(vertex[0], vertex[1], vertex[2]))
                    .collect(),
            )
        })
    }
}

fn _main_test() -> Result<(), gltf::Error> {
    let scene = Scene::new("Duck.gltf");

    let meshes = scene.gather_meshes();
    for mesh in &meshes {
        println!("{}", mesh.primitive_count());
        println!("indices count {:?}", mesh.indices_counts());
        println!("positions count {:?}", mesh.position_counts());

        if let Some(indices) = mesh.indices(0) {
            println!("indiecs = {}", indices.len());
        }
        if let Some(positions) = mesh.positions(0) {
            println!("positions = {}", positions.len());
        }
    }

    Ok(())
}
