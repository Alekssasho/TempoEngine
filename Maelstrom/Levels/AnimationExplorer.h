#pragma once

#include "../Resources/ScriptedLevel.h"

struct AnimationExplorerLevel : ScriptedLevelResource
{
public:
    AnimationExplorerLevel()
    {
    }

    const char* GetName() override
    {
        return "AnimationExplorer";
    }

    void ConstructScript() override
    {
        // Setup camera
        {
            const auto cameraPos = glm::vec3(0.0f, 1.0f, -2.0f);
            const auto cameraForward = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - cameraPos);
            const auto cameraUp = glm::cross(cameraForward, glm::normalize(glm::cross(sUpDirection, cameraForward)));

            m_Camera = Tempest::Definition::Camera(
                1.0f,
                0.1f,
                100.0f,
                1.77f,
                Common::Tempest::Vec3(cameraPos.x, cameraPos.y, cameraPos.z),
                Common::Tempest::Vec3(cameraForward.x, cameraForward.y, cameraForward.z),
                Common::Tempest::Vec3(cameraUp.x, cameraUp.y, cameraUp.z)
            );
        }

        // Add Sun
        {
            m_ECS.m_EntityWorld.entity("Sun")
                .set(Tempest::Components::Transform{ glm::quat(glm::vec3(glm::pi<float>() / 4.0f, 0.0f, 0.0f)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) })
                .set(Tempest::Components::LightColorInfo{
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    1.0f })
                    .add<Tempest::Tags::DirectionalLight>();
        }

        const uint32_t meshIndex = AddMeshRequest("Paladin_Anim", "Paladin_J_Nordstrom_Body", Tempest::Definition::MeshType_SkeletonMesh);
        m_ECS.m_EntityWorld.entity("Paladin")
            .set(Tempest::Components::SkeletonMesh{ meshIndex })
            .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)});
    }

private:
};
