#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/component_bindings.rs"));

pub use nalgebra_glm as glm;
pub type glm_vec3 = glm::Vec3;
pub type glm_vec4 = glm::Vec4;
pub type glm_mat4x4 = glm::Mat4x4;
