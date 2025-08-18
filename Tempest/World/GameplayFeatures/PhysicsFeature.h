#pragma once

#include <Engine.h>
#include <World/GameplayFeature.h>
#include <World/Components/Components.h>

#include <Physics/PhysicsManager.h>

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
            .system<const Components::Transform, const Components::PhysicsBody>("MirrorToPhysicsBodies")
            .kind(flecs::PreUpdate)
            .each([](const Components::Transform& transform, const Components::PhysicsBody& physicsBody) {
                gEngine->GetPhysics().CopyTransformToBody(transform, physicsBody.ID);
            });

        //world.m_EntityWorld
        //    .system<Components::Transform, Components::CarPhysicsPart>("MirrorFromPhysicsCar")
        //    .kind(flecs::PostUpdate)
        //    .each(&Physics::MirrorFromPhysicsCar);

        world.m_EntityWorld.observer<Components::PrefabPhysicsCreationRequest>("Init Prefab Physics")
            .event(flecs::OnAdd)
            .yield_existing()
            .with<const Components::Transform>().filter()
            .each([](flecs::iter itr, size_t row, Components::PrefabPhysicsCreationRequest& request) {
                auto e = itr.entity(row);

                const Components::Transform& transform = itr.field_at<const Components::Transform>(1, row);

                JPH::BodyID newId = gEngine->GetPhysics().CreateBodyFromPrefabScene(request.Index, transform);

                e.remove<Components::PrefabPhysicsCreationRequest>();
                e.set(Components::PhysicsBody{ newId });
            });

        world.m_EntityWorld.observer<Components::PhysicsBody>("Delete Physics Body")
            .event(flecs::OnRemove)
            .each([](Components::PhysicsBody& physicsBody) {
                gEngine->GetPhysics().RemoveBody(physicsBody.ID);
            });

        world.m_EntityWorld.system("Physics Update")
            .kind(flecs::OnUpdate)
            .run([](flecs::iter& it) {
                gEngine->GetPhysics().Update(it.delta_time());
            });
    }

    //static void MirrorFromPhysicsDynamicActors(flecs::entity, Components::Transform& transform, Components::DynamicPhysicsActor& physicsActor)
    //{
    //    physx::PxTransform pxTransform = physicsActor.Actor->getGlobalPose();
    //    transform.Position.x = pxTransform.p.x;
    //    transform.Position.y = pxTransform.p.y;
    //    transform.Position.z = pxTransform.p.z;
    //    transform.Rotation.x = pxTransform.q.x;
    //    transform.Rotation.y = pxTransform.q.y;
    //    transform.Rotation.z = pxTransform.q.z;
    //    transform.Rotation.w = pxTransform.q.w;
    //}


    //static void MirrorFromPhysicsCar(flecs::entity, Components::Transform& transform, Components::CarPhysicsPart& carPhysics)
    //{
    //    physx::PxShape* wantedShape = nullptr;
    //    carPhysics.CarActor->getShapes(&wantedShape, 1, carPhysics.ShapeIndex);
    //    physx::PxTransform pxTransform = physx::PxShapeExt::getGlobalPose(*wantedShape, *carPhysics.CarActor);
    //    transform.Position.x = pxTransform.p.x;
    //    transform.Position.y = pxTransform.p.y;
    //    transform.Position.z = pxTransform.p.z;
    //    transform.Rotation.x = pxTransform.q.x;
    //    transform.Rotation.y = pxTransform.q.y;
    //    transform.Rotation.z = pxTransform.q.z;
    //    transform.Rotation.w = pxTransform.q.w;
    //}
};

}
}