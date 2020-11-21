#pragma once

#ifndef BINDGEN
#include <Math/Math.h>
#else
namespace glm
{
struct vec4{};
struct mat4x4{};
}
#endif
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Components
{
struct Transform
{
	// TODO: Consider decomposed variant
	glm::mat4x4 Matrix;
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
}

namespace Tags
{
struct Boids
{
	static constexpr const char* Name = "Boids";
};
}
}
