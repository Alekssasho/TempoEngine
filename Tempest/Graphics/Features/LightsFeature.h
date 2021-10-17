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
	// TODO: Better ID
	static const int64_t ID = 2;

	virtual void Initialize(const World& world, Renderer& renderer) override;
	virtual void GatherData(const World&, FrameData&) override;
	virtual void GenerateCommands(const FrameData&, RendererCommandList&, const RenderGraphBlackboard&) override {};
private:
	EntityQuery<Components::Transform, Components::LightColorInfo, Tags::DirectionalLight> m_DirectionalLightQuery;
};
}
}