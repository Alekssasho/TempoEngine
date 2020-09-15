#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>
#include <World/Systems/BoidsSystem.h>

// As we are loading the entity world form a file, we don't need to
// explicitly register components, as they would have already been registered.
template<typename ComponentType>
void RegisterComponent(flecs::world& world)
{
	flecs::component<ComponentType>(world, ComponentType::Name);
}

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

	// Register all systems
	//RegisterSystem<Components::Transform>(Systems::MoveSystem::Run);
	m_Systems.emplace_back(new Systems::MoveSystem);
}

void World::Update(float deltaTime, Job::JobSystem& jobSystem)
{
	//m_EntityWorld.progress(deltaTime);

	TaskGraph::TaskGraph graph;
	for (const auto& system : m_Systems)
	{
		system->Update(deltaTime, graph);
	}

	graph.Compile();
	graph.Execute(jobSystem);

	// TODO: make this part of flecs systems
	//Systems::BoidsSystem boids;
	//boids.Run(m_EntityWorld, deltaTime);
}

void World::LoadFromLevel(const char* data, size_t size)
{
	flecs::writer writer(m_EntityWorld);
	writer.write(data, size);

	for (const auto& system : m_Systems)
	{
		system->PrepareQueries(*this);
	}
}
}