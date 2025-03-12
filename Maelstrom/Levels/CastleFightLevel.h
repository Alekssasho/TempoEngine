#pragma once

#include "../Resources/ScriptedLevel.h"

struct CastleFightLevel : ScriptedLevelResource
{
public:
    CastleFightLevel()
    {
    }

    const char* GetName() override
    {
        return "CastleFight";
    }

    void ConstructScript() override
    {
        // Setup camera

        {
            glm::quat rotation(glm::vec3(-glm::pi<float>() / 4.0f, 0.0f, 0.0f));
            auto rotatedUp = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
            auto rotatedForward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);

            m_Camera = Tempest::Definition::Camera(
                1.0f,
                0.1f,
                100.0f,
                1.77f,
                Common::Tempest::Vec3(8.4f, 6.0f, 7.8f),
                Common::Tempest::Vec3(rotatedForward.x, rotatedForward.y, rotatedForward.z),
                Common::Tempest::Vec3(rotatedUp.x, rotatedUp.y, rotatedUp.z)
            );
        }

        // Add Sun
        {
            m_ECS.m_EntityWorld.entity("Sun")
                .set(Tempest::Components::Transform{ glm::quat(glm::vec3(-glm::pi<float>() / 4.0f, 0.0f, 0.0f)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) })
                .set(Tempest::Components::LightColorInfo{
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    1.0f })
                    .add<Tempest::Tags::DirectionalLight>();
        }

        // Add ground
        {
            const uint32_t meshIndex = AddMeshRequest("car3", "Plane");
            m_ECS.m_EntityWorld.entity("Ground")
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(23.0f, 23.0f, 23.0f)});
        }
    }
};
