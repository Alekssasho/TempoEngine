#pragma once

#include <Graphics/RenderFeature.h>
#include <World/EntityQuery.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GraphicsFeature
{
struct Debug : RenderFeature
{
	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData& data, RendererCommandList& commandList, const RenderGraphBlackboard& blackboard) override;

private:
    PipelineStateHandle m_HandleRect;
    PipelineStateHandle m_HandleCube;
	flecs::query<const Components::SkeletonMesh, const Components::Transform> m_SkeletonQuery;
};
}
}