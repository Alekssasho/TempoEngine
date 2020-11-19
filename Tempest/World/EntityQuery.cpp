#include <World/EntityQuery.h>

#define FLECS_NO_CPP
#include <flecs.h>

namespace Tempest
{
EntityQuery::~EntityQuery()
{
	if (Query)
	{
		ecs_query_free(Query);
	}
}

int EntityQuery::GetMatchedArchetypesCount()
{
	if (!Query)
	{
		return 0;
	}
	ecs_iter_t iter = ecs_query_iter(Query);
	return iter.table_count;
}

int EntityQuery::GetMatchedEntitiesCount()
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
	return result;
}

eastl::pair<uint32_t, ecs_iter_t> EntityQuery::GetIterForAchetype(uint32_t index)
{
	if (!Query)
	{
		return { 0, {} };
	}

	ecs_iter_t iter = ecs_query_iter(Query);
	ecs_query_next(&iter); // Go to the first archetype
	uint32_t entityCount = 0;
	while (index--)
	{
		entityCount += iter.count;
		ecs_query_next(&iter);
	}

	return { entityCount, iter };
}
}