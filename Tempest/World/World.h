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
	void RegisterComponent(const char* name)
	{
		flecs::component<ComponentType>(m_EntityWorld, name);
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
private:
	flecs::world m_EntityWorld;
};
}