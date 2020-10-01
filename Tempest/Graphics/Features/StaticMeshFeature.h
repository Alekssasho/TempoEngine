#pragma once

#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace GraphicsFeature
{
struct StaticMesh
{
	// TODO: Better ID
	static const int64_t ID = 1;

	static void GatherData(const World&, FrameData&);
	static void GenerateCommands(const FrameData& data, RendererCommandList& commandList);

	static GraphicsFeatureDescription GetDescription();
};
}
}