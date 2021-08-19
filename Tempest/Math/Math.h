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

#ifdef GLM_FORCE_LEFT_HANDED
static const glm::vec3 sForwardDirection(0.0f, 0.0f, 1.0f);
#else
static const glm::vec3 sForwardDirection(0.0f, 0.0f, -1.0f);
#endif

static const glm::vec3 sUpDirection(0.0f, 1.0f, 0.0f);
