#![cfg_attr(
    target_arch = "spirv",
    no_std,
    feature(register_attr),
    register_attr(spirv)
)]
#![deny(warnings)]

use core::f32::consts::PI;
use glam::{vec3, Mat4, Vec3};

use spirv_std::arch::{ddx_vector, ddy_vector};
// Note: This cfg is incorrect on its surface, it really should be "are we compiling with std", but
// we tie #[no_std] above to the same condition, so it's fine.
#[cfg(target_arch = "spirv")]
use spirv_std::num_traits::Float;

#[derive(Copy, Clone)]
#[repr(C)]
pub struct ShaderConstants {
    pub view_projection_matrix: Mat4,
}

pub fn saturate(x: f32) -> f32 {
    x.max(0.0).min(1.0)
}

pub fn pow(v: Vec3, power: f32) -> Vec3 {
    vec3(v.x.powf(power), v.y.powf(power), v.z.powf(power))
}

pub fn exp(v: Vec3) -> Vec3 {
    vec3(v.x.exp(), v.y.exp(), v.z.exp())
}

/// Based on: <https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/>
pub fn acos_approx(v: f32) -> f32 {
    let x = v.abs();
    let mut res = -0.155972 * x + 1.56467; // p(x)
    res *= (1.0f32 - x).sqrt();

    if v >= 0.0 {
        res
    } else {
        PI - res
    }
}

pub fn smoothstep(edge0: f32, edge1: f32, x: f32) -> f32 {
    // Scale, bias and saturate x to 0..1 range
    let x = saturate((x - edge0) / (edge1 - edge0));
    // Evaluate polynomial
    x * x * (3.0 - 2.0 * x)
}

#[cfg(not(target_arch = "spirv"))]
#[macro_use]
pub extern crate spirv_std_macros;
use glam::{vec4, Vec4};

#[spirv(vertex)]
pub fn main_vs(
    position: Vec3,
    #[spirv(push_constant)] constants: &ShaderConstants,
    #[spirv(position, invariant)] out_pos: &mut Vec4,
    #[spirv(invariant)] out_pos_world: &mut Vec3,
) {
    *out_pos_world = position;
    *out_pos = constants.view_projection_matrix * vec4(position.x, position.y, position.z, 1.0);
}

#[spirv(fragment)]
pub fn main_fs(in_world_position: Vec3, output: &mut Vec4) {
    let light_direction = vec3(-1.0, 1.0, -1.0);
    let world_position_ddx = ddx_vector(in_world_position);
    let world_position_ddy = ddy_vector(in_world_position);
    let normal = world_position_ddx.cross(world_position_ddy).normalize();

    let diffuse_factor = saturate(normal.dot(-light_direction));
    let ambient_factor = 0.15;
    let color = vec4(1.0, 1.0, 1.0, 1.0);

    *output = color * diffuse_factor + (color * ambient_factor);
}
