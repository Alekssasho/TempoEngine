#include <CommonIncludes.h>

#include <World/World.h>

#include <World/TaskGraph/TaskGraph.h>

#include <World/Components/Components.h>
#include <World/Systems/MoveSystem.h>
#include <World/Systems/BoidsSystem.h>

// As we are loading the entity world form a file, we don't need to
// explicitly register components, as they would have already been registered.
//template<typename ComponentType>
//void RegisterComponent(flecs::world& world)
//{
//	flecs::component<ComponentType>(world, ComponentType::Name);
//}

namespace Tempest
{
World::World()
{
	m_EntityWorld = ecs_init();
	//ecs_os_api_t api;
	//ecs_os_set_api(&api);

	// Enable for debug
	//ecs_tracing_enable(3);
	ecs_set_target_fps(m_EntityWorld, 60);

	// This is not needed when we are loading the world from serialized data
	// Register all components
	//RegisterComponent<Components::Transform>(m_EntityWorld);
	//RegisterComponent<Components::Rect>(m_EntityWorld);
	//RegisterComponent<Components::StaticMesh>(m_EntityWorld);
	//RegisterComponent<Components::Boids>(m_EntityWorld);

	// Test Code
	//flecs::entity rect1 = flecs::entity(m_EntityWorld)
	//	.set<Components::Transform>({ glm::vec3(-0.75f, -0.75f, 0.0f) })
	//	.set<Components::Rect>({ 0.5f, 0.5f, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f} });

	//flecs::entity rect2 = flecs::entity(m_EntityWorld)
	//	.set<Components::Transform>({ glm::vec3(0.5f, 0.5f, 0.0f) })
	//	.set<Components::Rect>({ 0.1f, 0.1f, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f} });

	// Register all systems
	//RegisterSystem<Components::Transform>(Systems::MoveSystem::Run);
	//m_Systems.emplace_back(new Systems::MoveSystem);
	m_Systems.emplace_back(new Systems::BoidsSystem);
}

World::~World()
{
	m_Systems.clear();
	ecs_fini(m_EntityWorld);
	m_EntityWorld = nullptr;
}

void World::Update(float deltaTime, Job::JobSystem& jobSystem)
{
	//m_EntityWorld.progress(deltaTime);

	TaskGraph::TaskGraph graph;
	for (const auto& system : m_Systems)
	{
		system->Update(deltaTime, graph);
	}

	graph.CompileAndExecute(jobSystem);
}

void World::LoadFromLevel(const char* data, size_t size)
{
	auto writer = ecs_writer_init(m_EntityWorld);
	ecs_writer_write(data, ecs_size_t(size), &writer);

#ifdef _DEBUG
	//int i = 0;
	//while(true)
	//{
	//	auto table = ecs_dbg_get_table(m_EntityWorld.c_ptr(), i++);
	//	if (!table)
	//		break;
	//	ecs_dbg_table_t dbg_out;
	//	ecs_dbg_table(m_EntityWorld.c_ptr(), table, &dbg_out);
	//	FORMAT_LOG(Info, World, "New table with signature %s and %d num entities", ecs_type_str(m_EntityWorld.c_ptr(), dbg_out.type), dbg_out.entities_count);
	//}
#endif

	for (const auto& system : m_Systems)
	{
		system->PrepareQueries(*this);
	}
}
}