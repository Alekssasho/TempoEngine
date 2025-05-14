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
		world.m_EntityWorld.system<const Components::Transform, const Components::Attack>("Battle Initial")
			.kind(flecs::OnUpdate)
			.each([](flecs::iter itr, size_t row, const Components::Transform& transform, const Components::Attack& att) {
				auto spaceData = itr.world().get<SpaceLocation::SpaceData>();

				auto currentEntity = itr.entity(row);
				auto attacking = currentEntity.target<Tags::Attacks>();

				auto currentCell = spaceData->Cells[SpaceLocation::GetCellIndex(transform.Position)];
				assert(currentCell.Count);

				for (const auto e : eastl::span(&spaceData->Entities[currentCell.StartIndex], currentCell.Count))
				{
					if (e == attacking)
					{
						const auto& targetPos = e.get<Components::Transform>()->Position;
						if (glm::distance2(targetPos, transform.Position) <= (att.Radius * att.Radius))
						{
							auto health = e.get_mut<Components::Health>();
							health->CurrentHealth -= att.DamageAmount;

							// We are bombs so self destruct for now
							currentEntity.get_mut<Components::Health>()->CurrentHealth = 0;
						}
					}
				}
			});
	}

};
}
}