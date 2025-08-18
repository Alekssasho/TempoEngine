#pragma once

#include <Graphics/RenderFeature.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GraphicsFeature
{
struct StaticMesh : RenderFeature
{
	virtual const char* MarkerName() override { return "Static Mesh"; }
	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData& data, RendererCommandList& commandList, const RenderGraphBlackboard& blackboard) override;
private:
	flecs::query<Components::Transform, Components::StaticMesh> m_Query;
	PipelineStateHandle m_Handle;
	PipelineStateHandle m_ShadowHandle;
};
}
}