#pragma once

#include <Graphics/RendererTypes.h>
#include <World/Camera.h>

namespace physx
{
class PxRigidBody;
}

namespace Tempest
{
namespace CastleFight
{
enum class Faction : uint32_t
{
	Red,
	Blue,
	Count
};
}
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

struct CarPhysicsPart
{
	physx::PxRigidBody* CarActor;
	uint32_t ShapeIndex;

	static constexpr const char* Name = "CarPhysicsPart";
};

struct CameraController
{
	Camera CameraData;
	int InputMapIndex;

	static constexpr const char* Name = "Component::CameraController";
};

struct VehicleController
{
	// TODO: add input stuff
	int InputMapIndex;

	static constexpr const char* Name = "VehicleController";
};

struct Faction
{
	CastleFight::Faction FactionFlag;

	static constexpr const char* Name = "Faction";
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
}
}
