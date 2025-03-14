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
		world.m_EntityWorld.system<Components::Transform, const Components::Faction>("SoldierMovementController")
			.kind(flecs::PreUpdate)
            .each([](flecs::entity, Components::Transform& transform, const Components::Faction& faction) {
				transform.Position += (transform.Rotation * sForwardDirection) * 0.01f;
			});
	}

};
}
}