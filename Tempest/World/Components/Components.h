#pragma once

#include <Graphics/RendererTypes.h>

namespace physx
{
class PxRigidBody;
}

namespace Tempest
{
namespace Components
{
struct Transform
{
	glm::quat Rotation;
	glm::vec3 Position;
	glm::vec3 Scale;
	static constexpr const char* Name = "Transform";
};

struct Rect
{
	float width;
	float height;
	glm::vec4 color;

	static constexpr const char* Name = "Rect";
};

// Static mesh component for rendering
struct StaticMesh
{
	MeshHandle Mesh;

	static constexpr const char* Name = "StaticMesh";
};

struct DynamicPhysicsActor
{
	physx::PxRigidBody* Actor;

	static constexpr const char* Name = "DynamicPhysicsActor";
};

struct LightColorInfo
{
	glm::vec3 Color;
	float Intensity;

	static constexpr const char* Name = "LightColorInfo";
};
}

namespace Tags
{
struct Boids
{
	static constexpr const char* Name = "Boids";
};

struct DirectionalLight
{
	static constexpr const char* Name = "DirectionalLight";
};

struct CarChassis
{
	static constexpr const char* Name = "CarChassis";
};

struct CarWheel
{
	static constexpr const char* Name = "CarWheel";
};
}
}
