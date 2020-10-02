#pragma once

#include <World/World.h>

namespace Tempest
{
template<typename... Components>
void EntityQuery::Init(const World& world)
{
	// TODO: Remove the const cast
	flecs::query<Components...> query(const_cast<World&>(world).m_EntityWorld);
	Query = query.c_ptr();
}
}