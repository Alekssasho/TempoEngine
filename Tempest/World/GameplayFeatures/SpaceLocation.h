#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct SpaceLocation : public GameplayFeature
{
	static const uint32_t s_WorldSize = 50;
	static const uint32_t s_WorldCellDivisions = 11;

	static uint32_t GetCellIndex(glm::vec3 position)
	{
		position += glm::vec3(s_WorldSize / 2.0f, s_WorldSize / 2.0f, s_WorldSize / 2.0f);
		position /= glm::vec3(s_WorldSize, s_WorldSize, s_WorldSize);
		position *= glm::vec3(s_WorldCellDivisions, s_WorldCellDivisions, s_WorldCellDivisions);

		return uint32_t(floor(position.x) + s_WorldCellDivisions * floor(position.z));
	}

	struct SpaceData
	{
		struct Cell
		{
			uint32_t StartIndex;
			uint32_t Count;
		};
		eastl::vector<Cell> Cells;
		eastl::vector<flecs::entity> Entities;
	};

	virtual void PrepareSystems(class World& world) override
	{
		// TODO: Check if it is cached
		m_Query = world.m_EntityWorld.query_builder<const Components::Transform>("Space Location Get Entities")
			.with<Components::Faction>()
			.build();

		world.m_EntityWorld.component<SpaceData>();
		world.m_EntityWorld.set<SpaceData>({});

		// This could probably use different phases
		world.m_EntityWorld.system<SpaceData>("Space Location Task")
			.term_at(0).singleton()
			.kind(flecs::PreUpdate)
			.each([this](SpaceData& spaceData) {
			// Phase 1
			// Get count per cell so we can allocate the data
			spaceData.Entities.clear();

			spaceData.Cells.assign(s_WorldCellDivisions * s_WorldCellDivisions, SpaceData::Cell{ 0, 0 });

			uint32_t totalCount = 0;
			m_Query.each([&](flecs::entity e, const Components::Transform& transform) {
				spaceData.Cells[GetCellIndex(transform.Position)].Count++;
				totalCount++;
			});

			spaceData.Entities.resize(totalCount);
			eastl::array<uint32_t, s_WorldCellDivisions * s_WorldCellDivisions> currentIndexPerCell;
			uint32_t currentStartIndex = 0;
			// Phase 2
			// Allocate space for entities in proper location
			for (uint32_t i = 0; i < spaceData.Cells.size(); ++i)
			{
				auto& cell = spaceData.Cells[i];

				cell.StartIndex = currentStartIndex;
				currentIndexPerCell[i] = cell.StartIndex;
				currentStartIndex += cell.Count;
			}

			// Phase 3
			// Add entities to proper cell
			m_Query.each([&](flecs::entity e, const Components::Transform& transform) {
				auto cellIndex = GetCellIndex(transform.Position);
				spaceData.Entities[currentIndexPerCell[cellIndex]] = e;
				currentIndexPerCell[cellIndex]++;
			});
		});
	}

private:
	flecs::query<const Components::Transform> m_Query;
};
}
}