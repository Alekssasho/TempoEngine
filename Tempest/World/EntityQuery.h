#pragma once

#include <Defines.h>
#include <EASTL/utility.h>
#include <World/World.h>

namespace Tempest
{
template<typename... Components>
struct EntityQuery
{
	struct ecs_query_t* Query = nullptr;
	struct ecs_world_t* m_World = nullptr;

	~EntityQuery()
	{
		if (Query)
		{
			//ecs_query_free(Query);
			ecs_query_fini(Query);
		}
	}

	void Init(const World& world)
	{
		m_World = const_cast<World&>(world).m_EntityWorld;
		Query = const_cast<World&>(world).m_EntityWorld.query<Components...>().c_ptr();
	}

	int GetMatchedEntitiesCount()
	{
		if (!Query)
		{
			return 0;
		}

		ecs_iter_t iter = ecs_query_iter(m_World, Query);
		int result = 0;
		while (ecs_query_next(&iter)) {
			result += iter.count;
		}
		return result;
	}

	int GetMatchedArchetypesCount()
	{
		if (!Query)
		{
			return 0;
		}
		ecs_iter_t iter = ecs_query_iter(m_World, Query);
		return iter.table_count;
	}
	// Returns the number of entities before this one
	eastl::pair<uint32_t, ecs_iter_t> GetIterForAchetype(uint32_t index)
	{
		if (!Query)
		{
			return { 0, {} };
		}

		ecs_iter_t iter = ecs_query_iter(m_World, Query);
		ecs_query_next(&iter); // Go to the first archetype
		uint32_t entityCount = 0;
		while (index--)
		{
			entityCount += iter.count;
			ecs_query_next(&iter);
		}

		return { entityCount, iter };
	}
};
}
