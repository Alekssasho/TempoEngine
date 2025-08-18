#pragma once

#include <EngineCore.h>
#include <World/GameplayFeature.h>
#include <World/Components/Components.h>

namespace Tempest
{
namespace GameplayFeatures
{
struct AnimationController : public GameplayFeature
{
	virtual void PrepareSystems(class World& world) override
	{
        world.m_EntityWorld.observer<Components::SkeletonMesh>("Init Skeleton Meshes")
            .event(flecs::OnAdd)
            .yield_existing()
            .each([](flecs::iter& itr, size_t row, Components::SkeletonMesh& mesh) {
                AnimationManager& anim = gEngine->GetAnimation();
                // TODO: Get actual skeleton index
                anim.InitializeBoneTransforms(0, mesh.BoneTransforms);

                //itr.entity(row).set(Components::AnimationController{ 0, 0 });
            });

        world.m_EntityWorld.system<Components::SkeletonMesh, Components::AnimationController, const Components::AnimationInfo>("Animation Controller System")
            .kind(flecs::OnUpdate)
            .multi_threaded()
            .each([](flecs::iter& itr, size_t, Components::SkeletonMesh& mesh, Components::AnimationController& controller, const Components::AnimationInfo& animInfo) {
                AnimationManager& anim = gEngine->GetAnimation();

                const uint32_t animIndex = animInfo.AnimationIndices[uint32_t(controller.State)];
                auto animTime = anim.GetFrameTimeForAnimation(animIndex);
                controller.CurrentTime += itr.delta_time();

                if (controller.CurrentTime > animTime)
                {
                    controller.CurrentTime -= animTime;
                }

                anim.ApplyAnimtionWithTime(animIndex, controller.CurrentTime, mesh.BoneTransforms);
            });
	}

};
}
}