#pragma once

#include <Graphics/RenderFeature.h>
#include <World/EntityQuery.h>

namespace Tempest
{
namespace GraphicsFeature
{
struct StaticMesh : RenderFeature
{
	// TODO: Better ID
	static const int64_t ID = 1;

	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData& data, RendererCommandList& commandList, const Renderer& renderer) override;
private:
	EntityQuery m_Query;
	PipelineStateHandle m_Handle;
};
}
}