#include <World/World.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>

namespace Tempest
{
World::World()
{
	m_EntityWorld.set_target_fps(60);

	// Register all components
	RegisterComponent<Components::Transform>("Transform");

	// Register all systems
	RegisterSystem<Components::Transform>(Systems::MoveSystem::Run);
}

void World::Update(float deltaTime)
{
	m_EntityWorld.progress(deltaTime);
}
}