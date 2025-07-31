#pragma once

#include <Graphics/RenderFeature.h>
#include <World/EntityQuery.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GraphicsFeature
{
// Special RenderFeature which only job is to gather the light data from the world and prepare it for rendering. Doesn't generate any rendering commands
struct Lights : RenderFeature
{
	virtual const char* MarkerName() override { return "Lights"; }
	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData&, RendererCommandList&, const RenderGraphBlackboard&) override {};
private:
	EntityQuery<Components::Transform, Components::LightColorInfo, Tags::DirectionalLight> m_DirectionalLightQuery;
};
}
}