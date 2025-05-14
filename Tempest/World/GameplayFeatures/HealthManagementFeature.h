#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct HealthManagement : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
		world.m_EntityWorld.system<const Components::Health>("Dying")
			.kind(flecs::PostUpdate)
			.each([](flecs::entity e, const Components::Health& health) {
				if (health.CurrentHealth <= 0.0f)
				{
					e.destruct();
				}
			});
	}

};
}
}