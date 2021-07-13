pub use nalgebra_glm::Mat4x4;
pub use nalgebra_glm::Quat;
pub use nalgebra_glm::Vec3;
pub use nalgebra_glm::Vec2;

pub use nalgebra_glm::mat4x4;
pub use nalgebra_glm::quat_cross_vec;
pub use nalgebra_glm::vec3;
pub use nalgebra_glm::vec2;

pub struct TRS {
    pub translate: nalgebra_glm::Vec3,
    pub rotate: nalgebra_glm::Quat,
    pub scale: nalgebra_glm::Vec3,
}

impl TRS {
    pub fn new(mat: &nalgebra_glm::Mat4x4) -> TRS {
        // Tempest has LH and gltf uses RH, so we need to invert Z here.
        // let to_tempest = nalgebra_glm::scale(
        //     &nalgebra_glm::identity(),
        //     &nalgebra_glm::vec3(1.0, 1.0, -1.0),
        // );
        // let mat = to_tempest * mat;
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

        TRS {
            translate: nalgebra_glm::vec3(translate[0], translate[1], translate[2]),
            rotate: nalgebra_glm::quat(rotate[0], rotate[1], rotate[2], rotate[3]),
            scale: nalgebra_glm::vec3(scale[0], scale[1], scale[2]),
        }
    }
}

pub fn identity_matrix() -> Mat4x4 {
    nalgebra_glm::identity()
}
