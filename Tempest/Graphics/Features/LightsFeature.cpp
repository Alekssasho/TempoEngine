#include <CommonIncludes.h>

#include <Graphics/Features/LightsFeature.h>
#include <Graphics/Renderer.h>
#include <Graphics/FrameData.h>
#include <World/World.h>
#include <World/Components/Components.h>
#include <World/EntityQueryImpl.h>

namespace Tempest
{
namespace GraphicsFeature
{

void Lights::Initialize(const World& world, Renderer& renderer)
{
	m_DirectionalLightQuery.Init<Components::Transform, Components::LightColorInfo, Tags::DirectionalLight>(world);
}

void Lights::GatherData(const World& world, FrameData& frameData)
{
	int archetypeCount = m_DirectionalLightQuery.GetMatchedArchetypesCount();
	for (int i = 0; i < archetypeCount; ++i)
	{
		auto [_, iter] = m_DirectionalLightQuery.GetIterForAchetype(i);
		Components::Transform* transforms = ecs_column(&iter, Components::Transform, 1);
		Components::LightColorInfo* lightColorInfos = ecs_column(&iter, Components::LightColorInfo, 2);
		for (int row = 0; row < iter.count; ++row)
		{
			const glm::vec3 lightDirection = glm::normalize(transforms[row].Rotation * sForwardDirection);
			const glm::vec3 lightColor = lightColorInfos[row].Color * lightColorInfos[row].Intensity;

			frameData.DirectionalLights.push_back(FrameData::DirectionalLight{
				lightDirection,
				lightColor
			});
		}
	}
}
}
}
