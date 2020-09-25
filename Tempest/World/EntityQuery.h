#pragma once

#include <Defines.h>
#include <EASTL/utility.h>

struct ecs_query_t;
struct ecs_iter_t;

namespace Tempest
{
struct EntityQuery
{
	struct ecs_query_t* Query = nullptr;

	~EntityQuery();

	template<typename... Components>
	void Init(class World& world);

	int GetMatchedEntitiesCount();
	int GetMatchedArchetypesCount();
	// Returns the number of entities before this one
	eastl::pair<uint32_t, ecs_iter_t> GetIterForAchetype(uint32_t index);
};
}
