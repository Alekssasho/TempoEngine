#pragma once

#include <World/System.h>
#include <World/EntityQueryImpl.h> // For Query Init which is templated
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
		m_Query.Init<Components::Transform>(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(
			"MoveSystem::ParallelEach",
			&m_Query,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
				Components::Transform* transform = ecs_term(iter, Components::Transform, 1);
				for (int i = 0; i < iter->count; ++i)
				{
					transform[i].Position += glm::vec3{ 0.2f, 0.0f, 0.0f } * deltaTime;
				}
			});
	}
};

}
}