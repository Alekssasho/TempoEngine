#pragma once

#include <Graphics/RenderFeature.h>
#include <World/EntityQuery.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GraphicsFeature
{
struct Rects : RenderFeature
{
	// TODO: Better ID
	static const int64_t ID = 0;

	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData& data, RendererCommandList& commandList, const Renderer& renderer, RenderPhase phase) override;

private:
	EntityQuery<Components::Transform, Components::Rect> m_Query;
	PipelineStateHandle m_Handle;
};
}
}