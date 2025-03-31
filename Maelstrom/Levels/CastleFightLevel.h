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

    const char* GetSuffixPerFaction(Tempest::CastleFight::Faction faction)
    {
        switch (faction)
        {
        case Tempest::CastleFight::Faction::Red:
            return "_Red";
        case Tempest::CastleFight::Faction::Blue:
            return "_Blue";
        default:
            assert(false);
        }
        return "";
    }

    void AddPrefabsPerFaction(Tempest::CastleFight::Faction faction)
    {
        eastl::string suffix = GetSuffixPerFaction(faction);

        {
            const uint32_t meshIndex = AddMeshRequest("Soldier", "Warrior" + suffix);
            m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Soldier] = m_ECS.m_EntityWorld.prefab(("Soldier_Prefab" + suffix).c_str())
                .is_a(m_BasePrefabs[Prefabs::Soldier])
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Faction{ faction });
        }

        {
            const uint32_t meshIndex = AddMeshRequest("car3", "Cube.001");
            m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Factory] = m_ECS.m_EntityWorld.prefab(("Factory_Prefab" + suffix).c_str())
                .is_a(m_BasePrefabs[Prefabs::Factory])
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Faction{ faction })
                .set(Tempest::Components::Factory{
                        .PrefabToSpawn = m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Soldier],
                        .TimeToSpawn = 5.0f,
                        .CurrentTime = 0.0f,
                    });
        }
    }

    void AddPrefabs()
    {
        m_BasePrefabs[Prefabs::Factory] = m_ECS.m_EntityWorld.prefab("Factory_Prefab_Base");
        m_BasePrefabs[Prefabs::Soldier] = m_ECS.m_EntityWorld.prefab("Soldier_Prefab_Base")
            .add<Tempest::Tags::SimpleMovement>();

        AddPrefabsPerFaction(Tempest::CastleFight::Faction::Blue);
        AddPrefabsPerFaction(Tempest::CastleFight::Faction::Red);
    }

    void AddInitialEntitiesPerFaction(Tempest::CastleFight::Faction faction, glm::vec3 originPoint)
    {
        eastl::string suffix = GetSuffixPerFaction(faction);

        const glm::vec3 factionOriginToWorldOrigin = glm::normalize(-originPoint);

        // Add castle
        {
            flecs::entity castleId = m_ECS.m_EntityWorld.entity(("Castle" + suffix).c_str())
                .is_a(m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Factory])
                .add<Tempest::Tags::Castle>()
                .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), originPoint, glm::vec3(1.0f, 1.0f, 1.0f) });
            m_CastleManager.Castles[(uint32_t(faction) + 1) % uint32_t(Tempest::CastleFight::Faction::Count)] = castleId;
        }

        //// Soldiers
        //{
        //    m_ECS.m_EntityWorld.entity(("Soldier" + suffix).c_str())
        //        .is_a(m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Soldier])
        //        .set(Tempest::Components::Transform{ glm::identity<glm::quat>(), originPoint + 2.0f * factionOriginToWorldOrigin, glm::vec3(1.0f, 1.0f, 1.0f) });
        //}
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

        AddPrefabs();

        m_CastleManager.Castles.resize(uint32_t(Tempest::CastleFight::Faction::Count));

        AddInitialEntitiesPerFaction(Tempest::CastleFight::Faction::Blue, glm::vec3(20.0f, 0.0f, 0.0f));
        AddInitialEntitiesPerFaction(Tempest::CastleFight::Faction::Red, glm::vec3(-20.0f, 0.0f, 0.0f));

        m_ECS.m_EntityWorld.set<Tempest::Components::CastleManager>(m_CastleManager);
    }

private:
    enum Prefabs
    {
        Factory,
        Soldier,
        Count
    };

    eastl::array<flecs::entity, Prefabs::Count> m_BasePrefabs;
    eastl::array<eastl::array<flecs::entity, Prefabs::Count>, uint32_t(Tempest::CastleFight::Faction::Count)> m_PerFactionPrefabs;

    Tempest::Components::CastleManager m_CastleManager;
};
