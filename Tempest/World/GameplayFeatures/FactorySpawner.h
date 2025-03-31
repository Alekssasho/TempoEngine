#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct FactorySpawner : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
		world.m_EntityWorld.system<const Components::Transform, Components::Factory, const Components::Faction>("FactorySpawner")
			.kind(flecs::PreUpdate)
            .each([](flecs::iter& it, size_t, const Components::Transform& transform, Components::Factory& factory, const Components::Faction& faction) {

                auto manager = it.world().get<Components::CastleManager>();

                factory.CurrentTime += it.delta_time();
                if (factory.CurrentTime >= factory.TimeToSpawn)
                {
                    it.world().entity()
                        .is_a(factory.PrefabToSpawn)
                        .set(transform)
                        .add<Tags::Attacks>(manager->Castles[uint32_t(faction.FactionFlag)]);
                    factory.CurrentTime = 0.0f;
                }
			});
	}

};
}
}