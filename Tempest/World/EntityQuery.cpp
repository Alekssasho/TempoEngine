#include <World/EntityQuery.h>

#include <flecs.h>

namespace Tempest
{

EntitiyQuery::~EntitiyQuery()
{
	if (Query)
	{
		ecs_query_free(Query);
	}
}

int EntitiyQuery::GetMatchedArchetypesCount()
{
	if (!Query)
	{
		return 0;
	}
	ecs_iter_t iter = ecs_query_iter(Query);
	return iter.table_count;
}

int EntitiyQuery::GetMatchedEntitiesCount()
{
	if (!Query)
	{
		return 0;
	}

	ecs_iter_t iter = ecs_query_iter(Query);
	int result = 0;
	while (ecs_query_next(&iter)) {
		result += iter.count;
	}
	return iter.table_count;
}

ecs_iter_t EntitiyQuery::GetIterForAchetype(uint32_t index)
{
	if (!Query)
	{
		return {};
	}

	ecs_iter_t iter = ecs_query_iter(Query);
	int result = 0;
	do {
		ecs_query_next(&iter);
	} while (index--);

	return iter;
}
}