#pragma once

#define GLM_FORCE_LEFT_HANDED
//#define GLM_FORCE_SWIZZLE
// TODO: this will be better in debugging
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#ifdef GLM_FORCE_LEFT_HANDED
static const glm::vec3 sForwardDirection(0.0f, 0.0f, 1.0f);
#else
static const glm::vec3 sForwardDirection(0.0f, 0.0f, -1.0f);
#endif

static const glm::vec3 sUpDirection(0.0f, 1.0f, 0.0f);

static inline JPH::Vec3 ToJPH(glm::vec3 i)
{
    return JPH::Vec3(i.x, i.y, i.z);
}

static inline JPH::Quat ToJPH(glm::quat i)
{
    return JPH::Quat(i.x, i.y, i.z, i.w);
}

static inline glm::vec3 FromJPH(JPH::Vec3 i)
{
    return glm::vec3(i.GetX(), i.GetY(), i.GetZ());
}

static inline glm::quat FromJPH(JPH::Quat i)
{
    return glm::quat::wxyz(i.GetW(), i.GetX(), i.GetY(), i.GetZ());
}
