#pragma once

#include <World/System.h>
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace Systems
{
struct MoveSystem : public System
{
	EntityQuery<Components::Transform> m_Query;

	virtual void PrepareQueries(class World& world) override
	{
		m_Query.Init(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.AddTask(
			"MoveSystem::ParallelEach",
			new Task::ParallelQueryEach(&m_Query, [deltaTime](flecs::entity, Components::Transform& transform) {
				transform.Position += glm::vec3{ 0.2f, 0.0f, 0.0f } * deltaTime;
		}, 0));
	}
};

}
}