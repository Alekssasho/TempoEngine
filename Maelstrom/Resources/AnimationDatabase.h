#pragma once

#include "Resource.h"

#include "../GLTFScene.h"

#include <DataDefinitions/AnimationDatabase_generated.h>

struct SkeletonRequest
{
    uint32_t SceneIndex;
    uint32_t SkeletonIndex;
};

struct AnimationDatabaseResult
{
    eastl::vector<uint8_t> CompiledData;
    eastl::vector<eastl::unordered_map<uint32_t, uint32_t>> SkeletonJointsMapping;
};

struct AnimationDatabaseResource : Resource<AnimationDatabaseResult>
{
public:
	AnimationDatabaseResource(const Scene& scene)
		: m_Scenes(&scene, 1)
    {
		//TODO:
    }

	AnimationDatabaseResource(eastl::span<const Scene> scenes, eastl::span<SkeletonRequest> requests)
		: m_Scenes(scenes)
	{
		m_Requests.assign(requests.begin(), requests.end());
	}

	void Compile() override
	{
        eastl::vector<Tempest::Definition::Skeleton> skeletons;
        eastl::vector<Tempest::Definition::Bone> bones;
        eastl::vector<uint8_t> animationDataArray;
        eastl::vector<Tempest::Definition::Animation> animationsArray;
        eastl::vector<uint8_t> inverseBindMatrices;

        for (const auto& request : m_Requests)
        {
            eastl::unordered_map<uint32_t, uint32_t> boneMapping;

            const Scene& scene = m_Scenes[request.SceneIndex];
            cgltf_skin* skin = scene.m_SkeletonIndices[request.SkeletonIndex];
            uint32_t startBoneIndex = uint32_t(bones.size());
            uint32_t boneCount = uint32_t(skin->joints_count);
            uint32_t inverseBindMatricesStartIndex = uint32_t(inverseBindMatrices.size());

            eastl::unordered_map<cgltf_node*, uint32_t> nodeToBoneIndex;

            // This should be the root
            cgltf_node* rootNode = skin->joints[0];
            assert(strcmp(rootNode->name, "Root") == 0);
            assert(!rootNode->has_matrix);
            //assert(!rootNode->has_rotation);
            assert(!rootNode->has_translation);
            assert(!rootNode->has_scale);
            assert(rootNode->children_count == 1);

            bones.reserve(bones.size() + boneCount);

            // add root bone, with -1 as parent
            glm::quat rootRotation = glm::quat::wxyz(-rootNode->rotation[3], rootNode->rotation[0], rootNode->rotation[1], -rootNode->rotation[2]);
            bones.emplace_back(Common::Tempest::Quat(rootRotation.x, rootRotation.y, rootRotation.z, rootRotation.w), Common::Tempest::Vec3(0.0f, 0.0f, 0.0f), uint32_t(-1));
            nodeToBoneIndex.emplace(rootNode, uint32_t(bones.size() - 1));

            {
                auto inverseBindMatricesIndex = eastl::find(skin->joints, skin->joints + skin->joints_count, rootNode) - skin->joints;
                float matrix[16];
                cgltf_accessor_read_float(skin->inverse_bind_matrices, inverseBindMatricesIndex, matrix, 16);
                TRS trsMatrix(glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));
                glm::mat4x4 properMatrix = glm::translate(trsMatrix.Translation) * glm::toMat4(trsMatrix.Rotation) * glm::scale(trsMatrix.Scale);
                inverseBindMatrices.insert(inverseBindMatrices.end(), reinterpret_cast<uint8_t*>(&properMatrix), reinterpret_cast<uint8_t*>(&properMatrix) + 16 * sizeof(float));
            }

            uint32_t currentBonesAdded = 1;

            eastl::deque<eastl::pair<cgltf_node*, uint32_t>> nodeStack;
            nodeStack.emplace_back(rootNode->children[0], uint32_t(bones.size() - 1));
            while (!nodeStack.empty())
            {
                auto nodePair = nodeStack.front();
                nodeStack.pop_front();

                cgltf_node* node = nodePair.first;
                assert(node->has_rotation);
                assert(node->has_translation);

                glm::vec3 translation(node->translation[0], node->translation[1], -node->translation[2]);
                glm::quat rotation = glm::quat::wxyz(-node->rotation[3], node->rotation[0], node->rotation[1], -node->rotation[2]);

                bones.emplace_back(
                   Common::Tempest::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                   Common::Tempest::Vec3(translation.x, translation.y, translation.z),
                   nodePair.second);

                auto inverseBindMatricesIndex = eastl::find(skin->joints, skin->joints + skin->joints_count, node) - skin->joints;
                float matrix[16];
                cgltf_accessor_read_float(skin->inverse_bind_matrices, inverseBindMatricesIndex, matrix, 16);
                TRS trsMatrix(glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));
                glm::mat4x4 properMatrix = glm::translate(trsMatrix.Translation) * glm::toMat4(trsMatrix.Rotation) * glm::scale(trsMatrix.Scale);
                inverseBindMatrices.insert(inverseBindMatrices.end(), reinterpret_cast<uint8_t*>(&properMatrix), reinterpret_cast<uint8_t*>(&properMatrix) + 16 * sizeof(float));

                nodeToBoneIndex.emplace(node, uint32_t(bones.size() - 1));

                for (uint32_t i = 0; i < node->children_count; ++i)
                {
                    nodeStack.emplace_back(node->children[i], bones.size() - 1);
                }
                ++currentBonesAdded;
            }

            for (uint32_t jointIndex = 0; jointIndex < skin->joints_count; ++jointIndex)
            {
                boneMapping[jointIndex] = nodeToBoneIndex[skin->joints[jointIndex]];
            }

            m_CompiledData.SkeletonJointsMapping.push_back(boneMapping);

            assert(currentBonesAdded == boneCount);
            skeletons.emplace_back(startBoneIndex, boneCount, inverseBindMatricesStartIndex);

            struct NodeAnimationChannel
            {
                eastl::vector<float> Times;
                eastl::vector<float> Data;
                bool isLinear;
            };

            struct NodeAnimation
            {
                eastl::array<NodeAnimationChannel, 2> Channels;
            };

            struct Animation
            {
                eastl::string Name;
                float MaxTime;
                eastl::vector<NodeAnimation> NodeAnims;
            };
            eastl::vector<Animation> loadedAnimations;

            uint32_t animCount = uint32_t(scene.m_Data->animations_count);
            for (uint32_t animIndex = 0; animIndex < animCount; ++animIndex)
            {
                cgltf_animation* anim = scene.m_Data->animations + animIndex;
                Animation& newAnimationData = loadedAnimations.emplace_back();
                newAnimationData.Name = anim->name;

                newAnimationData.NodeAnims.resize(boneCount);

                for (uint32_t channelIndex = 0; channelIndex < anim->channels_count; ++channelIndex)
                {
                    cgltf_animation_channel* channel = anim->channels + channelIndex;
                    auto findItr = nodeToBoneIndex.find(channel->target_node);
                    assert(findItr != nodeToBoneIndex.end());

                    NodeAnimation& nodeAnim = newAnimationData.NodeAnims[findItr->second - startBoneIndex];
                    if (channel->target_path == cgltf_animation_path_type_rotation || channel->target_path == cgltf_animation_path_type_translation)
                    {
                        uint32_t channelArrayIndex = channel->target_path == cgltf_animation_path_type_rotation ? 0 : 1;

                        const cgltf_accessor* output = channel->sampler->output;
                        assert(channelArrayIndex == 0 ? output->type == cgltf_type_vec4 : output->type == cgltf_type_vec3);
                        nodeAnim.Channels[channelArrayIndex].Data.resize(output->count * (channelArrayIndex == 0 ? 4 : 3));
                        cgltf_accessor_unpack_floats(output, reinterpret_cast<float*>(nodeAnim.Channels[channelArrayIndex].Data.data()), output->count * (channelArrayIndex == 0 ? 4 : 3));

                        const cgltf_accessor* input = channel->sampler->input;
                        assert(input->type == cgltf_type_scalar);
                        nodeAnim.Channels[channelArrayIndex].Times.resize(input->count);
                        cgltf_accessor_unpack_floats(input, nodeAnim.Channels[channelArrayIndex].Times.data(), input->count);

                        assert(channel->sampler->interpolation != cgltf_interpolation_type_cubic_spline);
                        nodeAnim.Channels[channelArrayIndex].isLinear = channel->sampler->interpolation == cgltf_interpolation_type_linear;
                    }
                }

                newAnimationData.MaxTime = 0.0f;
                for (const auto& nodeAnim : newAnimationData.NodeAnims)
                {
                    newAnimationData.MaxTime = std::max(newAnimationData.MaxTime, nodeAnim.Channels[0].Times.back());
                    newAnimationData.MaxTime = std::max(newAnimationData.MaxTime, nodeAnim.Channels[1].Times.back());
                }
            }

            auto sampleTranslationChannel = [](float time, const NodeAnimationChannel& channel) -> glm::vec3
                {
                    auto index = eastl::lower_bound(channel.Times.begin(), channel.Times.end(), time) - channel.Times.begin();
                    bool isLinear = channel.isLinear;

                    if (index == channel.Times.size() - 1)
                    {
                        isLinear = false;
                    }

                    glm::vec3 firstData(channel.Data[index * 3], channel.Data[index * 3 + 1], channel.Data[index * 3 + 2]);
                    if (isLinear)
                    {
                        glm::vec3 secondData(channel.Data[index * 3 + 3], channel.Data[index * 3 + 4], channel.Data[index * 3 + 5]);
                        float lerpTime = (time - channel.Times[index]) / (channel.Times[index + 1] - channel.Times[index]);
                        return firstData + lerpTime * (secondData - firstData);
                    }
                    else
                    {
                        return firstData;
                    }
                };

            auto sampleRotationChannel = [](float time, const NodeAnimationChannel& channel) -> glm::quat
                {
                    auto index = eastl::lower_bound(channel.Times.begin(), channel.Times.end(), time) - channel.Times.begin();
                    bool isLinear = channel.isLinear;

                    if (index == channel.Times.size() - 1)
                    {
                        isLinear = false;
                    }

                    glm::quat firstData(channel.Data[index * 4 + 3], channel.Data[index * 4], channel.Data[index * 4 + 1], channel.Data[index * 4 + 2]);
                    if (isLinear)
                    {
                        glm::quat secondData(channel.Data[index * 4 + 7], channel.Data[index * 4 + 4], channel.Data[index * 4 + 5], channel.Data[index * 4 + 6]);
                        float lerpTime = (time - channel.Times[index]) / (channel.Times[index + 1] - channel.Times[index]);
                        return glm::slerp(firstData, secondData, lerpTime);
                    }
                    else
                    {
                        return firstData;
                    }
                };

            // Resample animations to our data
            for (const auto& anim : loadedAnimations)
            {
                uint32_t startAnimDataIndex = uint32_t(animationDataArray.size());
                constexpr uint32_t FramesPerSecond = 30;
                constexpr float TimePerFrame = 1.0f / FramesPerSecond;
                const float numFrames = std::ceil(anim.MaxTime / TimePerFrame);

                struct AnimFramePerBone
                {
                    glm::vec3 Translation;
                    glm::quat Rotation;
                };

                eastl::vector<AnimFramePerBone> animFrame;
                animFrame.resize(boneCount);

                assert(boneCount == anim.NodeAnims.size());
                for (uint32_t frameIndex = 0; frameIndex < numFrames; ++frameIndex)
                {
                    const float currentTime = frameIndex * TimePerFrame;
                    for (uint32_t boneIndex = 0; boneIndex < boneCount; ++boneIndex)
                    {
                        auto rot = sampleRotationChannel(currentTime, anim.NodeAnims[boneIndex].Channels[0]);
                        auto translate = sampleTranslationChannel(currentTime, anim.NodeAnims[boneIndex].Channels[1]);
                        animFrame[boneIndex].Rotation = glm::quat(-rot.w, rot.x, rot.y, -rot.z);
                        animFrame[boneIndex].Translation = glm::vec3(translate.x, translate.y, -translate.z);
                    }

                    uint8_t* startDataPtr = reinterpret_cast<uint8_t*>(animFrame.data());
                    uint8_t* endDataPtr = reinterpret_cast<uint8_t*>(animFrame.data() + animFrame.size());
                    animationDataArray.insert(animationDataArray.end(), startDataPtr, endDataPtr);
                }

                animationsArray.emplace_back(uint32_t(skeletons.size()) - 1, numFrames, startAnimDataIndex, uint32_t(animationDataArray.size()) - startAnimDataIndex);
            }
        }

        flatbuffers::FlatBufferBuilder builder(1024 * 1024);
        auto animationData = builder.CreateVector<uint8_t>(animationDataArray.data(), animationDataArray.size());
        auto skeletonsData = builder.CreateVectorOfStructs<Tempest::Definition::Skeleton>(skeletons.data(), skeletons.size());
        auto bonesData = builder.CreateVectorOfStructs<Tempest::Definition::Bone>(bones.data(), bones.size());
        auto animations = builder.CreateVectorOfStructs<Tempest::Definition::Animation>(animationsArray.data(), animationsArray.size());
        auto inverseBindMatricesData = builder.CreateVector<uint8_t>(inverseBindMatrices.data(), inverseBindMatrices.size());

        auto root = Tempest::Definition::CreateAnimationDatabase(
            builder,
            skeletonsData,
            bonesData,
            animations,
            animationData,
            inverseBindMatricesData
        );

        Tempest::Definition::FinishAnimationDatabaseBuffer(builder, root);

        m_CompiledData.CompiledData.resize(builder.GetSize());
        memcpy(m_CompiledData.CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.CompiledData.size());
	}

private:
	eastl::span<const Scene> m_Scenes;
	eastl::vector<SkeletonRequest> m_Requests;
};
