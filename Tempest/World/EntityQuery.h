#pragma once

struct ecs_query_t;

namespace Tempest
{
struct EntitiyQuery
{
	struct ecs_query_t* Query = nullptr;

	~EntitiyQuery();

	int GetMatchedEntitiesCount();
	int GetMatchedArchetypesCount();
};
}
