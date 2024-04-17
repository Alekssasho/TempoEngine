#pragma once

#include <Engine.h>
#include <World/GameplayFeature.h>
#include <World/Components/Components.h>

#include <Physics/PhysicsManager.h>
// TODO: hide this in cpp
#include <PxPhysicsAPI.h>
#include <extensions/PxShapeExt.h>

namespace Tempest
{
namespace GameplayFeatures
{

class Physics : public GameplayFeature
{
public:
    virtual void PrepareSystems(class World& world) override
    {
        world.m_EntityWorld
            .system<Components::Transform, Components::DynamicPhysicsActor>("MirrorFromPhysicsDynamicActors")
            .kind(flecs::PostUpdate)
            .each(&Physics::MirrorFromPhysicsDynamicActors);

        world.m_EntityWorld
            .system<Components::Transform, Components::CarPhysicsPart>("MirrorFromPhysicsCar")
            .kind(flecs::PostUpdate)
            .each(&Physics::MirrorFromPhysicsCar);

        world.m_EntityWorld
            .system<>("Physics Update")
            .kind(flecs::OnUpdate)
            //.singleton()
            .iter(&Physics::PhysicsUpdate);
    }

    static void PhysicsUpdate(flecs::iter& it)
    {
        gEngine->GetPhysics().Update(it.delta_time());
    }

    static void MirrorFromPhysicsDynamicActors(flecs::entity, Components::Transform& transform, Components::DynamicPhysicsActor& physicsActor)
    {
        physx::PxTransform pxTransform = physicsActor.Actor->getGlobalPose();
        transform.Position.x = pxTransform.p.x;
        transform.Position.y = pxTransform.p.y;
        transform.Position.z = pxTransform.p.z;
        transform.Rotation.x = pxTransform.q.x;
        transform.Rotation.y = pxTransform.q.y;
        transform.Rotation.z = pxTransform.q.z;
        transform.Rotation.w = pxTransform.q.w;
    }


    static void MirrorFromPhysicsCar(flecs::entity, Components::Transform& transform, Components::CarPhysicsPart& carPhysics)
    {
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
    }
};

}
}