pub use glam::Mat4;
pub use glam::Quat;
pub use glam::Vec2;
pub use glam::Vec3;
pub use glam::Vec4;

pub use glam::mat4;
pub use glam::quat;
pub use glam::vec2;
pub use glam::vec3;

pub struct TRS {
    pub translate: Vec3,
    pub rotate: Quat,
    pub scale: Vec3,
}

impl TRS {
    pub fn new(mat: &Mat4) -> TRS {
        // Tempest has LH and gltf uses RH, so we need to invert Z here.
        // let to_tempest = nalgebra_glm::scale(
        //     &nalgebra_glm::identity(),
        //     &nalgebra_glm::vec3(1.0, 1.0, -1.0),
        // );
        // let mat = to_tempest * mat;

        let (scale, rotate, translate) = mat.to_scale_rotation_translation();

        TRS {
            translate,
            rotate,
            scale,
        }
    }
}

pub fn identity_matrix() -> Mat4 {
    Mat4::IDENTITY
}
