#pragma once

#include <EngineCore.h>
#include <World/System.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct MoveSystem : public System
{
	virtual void PrepareQueries(class World& world) override
	{
		// TODO: wrap this in special function
		flecs::query<Components::Transform> query(world.m_EntityWorld);
		m_Query.Query = query.c_ptr();
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		// TODO: Change the ForEach to take the components and be pretty :D
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(&m_Query, [deltaTime](ecs_iter_t* iter) {
			using namespace Components;
			ECS_COLUMN(iter, Transform, transform, 1);
			for (int i = 0; i < iter->count; ++i)
			{
				transform[i].Position += glm::vec3{ 0.2f, 0.0f, 0.0f } *deltaTime;
			}
		});
	}
};

}
}