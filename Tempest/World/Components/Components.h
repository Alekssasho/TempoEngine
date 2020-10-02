#pragma once

#include <Math/Math.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Components
{
struct Transform
{
	// TODO: Change to transforms
	glm::vec3 Position;
	glm::vec3 Heading;
	static constexpr const char* Name = "Transform";
};

struct Rect
{
	float width;
	float height;
	glm::vec4 color;

	static constexpr const char* Name = "Rect";
};

struct StaticMesh
{
	MeshHandle Mesh;

	static constexpr const char* Name = "StaticMesh";
};

}
}