#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>
#include <World/GameplayFeatures/SpaceLocation.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct Battle : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
		world.m_EntityWorld.system<const Components::Transform, const Components::AttackInfo, const Components::Faction, Components::Attacking>("Acquire Target")
			.kind(flecs::PreUpdate)
			.each([](flecs::iter itr, size_t row, const Components::Transform& transform, const Components::AttackInfo& attackInfo, const Components::Faction& faction, Components::Attacking& att) {
				if (att.Target.is_valid() && att.Target.is_alive())
				{
					return;
				}

				auto spaceData = itr.world().get<SpaceLocation::SpaceData>();
				auto currentEntity = itr.entity(row);

				float closestTarget = std::numeric_limits<float>::max();
				SpaceLocation::ForEachCellTouched(transform.Position, attackInfo.AwarenessRadius, [&](uint32_t cellIndex) {
					auto currentCell = spaceData->Cells[cellIndex];
					if (currentCell.Count == 0)
					{
						return;
					}

					for (const auto e : eastl::span(&spaceData->Entities[currentCell.StartIndex], currentCell.Count))
					{
						if (faction.FactionFlag == e.get<Components::Faction>()->FactionFlag)
						{
							continue;
						}

						const auto& targetPos = e.get<Components::Transform>()->Position;
						const auto distanceSqr = glm::distance2(transform.Position, targetPos);
						if (distanceSqr < closestTarget)
						{
							closestTarget = distanceSqr;
							att.Target = e;
						}
					}
				});
			});


		world.m_EntityWorld.system<const Components::Transform, const Components::AttackInfo, Components::Attacking>("Battle Initial")
			.kind(flecs::OnUpdate)
			.each([](flecs::iter itr, size_t row, const Components::Transform& transform, const Components::AttackInfo& attackInfo, Components::Attacking& att) {
				auto spaceData = itr.world().get<SpaceLocation::SpaceData>();

				auto currentEntity = itr.entity(row);

				auto currentCell = spaceData->Cells[SpaceLocation::GetCellIndex(transform.Position)];
				assert(currentCell.Count);

				att.CurrentTime -= itr.delta_time();
				if (att.CurrentTime < 0)
					att.CurrentTime = 0;

				for (const auto e : eastl::span(&spaceData->Entities[currentCell.StartIndex], currentCell.Count))
				{
					if (e == att.Target)
					{
						const auto& targetPos = e.get<Components::Transform>()->Position;
						if (glm::distance2(targetPos, transform.Position) <= (attackInfo.Range * attackInfo.Range)
							&& att.CurrentTime <= 0)
						{
							auto health = e.get_mut<Components::Health>();
							health->CurrentHealth -= attackInfo.DamageAmount;

							att.CurrentTime = attackInfo.Speed;

							// We are bombs so self destruct for now
							//currentEntity.get_mut<Components::Health>()->CurrentHealth = 0;
						}
					}
				}
			});
	}

};
}
}