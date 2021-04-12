#pragma once

#include <World/System.h>
#include <World/EntityQueryImpl.h> // For Query Init which is templated
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

// TODO: hide this in cpp
#include <PxPhysicsAPI.h>

namespace Tempest
{
namespace Systems
{
struct MirrorToPhysics : public System
{
	virtual void PrepareQueries(class World& world) override
	{
		m_Query.Init<Components::Transform, Components::DynamicPhysicsActor>(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(
			"MirrorToPhysics::ParallelEach",
			&m_Query,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
				Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
				Components::DynamicPhysicsActor* physicsActors = ecs_column(iter, Components::DynamicPhysicsActor, 2);
				for (int i = 0; i < iter->count; ++i)
				{
					physx::PxTransform pxTransform;
					pxTransform.p.x = transform->Position.x;
					pxTransform.p.y = transform->Position.y;
					pxTransform.p.z = transform->Position.z;
					pxTransform.q.x = transform->Rotation.x;
					pxTransform.q.y = transform->Rotation.y;
					pxTransform.q.z = transform->Rotation.z;
					pxTransform.q.w = transform->Rotation.w;
					physicsActors->Actor->setGlobalPose(pxTransform, false);
				}
		});
	}
};

struct MirrorFromPhysics : public System
{
	virtual void PrepareQueries(class World& world) override
	{
		m_Query.Init<Components::Transform, Components::DynamicPhysicsActor>(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		// TODO: this is inefficient check comment in PhysicsManager.cpp why is that
		
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(
			"MirrorFromPhysics::ParallelEach",
			&m_Query,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
				Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
				Components::DynamicPhysicsActor* physicsActors = ecs_column(iter, Components::DynamicPhysicsActor, 2);
				for (int i = 0; i < iter->count; ++i)
				{
					physx::PxTransform pxTransform = physicsActors->Actor->getGlobalPose();
					transform->Position.x = pxTransform.p.x;
					transform->Position.y = pxTransform.p.y;
					transform->Position.z = pxTransform.p.z;
					transform->Rotation.x = pxTransform.q.x;
					transform->Rotation.y = pxTransform.q.y;
					transform->Rotation.z = pxTransform.q.z;
					transform->Rotation.w = pxTransform.q.w;
				}
		});
	}
};

}
}