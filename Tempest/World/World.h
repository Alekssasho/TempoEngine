#pragma once

// TODO: check if we can remove this from this header
#include <flecs.h>

namespace Tempest
{
class World
{
public:
	World();

	void Update(float deltaTime);

	template<typename ComponentType>
	void RegisterComponent()
	{
		flecs::component<ComponentType>(m_EntityWorld, ComponentType::Name);
	}

	template<typename ...ComponentTypes, typename UpdateFunc>
	void RegisterSystem(UpdateFunc func)
	{
		flecs::system<ComponentTypes...>(m_EntityWorld)
			.each(func);
	}

	void AddEntity()
	{
		flecs::entity(m_EntityWorld);
	}
// TODO: maybe being private is better
//private:
	flecs::world m_EntityWorld;
};
}