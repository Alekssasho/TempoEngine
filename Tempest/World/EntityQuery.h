#pragma once

#include <Defines.h>

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
	ecs_iter_t GetIterForAchetype(uint32_t index);
};
}
