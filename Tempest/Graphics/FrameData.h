#pragma once

#include <Graphics/RendererTypes.h>

namespace Tempest
{
// TODO: Remove me
struct RectData
{
	float x;
	float y;
	float width;
	float height;
	glm::vec4 color;
};

struct FrameData
{
	uint64_t FrameIndex;

	glm::mat4x4 ViewProjection;

	// TODO: This is not very good memory wise as it contains pointers. We need a single allocation and just suballocate from it.
	eastl::vector<RectData> Rects;
	// TODO: This should not be part of this. This class should only be a memory pool in which every feature writes arbitrary data.
	struct StaticMeshData {
		MeshHandle Mesh;
		glm::mat4x4 Transform;
	};
	eastl::vector<StaticMeshData> StaticMeshes;

	struct DirectionalLight
	{
		glm::vec3 Direction;
		glm::vec3 Color;
	};
	eastl::vector<DirectionalLight> DirectionalLights;
};
}
