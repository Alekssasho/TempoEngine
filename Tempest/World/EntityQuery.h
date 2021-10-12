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

	~EntityQuery()
	{
		Query.destruct();
	}

	void Init(const World& world)
	{
		Query = world.m_EntityWorld.query<Components...>();
	}

	template<typename Func>
	void ForEach(Func&& func)
	{
		assert(Query.c_ptr());
		Query.each(std::forward<Func>(func));
	}

	template<typename Func>
	void ForEachWorker(uint32_t current, uint32_t count, Func&& func)
	{
		assert(Query.c_ptr());
		Query.each_worker(current, count, std::forward<Func>(func));
	}

	int GetMatchedEntitiesCount()
	{
		assert(Query.c_ptr());

		int result = 0;
		Query.iter([&result](flecs::iter it, Components*...) {
			result += int(it.count());
		});

		return result;
	}
};
}
