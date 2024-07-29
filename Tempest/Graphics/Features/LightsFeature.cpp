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
	m_DirectionalLightQuery.ForEach([&frameData](flecs::entity, Components::Transform& transform, Components::LightColorInfo& lightColorInfo, Tags::DirectionalLight) {
        const glm::mat4x4 scale = glm::scale(transform.Scale);
        const glm::mat4x4 rotate = glm::toMat4(transform.Rotation);
        const glm::mat4x4 translate = glm::translate(transform.Position);
		const glm::mat4x4 matrix = (translate * rotate * scale);
		const glm::vec3 lightDirection = glm::normalize(matrix * glm::vec4(sForwardDirection, 0.0f));
		const glm::vec3 lightColor = lightColorInfo.Color * lightColorInfo.Intensity;

		frameData.DirectionalLights.push_back(FrameData::DirectionalLight{
			lightDirection,
			lightColor
		});
	});
}
}
}
