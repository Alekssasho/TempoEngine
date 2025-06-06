#pragma once

#include <Graphics/RendererTypes.h>

namespace Tempest
{
// Rect is rendered as
// x = (0,0,0), y = (0,0,0)
// width = (1,0,0), height = (0,0,1)
struct DebugRectData
{
	glm::mat4x4 Transform;
	glm::vec4 Color;
};

struct FrameData
{
	uint64_t FrameIndex;

	glm::mat4x4 ViewProjection;

	// TODO: This is not very good memory wise as it contains pointers. We need a single allocation and just suballocate from it.
	eastl::vector<DebugRectData> DebugRects;
	eastl::vector<DebugRectData> DebugCubes;
	// TODO: This should not be part of this. This class should only be a memory pool in which every feature writes arbitrary data.
	struct MeshData {
		MeshHandle Mesh;
		glm::mat4x4 Transform;
	};
    eastl::vector<MeshData> StaticMeshes;
    eastl::vector<MeshData> SkeletonMeshes;

	struct DirectionalLight
	{
		glm::vec3 Direction;
		glm::vec3 Color;
	};
	eastl::vector<DirectionalLight> DirectionalLights;
};
}
