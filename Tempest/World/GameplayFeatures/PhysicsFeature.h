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
        //world.m_EntityWorld
        //    .system<const Components::Transform, const Components::PhysicsBody>("MirrorToPhysicsBodies")
        //    .kind(flecs::PreUpdate)
        //    .each([](const Components::Transform& transform, const Components::PhysicsBody& physicsBody) {
        //        gEngine->GetPhysics().CopyTransformToBody(transform, physicsBody.ID);
        //    });

        world.m_EntityWorld
            .system<Components::Transform, const Components::PhysicsBody>("MirrorFromPhysicsBodies")
            .kind(flecs::PostUpdate)
            .each([](Components::Transform& transform, const Components::PhysicsBody& physicsBody) {
                gEngine->GetPhysics().CopyTransformFromBody(transform, physicsBody.ID);
             });

        world.m_EntityWorld.system<Components::Transform, const Components::Movement, const Components::MovementInfo, const Components::PhysicsBody>("Kinematic Movement System")
            .kind(flecs::PreUpdate)
            .multi_threaded()
            .with<Tags::SimpleMovement>()
            .each([](Components::Transform& transform, const Components::Movement& movement, const Components::MovementInfo& movementInfo, const Components::PhysicsBody& body) {
                    gEngine->GetPhysics().SetVelocity(body.ID, movement.Velocity * movementInfo.Speed, glm::vec3(0.0f, 0.0f, 0.0f));
                    if (glm::length2(movement.Velocity) != 0.0f)
                    {
                        transform.Rotation = glm::rotation(sForwardDirection, movement.Velocity);
                        gEngine->GetPhysics().CopyTransformToBody(transform, body.ID);
                    }
                });

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
};

}
}