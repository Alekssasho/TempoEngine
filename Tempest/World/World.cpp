#include <World/World.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>
#include <World/Systems/BoidsSystem.h>

namespace Tempest
{
World::World()
{
	m_EntityWorld.set_target_fps(60);

	// This is not needed when we are loading the world from serialized data
	// Register all components
	//RegisterComponent<Components::Transform>();
	//RegisterComponent<Components::Rect>();

	// Test Code
	//flecs::entity rect1 = flecs::entity(m_EntityWorld)
	//	.set<Components::Transform>({ glm::vec3(-0.75f, -0.75f, 0.0f) })
	//	.set<Components::Rect>({ 0.5f, 0.5f, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} });

	//flecs::entity rect2 = flecs::entity(m_EntityWorld)
	//	.set<Components::Transform>({ glm::vec3(0.5f, 0.5f, 0.0f) })
	//	.set<Components::Rect>({ 0.1f, 0.1f, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} });
}

void World::Update(float deltaTime)
{
	m_EntityWorld.progress(deltaTime);

	// TODO: make this part of flecs systems
	Systems::BoidsSystem boids;
	boids.Run(m_EntityWorld);
}

void World::LoadFromLevel(const char* data, size_t size)
{
	flecs::writer writer(m_EntityWorld);
	writer.write(data, size);

	// Register all systems
	//RegisterSystem<Components::Transform>(Systems::MoveSystem::Run);
}
}