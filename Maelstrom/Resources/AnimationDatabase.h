#pragma once

#include "Resource.h"

#include "../GLTFScene.h"

#include <DataDefinitions/AnimationDatabase_generated.h>

struct SkeletonRequest
{
    uint32_t SceneIndex;
    uint32_t SkeletonIndex;
};

struct AnimationDatabaseResource : Resource<eastl::vector<uint8_t>>
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
        for (const auto& request : m_Requests)
        {
            const Scene& scene = m_Scenes[request.SceneIndex];
            cgltf_skin* skin = scene.m_SkeletonIndices[request.SkeletonIndex];
            uint32_t startBoneIndex = uint32_t(bones.size());
            uint32_t boneCount = uint32_t(skin->joints_count);

            // This should be the root
            cgltf_node* rootNode = skin->joints[0];
            assert(strcmp(rootNode->name, "Root") == 0);
            assert(!rootNode->has_matrix);
            assert(!rootNode->has_rotation);
            assert(!rootNode->has_translation);
            assert(!rootNode->has_scale);
            assert(rootNode->children_count == 1);

            bones.reserve(bones.size() + boneCount);

            // add root bone, with -1 as parent
            bones.emplace_back(Common::Tempest::Quat(0.0f, 0.0f, 0.0f, 1.0f), Common::Tempest::Vec3(0.0f, 0.0f, 0.0f), uint32_t(-1));

            uint32_t currentBonesAdded = 1;

            eastl::deque<eastl::pair<cgltf_node*, uint32_t>> nodeStack;
            nodeStack.emplace_back(rootNode->children[0], bones.size() - 1);
            while (!nodeStack.empty())
            {
                auto nodePair = nodeStack.front();
                nodeStack.pop_front();

                cgltf_node* node = nodePair.first;
                assert(node->has_rotation);
                assert(node->has_translation);

                bones.emplace_back(
                   Common::Tempest::Quat(node->rotation[0], -node->rotation[1], -node->rotation[2], node->rotation[3]),
                   Common::Tempest::Vec3(-node->translation[0], node->translation[1], node->translation[2]),
                   nodePair.second);

                for (uint32_t i = 0; i < node->children_count; ++i)
                {
                    nodeStack.emplace_back(node->children[i], bones.size() - 1);
                }
                ++currentBonesAdded;
            }

            assert(currentBonesAdded == boneCount);
            skeletons.emplace_back(startBoneIndex, boneCount);
        }



        flatbuffers::FlatBufferBuilder builder(1024 * 1024);
        auto animationData = builder.CreateVector<uint8_t>((uint8_t*)nullptr, 0);
        auto skeletonsData = builder.CreateVectorOfStructs<Tempest::Definition::Skeleton>(skeletons.data(), skeletons.size());
        auto bonesData = builder.CreateVectorOfStructs<Tempest::Definition::Bone>(bones.data(), bones.size());
        auto animations = builder.CreateVectorOfStructs<Tempest::Definition::Animation>(nullptr, 0);

        auto root = Tempest::Definition::CreateAnimationDatabase(
            builder,
            skeletonsData,
            bonesData,
            animations,
            animationData
        );

        Tempest::Definition::FinishAnimationDatabaseBuffer(builder, root);

        m_CompiledData.resize(builder.GetSize());
        memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
	}

private:
	eastl::span<const Scene> m_Scenes;
	eastl::vector<SkeletonRequest> m_Requests;
};
