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

    void AddPerFactionData(Tempest::CastleFight::Faction faction, glm::vec3 originPoint)
    {
        eastl::string suffix;
        switch (faction)
        {
        case Tempest::CastleFight::Faction::Red:
            suffix = "_Red";
            break;
        case Tempest::CastleFight::Faction::Blue:
            suffix = "_Blue";
            break;
        default:
            assert(false);
        }

        const glm::vec3 factionOriginToWorldOrigin = glm::normalize(-originPoint);

        // Add castle
        {
            const uint32_t meshIndex = AddMeshRequest("car3", "Cube.001");
            m_ECS.m_EntityWorld.entity(("Castle" + suffix).c_str())
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), originPoint, glm::vec3(1.0f, 1.0f, 1.0f) });
        }

        // Soldiers
        {
            const uint32_t meshIndex = AddMeshRequest("Soldier", "Warrior" + suffix);
            m_ECS.m_EntityWorld.entity(("Warrior" + suffix).c_str())
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), originPoint + 2.0f * factionOriginToWorldOrigin, glm::vec3(1.0f, 1.0f, 1.0f) })
                .set(Tempest::Components::Faction{ faction });
        }
    }

    void ConstructScript() override
    {
        // Setup camera
        {
            const auto cameraPos = glm::vec3(20.0f, 20.0f, 0.0f);
            const auto cameraForward = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - cameraPos);
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

        // Add ground
        {
            const uint32_t meshIndex = AddMeshRequest("car3", "Plane");
            m_ECS.m_EntityWorld.entity("Ground")
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(23.0f, 23.0f, 23.0f)});
        }

        AddPerFactionData(Tempest::CastleFight::Faction::Blue, glm::vec3(20.0f, 0.0f, 0.0f));
        AddPerFactionData(Tempest::CastleFight::Faction::Red, glm::vec3(-20.0f, 0.0f, 0.0f));

    }
};
