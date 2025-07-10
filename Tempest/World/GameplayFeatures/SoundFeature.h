#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct Sound : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
        m_Query = world.m_EntityWorld.query_builder<const Components::CameraController>("Get Sound Listener")
			.with<Tags::SoundListener>()
            .build();

		world.m_EntityWorld.system<const Components::Transform, Components::SoundSource>("Sound System")
			.kind(flecs::PostUpdate)
			.each([this](flecs::iter itr, size_t row, const Components::Transform& transform, Components::SoundSource& soundSource) {

				Camera soundListenerLocation;
				m_Query.each([&](const Components::CameraController& transform)
				{
					soundListenerLocation = transform.CameraData;
				});

				if (soundSource.RequestedSoundEffect != -1)
				{
					glm::vec3 sourceLocation = transform.Position;

					float attenuationVolume = 1.0f;
					auto distance = glm::distance(sourceLocation, soundListenerLocation.Position);
					if (distance < 1.0f)
						attenuationVolume = 1.0f;
					else if (distance > 50.0f)
						attenuationVolume = 0.0f;
					else
						attenuationVolume = 1.0f / (1.0f + (distance - 1.0f));

					if (attenuationVolume > 0.0f)
					{
						glm::vec3 soundDir = glm::normalize(sourceLocation - soundListenerLocation.Position);
						float pan = glm::dot(soundDir, glm::normalize(glm::cross(soundListenerLocation.Up, soundListenerLocation.Forward)));
						float angle = (pan + 1.0f) * glm::quarter_pi<float>();
						float left = std::cos(angle);
						float right = std::sin(angle);

						gEngine->GetAudio().PlaySoundEffect(soundSource.RequestedSoundEffect, attenuationVolume * left, attenuationVolume * right);
					}
					soundSource.RequestedSoundEffect = -1;
				}
			});
	}
private:
	flecs::query<const Components::CameraController> m_Query;
};
}
}