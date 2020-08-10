#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/component_bindings.rs"));

use nalgebra_glm as glm;
type glm_vec3 = glm::Vec3;

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test() {
        let component = Tempest_Components_Transform{ Position: glm::vec3(0.0f32, 0.0f32, 0.0f32) };
        assert_eq!(0.0f32, component.Position.x);
        assert_eq!(12, std::mem::size_of::<glm_vec3>());
    }
}