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
    assert(transforms.size() == 0);
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

void AnimationManager::ApplyFrameFromAnimation(uint32_t animationIndex, uint32_t frameIndex, eastl::vector<glm::mat4x4>& transforms)
{
    const Definition::Animation* animation = (*m_Database->animations())[animationIndex];
    const Definition::Skeleton* skeleton = (*m_Database->skeletons())[animation->skeleton_index()];
    const auto& bonesArray = *m_Database->bones();

    assert(transforms.size() == skeleton->count());

    struct BoneFrameData
    {
        glm::vec3 Translation;
        glm::quat Rotation;
    };
    const uint8_t* data = m_Database->anim_data()->Data();

    const BoneFrameData* frameData = reinterpret_cast<const BoneFrameData*>(data + animation->start_index()) + (frameIndex * skeleton->count());

    for (uint32_t boneIndex = 0; boneIndex < skeleton->count(); ++boneIndex)
    {
        const Definition::Bone* bone = bonesArray[skeleton->start_index() + boneIndex];

        const BoneFrameData* currentBoneFrameData = frameData + boneIndex;

        const glm::mat4x4 boneRotation = glm::toMat4(currentBoneFrameData->Rotation);
        const glm::mat4x4 bonePosition = glm::translate(currentBoneFrameData->Translation);

        glm::mat4x4 finalTransform;
        if (bone->parent() == -1) // Only root has no parent, as we don't have root motion only use rotation data
        {
            finalTransform = boneRotation;
        }
        else
        {
            finalTransform = transforms[bone->parent() - skeleton->start_index()] * bonePosition * boneRotation;
        }

        transforms[boneIndex] = finalTransform;
    }
}

void AnimationManager::ApplyInverseBindMatrices(uint32_t skeletonIndex, eastl::vector<glm::mat4x4>& transforms)
{
    if (transforms.size() == 0)
        return;
    const Definition::Skeleton* skeleton = (*m_Database->skeletons())[skeletonIndex];
    const auto& bonesArray = *m_Database->bones();
    const auto& inverseBindMatricesArray = *m_Database->inverse_bind_matrices();
    const auto& inverseBoneMatrices = reinterpret_cast<const glm::mat4x4*>(inverseBindMatricesArray.data() + skeleton->inverse_bind_matrices_start_index());

    assert(transforms.size() == skeleton->count());

    for (uint32_t boneIndex = 0; boneIndex < skeleton->count(); ++boneIndex)
    {
        transforms[boneIndex] = transforms[boneIndex] * inverseBoneMatrices[boneIndex];
    }
}

uint32_t AnimationManager::GetNumFramesForAnimation(uint32_t animationIndex)
{
    const Definition::Animation* animation = (*m_Database->animations())[animationIndex];
    return animation->num_frames();
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
