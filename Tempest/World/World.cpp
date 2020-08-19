#include <World/World.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>

namespace Tempest
{
World::World()
{
	m_EntityWorld.set_target_fps(60);

	// Register all components
	RegisterComponent<Components::Transform>();
	RegisterComponent<Components::Rect>();

	// Register all systems
	//RegisterSystem<Components::Transform>(Systems::MoveSystem::Run);

	// Test Code
	flecs::entity rect1 = flecs::entity(m_EntityWorld)
		.set<Components::Transform>({ glm::vec3(-0.75f, -0.75f, 0.0f) })
		.set<Components::Rect>({ 0.5f, 0.5f, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} });

	flecs::entity rect2 = flecs::entity(m_EntityWorld)
		.set<Components::Transform>({ glm::vec3(0.5f, 0.5f, 0.0f) })
		.set<Components::Rect>({ 0.1f, 0.1f, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} });
}

void World::Update(float deltaTime)
{
	m_EntityWorld.progress(deltaTime);
}
}