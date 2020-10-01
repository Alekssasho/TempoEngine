#pragma once

#include <inttypes.h>
#include <EASTL/vector.h>
#include <Math/Math.h>

namespace Tempest
{
enum class RenderPhase : uint8_t
{
	Main
};

class World;
struct FrameData;
struct RendererCommandList;

using GatherDataFunc = void(*)(const World&, FrameData&);
using PrepareDataFunc = void(*)();
using GenerateCommandsFunc = void(*)(const FrameData&, RendererCommandList&);

struct GraphicsFeatureDescription
{
	int64_t Id;
	GatherDataFunc GatherData;
	PrepareDataFunc PrepareData;
	GenerateCommandsFunc GenerateCommands;
};

// TODO: Remove me
struct RectData
{
	float x;
	float y;
	float width;
	float height;
	glm::vec4 color;
};

using MeshHandle = uint32_t;

struct FrameData
{
	uint64_t FrameIndex;

	// TODO: This is not very good memory wise as it contains pointers. We need a single allocation and just suballocate from it.
	eastl::vector<RectData> Rects;
	// TODO: This should not be part of this. This class should only be a memory pool in which every feature writes arbirtrary data.
	struct StaticMeshData {
		MeshHandle Mesh;
		//glm::mat4x4 Transform;
	};
	eastl::vector<StaticMeshData> StaticMeshes;
};
}
