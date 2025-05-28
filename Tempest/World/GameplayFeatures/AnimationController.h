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
            .event(flecs::OnSet)
            .each([](Components::SkeletonMesh& mesh) {
                AnimationManager& anim = gEngine->GetAnimation();
                // TODO: Get actual skeleton index
                anim.InitializeBoneTransforms(0, mesh.BoneTransforms);
            });

	}

};
}
}