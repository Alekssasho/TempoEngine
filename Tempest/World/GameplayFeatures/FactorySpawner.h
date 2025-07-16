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
            .each([](flecs::iter& it, size_t row, const Components::Transform& transform, Components::Factory& factory, const Components::Faction& faction) {

                auto& manager = it.world().get<Components::CastleManager>();

                factory.CurrentTime += it.delta_time();
                if (factory.CurrentTime >= factory.TimeToSpawn)
                {
                    eastl::string spawnedEntityName;
                    spawnedEntityName.sprintf("%s_Spawned_%d", it.entity(row).name().c_str(), factory.NumSpawned);

                    it.world().entity(spawnedEntityName.c_str())
                        .is_a(factory.PrefabToSpawn)
                        .set(transform)
                        .set(Components::LaneMovement{}) // TODO: maybe we can set in the prefab
                        .add<Tags::Attacks>(manager.Castles[uint32_t(faction.FactionFlag)]);
                    factory.CurrentTime = 0.0f;
                    factory.NumSpawned++;
                }
			});
	}

};
}
}