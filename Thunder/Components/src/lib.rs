#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/component_bindings.rs"));

pub type glm_vec3 = math::Vec3;
pub type glm_vec4 = math::Vec4;
pub type glm_mat4x4 = math::Mat4;
pub type glm_quat = math::Quat;