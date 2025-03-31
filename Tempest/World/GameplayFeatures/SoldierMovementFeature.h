#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct SoldierMovementController : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
		world.m_EntityWorld.system<Components::Transform>("SoldierMovementController")
			.kind(flecs::PreUpdate)
			.with<Tags::Attacks>().second(flecs::Wildcard)
			.with<Tags::SimpleMovement>()
			.each([](flecs::entity e, Components::Transform& transform) {
				auto deltaTime = e.world().delta_time();
				auto target = e.target<Tags::Attacks>();
				auto targetTransform = e.world().entity(target).get_ref<Components::Transform>();

				auto dir = glm::normalize(targetTransform->Position - transform.Position);

				//transform.Position += (transform.Rotation * sForwardDirection) * 0.01f;
				transform.Position += dir * 1.0f * deltaTime;
			});
	}

};
}
}