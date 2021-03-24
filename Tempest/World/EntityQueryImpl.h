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
	Query = ecs_query_new(const_cast<World&>(world).m_EntityWorld, querySignature.c_str());
}
}