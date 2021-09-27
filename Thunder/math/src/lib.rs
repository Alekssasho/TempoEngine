pub use glam::Mat4;
pub use glam::Quat;
pub use glam::Vec2;
pub use glam::Vec3;
pub use glam::Vec4;

pub use glam::mat4;
pub use glam::quat;
pub use glam::vec2;
pub use glam::vec3;
pub use glam::vec4;

pub struct TRS {
    pub translate: Vec3,
    pub rotate: Quat,
    pub scale: Vec3,
}

impl TRS {
    pub fn new(mat: &Mat4) -> TRS {
        let (scale, rotate, translate) = mat.to_scale_rotation_translation();
        // Tempest has LH and gltf uses RH, so we need to invert Z here.
        // First negate rotation angle, then negate z axis. as Quaternion stores in xyz sin(theta), sin(-theta) = - sin(theta). So we need to invert
        // xy and z, however we need to invert z once more, so it becomes positive again. in W we store cos(theta). cos(-theta) = cos(theta) so no need to change that
        TRS {
            translate: vec3(-translate.x, translate.y, translate.z),
            rotate: quat(rotate.x, -rotate.y, -rotate.z, rotate.w),
            scale,
        }
    }
}

pub fn identity_matrix() -> Mat4 {
    Mat4::IDENTITY
}
