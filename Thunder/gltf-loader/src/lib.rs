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

    pub fn gather_nodes(&self) -> Vec<Node> {
        self.document
            .nodes()
            .map(|node| Node { _scene: self, node })
            .collect()
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

    pub fn transform(&self) -> TRS {
        let (translate, rotate, scale) = self.node.transform().decomposed();
        // GLTF uses right handed system, and we need to invert Z to align
        // properly with what Tempest is using
        TRS {
            translate: nalgebra_glm::vec3(translate[0], translate[1], -translate[2]),
            rotate: nalgebra_glm::quat(rotate[3], rotate[0], rotate[1], rotate[2]),
            scale: nalgebra_glm::vec3(scale[0], scale[1], scale[2]),
        }
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
                // GLTF uses right handed system, and we need to invert Y to align, This is due to the way Blender GLTF exporter is written
                // properly with what Tempest is using
                iter.map(|vertex| nalgebra_glm::vec3(vertex[0], -vertex[1], vertex[2]))
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
