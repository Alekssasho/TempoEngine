#pragma once

#include <Graphics/RendererTypes.h>

namespace Tempest
{
class Renderer;
class RenderGraphBlackboard;
struct RenderFeature
{
	virtual ~RenderFeature() {}

	virtual void Initialize(const World& world, Renderer& renderer) = 0;
	virtual void GatherData(const World& world, FrameData& frameData) = 0;
	virtual void GenerateCommands(const FrameData& data, RendererCommandList& commandList, const RenderGraphBlackboard& blackboard) = 0;
};
}