#pragma once

#include <World/World.h>

namespace Tempest
{
template<typename... Components>
void EntityQuery::Init(World& world)
{
	flecs::query<Components...> query(world.m_EntityWorld);
	Query = query.c_ptr();
}
}