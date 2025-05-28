#include <CommonIncludes.h>

#include <Engine.h>
#include <Animation/AnimationManager.h>

#include <DataDefinitions/AnimationDatabase_generated.h>

namespace Tempest
{
AnimationManager::AnimationManager()
{

}

void AnimationManager::InitializeBoneTransforms(uint32_t skeletonIndex, eastl::vector<glm::mat4x4>& transforms)
{
    const Definition::Skeleton* skeleton = (*m_Database->skeletons())[skeletonIndex];
    const auto& bonesArray = *m_Database->bones();

    transforms.reserve(skeleton->count());
    for (uint32_t boneIndex = 0; boneIndex < skeleton->count(); ++boneIndex)
    {
        const Definition::Bone* bone = bonesArray[skeleton->start_index() + boneIndex];

        const glm::mat4x4 boneRotation = glm::toMat4(glm::quat(bone->local_rotation().w(), bone->local_rotation().x(), bone->local_rotation().y(), bone->local_rotation().z()));
        const glm::mat4x4 bonePosition = glm::translate(glm::vec3(bone->local_position().x(), bone->local_position().y(), bone->local_position().z()));

        glm::mat4x4 finalTransform;
        if (bone->parent() == -1)
        {
            finalTransform = bonePosition * boneRotation;
        }
        else
        {
            finalTransform = transforms[bone->parent() - skeleton->start_index()] * bonePosition * boneRotation;
        }

        transforms.emplace_back(finalTransform);
    }
}

void AnimationManager::LoadDatabase(const char* databaseName)
{
    const Definition::AnimationDatabase* animationDatabase = gEngine->GetResourceLoader().LoadResource<Definition::AnimationDatabase>(databaseName);
    if (!animationDatabase)
    {
        LOG(Warning, Animation, "Animation Database is Invalid!");
        return;
    }

    m_Database = animationDatabase;
}
}
