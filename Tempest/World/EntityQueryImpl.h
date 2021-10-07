#pragma once

#include <World/World.h>

namespace Tempest
{

template<typename Component>
eastl::string buildSignature()
{
	return Component::Name;
}

template<typename T, typename ...Components, typename std::enable_if<sizeof...(Components) >= 1, int>::type = 0>
eastl::string buildSignature()
{
	return buildSignature<T>() + eastl::string(",") + buildSignature<Components...>();
}

template<typename... Components>
void EntityQuery::Init(const World& world)
{
	// TODO: Remove the const cast
	eastl::string querySignature = buildSignature<Components...>();
	ecs_query_desc_t desc = {};
	desc.filter.expr = querySignature.c_str();
	Query = ecs_query_init(const_cast<World&>(world).m_EntityWorld, &desc);
	m_World = const_cast<World&>(world).m_EntityWorld;
}
}