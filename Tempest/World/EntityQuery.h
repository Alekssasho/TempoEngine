#pragma once

#include <Defines.h>

struct ecs_query_t;
struct ecs_iter_t;

namespace Tempest
{
struct EntitiyQuery
{
	struct ecs_query_t* Query = nullptr;

	~EntitiyQuery();

	int GetMatchedEntitiesCount();
	int GetMatchedArchetypesCount();
	ecs_iter_t GetIterForAchetype(uint32_t index);
};
}
