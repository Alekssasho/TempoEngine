#include <CommonIncludes.h>

#include <Graphics/Features/LightsFeature.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <World/World.h>

namespace Tempest
{
namespace GraphicsFeature
{

void Lights::Initialize(const World& world, Renderer& renderer)
{
	m_DirectionalLightQuery.Init(world);
}

void Lights::GatherData(const World& world, FrameData& frameData)
{
	m_DirectionalLightQuery.ForEach([&frameData](flecs::entity, Components::Transform& transform, Components::LightColorInfo& lightColorInfo, Tags::DirectionalLight&) {
		const glm::vec3 lightDirection = glm::normalize(transform.Rotation * sForwardDirection);
		const glm::vec3 lightColor = lightColorInfo.Color * lightColorInfo.Intensity;

		frameData.DirectionalLights.push_back(FrameData::DirectionalLight{
			lightDirection,
			lightColor
		});
	});
}
}
}
