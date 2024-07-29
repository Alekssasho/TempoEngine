#pragma once

#include <Defines.h>
#include <EASTL/utility.h>
#include <World/World.h>

namespace Tempest
{
template<typename... Components>
struct EntityQuery
{
	flecs::query<Components...> Query;
	const World* m_world;

	~EntityQuery()
	{
		//Query.destruct();
	}

	void Init(const World& world)
	{
		Query = world.m_EntityWorld.query<Components...>();
		m_world = &world;
	}

	template<typename Func>
	void ForEach(Func&& func)
	{
		Query.each(std::forward<Func>(func));
	}

	template<typename Func>
	void ForEachWorker(uint32_t current, uint32_t count, Func&& func, uint32_t stage)
	{
		//auto iter = Query.worker(current, count);
		//iter.each(std::forward<Func>(func));
		Query.each(m_world->m_EntityWorld.get_stage(stage), std::forward<Func>(func));
	}

	int GetMatchedEntitiesCount()
	{
		int result = 0;
		Query.iter([&result](flecs::iter it, Components*...) {
			result += int(it.count());
		});

		return result;
	}
};
}
