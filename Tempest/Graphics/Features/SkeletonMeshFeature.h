#pragma once

#include <Graphics/RenderFeature.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GraphicsFeature
{
struct SkeletonMesh : RenderFeature
{
	virtual const char* MarkerName() override { return "Skeleton Mesh"; }
	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData& data, RendererCommandList& commandList, const RenderGraphBlackboard& blackboard) override;
private:
	flecs::query<const Components::Transform, const Components::SkeletonMesh> m_Query;
	PipelineStateHandle m_Handle;
	PipelineStateHandle m_ShadowHandle;
};
}
}