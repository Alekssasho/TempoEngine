#pragma once

#include <World/System.h>
#include <World/EntityQueryImpl.h> // For Query Init which is templated
#include <World/TaskGraph/Tasks.h>
#include <World/Components/Components.h>

// TODO: hide this in cpp
#include <PxPhysicsAPI.h>
#include <extensions/PxShapeExt.h>

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
					physicsActors[i].Actor->setGlobalPose(pxTransform, false);
				}
		});
	}
};

struct MirrorFromPhysics : public System
{
	EntityQuery m_CarQuery;
	virtual void PrepareQueries(class World& world) override
	{
		m_Query.Init<Components::Transform, Components::DynamicPhysicsActor>(world);
		m_CarQuery.Init<Components::Transform, Components::CarPhysicsPart>(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		// TODO: this is inefficient check comment in PhysicsManager.cpp why is that
		
		TaskGraph::TaskHandle handle = graph.CreateTask<Task::ParallelQueryEach>(
			"MirrorFromPhysics::DynamicActors::ParallelEach",
			&m_Query,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
				Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
				Components::DynamicPhysicsActor* physicsActors = ecs_column(iter, Components::DynamicPhysicsActor, 2);
				for (int i = 0; i < iter->count; ++i)
				{
					physx::PxTransform pxTransform = physicsActors[i].Actor->getGlobalPose();
					transform[i].Position.x = pxTransform.p.x;
					transform[i].Position.y = pxTransform.p.y;
					transform[i].Position.z = pxTransform.p.z;
					transform[i].Rotation.x = pxTransform.q.x;
					transform[i].Rotation.y = pxTransform.q.y;
					transform[i].Rotation.z = pxTransform.q.z;
					transform[i].Rotation.w = pxTransform.q.w;
				}
		});

		TaskGraph::TaskHandle carHandle = graph.CreateTask<Task::ParallelQueryEach>(
			"MirrorFromPhysics::CarPhysicsParts::ParallelEach",
			&m_CarQuery,
			[deltaTime](uint32_t, ecs_iter_t* iter) {
				Components::Transform* transform = ecs_column(iter, Components::Transform, 1);
				Components::CarPhysicsPart* carPhysics = ecs_column(iter, Components::CarPhysicsPart, 2);
				for (int i = 0; i < iter->count; ++i)
				{
					physx::PxShape* wantedShape = nullptr;
					carPhysics[i].CarActor->getShapes(&wantedShape, 1, carPhysics[i].ShapeIndex);
					physx::PxTransform pxTransform = physx::PxShapeExt::getGlobalPose(*wantedShape, *carPhysics[i].CarActor);
					transform[i].Position.x = pxTransform.p.x;
					transform[i].Position.y = pxTransform.p.y;
					transform[i].Position.z = pxTransform.p.z;
					transform[i].Rotation.x = pxTransform.q.x;
					transform[i].Rotation.y = pxTransform.q.y;
					transform[i].Rotation.z = pxTransform.q.z;
					transform[i].Rotation.w = pxTransform.q.w;
				}
		});
	}
};

}
}