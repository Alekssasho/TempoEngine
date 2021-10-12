#pragma once

#include <World/System.h>
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
	EntityQuery<Components::Transform, Components::DynamicPhysicsActor> m_Query;

	virtual void PrepareQueries(class World& world) override
	{
		m_Query.Init(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		TaskGraph::TaskHandle handle = graph.AddTask(
			"MirrorToPhysics::ParallelEach",
			new Task::ParallelQueryEach(
				&m_Query,
				[deltaTime](flecs::entity, Components::Transform& transform, Components::DynamicPhysicsActor& physicsActor) {
					physx::PxTransform pxTransform;
					pxTransform.p.x = transform.Position.x;
					pxTransform.p.y = transform.Position.y;
					pxTransform.p.z = transform.Position.z;
					pxTransform.q.x = transform.Rotation.x;
					pxTransform.q.y = transform.Rotation.y;
					pxTransform.q.z = transform.Rotation.z;
					pxTransform.q.w = transform.Rotation.w;
					physicsActor.Actor->setGlobalPose(pxTransform, false);
		}));
	}
};

struct MirrorFromPhysics : public System
{
	EntityQuery<Components::Transform, Components::DynamicPhysicsActor> m_Query;
	EntityQuery<Components::Transform, Components::CarPhysicsPart> m_CarQuery;

	virtual void PrepareQueries(class World& world) override
	{
		m_Query.Init(world);
		m_CarQuery.Init(world);
	}

	virtual void Update(float deltaTime, TaskGraph::TaskGraph& graph) override
	{
		// TODO: this is inefficient check comment in PhysicsManager.cpp why is that
		
		TaskGraph::TaskHandle handle = graph.AddTask(
			"MirrorFromPhysics::DynamicActors::ParallelEach",
			new Task::ParallelQueryEach(
				&m_Query,
				[deltaTime](flecs::entity, Components::Transform& transform, Components::DynamicPhysicsActor& physicsActor) {
					physx::PxTransform pxTransform = physicsActor.Actor->getGlobalPose();
					transform.Position.x = pxTransform.p.x;
					transform.Position.y = pxTransform.p.y;
					transform.Position.z = pxTransform.p.z;
					transform.Rotation.x = pxTransform.q.x;
					transform.Rotation.y = pxTransform.q.y;
					transform.Rotation.z = pxTransform.q.z;
					transform.Rotation.w = pxTransform.q.w;
		}));

		TaskGraph::TaskHandle carHandle = graph.AddTask(
			"MirrorFromPhysics::CarPhysicsParts::ParallelEach",
			new Task::ParallelQueryEach(
				&m_CarQuery,
				[deltaTime](flecs::entity, Components::Transform& transform, Components::CarPhysicsPart& carPhysics) {
					physx::PxShape* wantedShape = nullptr;
					carPhysics.CarActor->getShapes(&wantedShape, 1, carPhysics.ShapeIndex);
					physx::PxTransform pxTransform = physx::PxShapeExt::getGlobalPose(*wantedShape, *carPhysics.CarActor);
					transform.Position.x = pxTransform.p.x;
					transform.Position.y = pxTransform.p.y;
					transform.Position.z = pxTransform.p.z;
					transform.Rotation.x = pxTransform.q.x;
					transform.Rotation.y = pxTransform.q.y;
					transform.Rotation.z = pxTransform.q.z;
					transform.Rotation.w = pxTransform.q.w;
		}));
	}
};

}
}