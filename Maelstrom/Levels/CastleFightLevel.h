#pragma once

#include "../Resources/ScriptedLevel.h"

#include "../NavigationBuilder/NavigationBuilder.h"

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

    eastl::vector<uint32_t> GetAnimationStates()
    {
        eastl::vector<uint32_t> result;
        result.resize(uint32_t(Tempest::AnimationState::Count));

        result[uint32_t(Tempest::AnimationState::Idle)] = RequestAnimation("sword and shield idle");
        result[uint32_t(Tempest::AnimationState::Move)] = RequestAnimation("sword and shield walk");
        result[uint32_t(Tempest::AnimationState::Attack)] = RequestAnimation("sword and shield slash");

        return result;
    }

    void AddPrefabsPerFaction(Tempest::CastleFight::Faction faction)
    {
        eastl::string suffix = GetSuffixPerFaction(faction);

        {
            const uint32_t meshIndex = AddMeshRequest("Paladin_Anim", "Paladin_J_Nordstrom_Body", Tempest::Definition::MeshType_SkeletonMesh);
            m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Soldier] = m_ECS.m_EntityWorld.prefab(("Soldier_Prefab" + suffix).c_str())
                .is_a(m_BasePrefabs[Prefabs::Soldier])
                .set(Tempest::Components::SkeletonMesh{ meshIndex })
                .set(Tempest::Components::Faction{ faction })
                .set(Tempest::Components::AnimationController{ Tempest::AnimationState::Idle, 0.0f })
                .set(Tempest::Components::AnimationInfo{ GetAnimationStates() });
        }

        {
            const uint32_t meshIndex = AddMeshRequest("car3", "Cube.001");
            m_PerFactionPrefabs[uint32_t(faction)][Prefabs::Factory] = m_ECS.m_EntityWorld.prefab(("Factory_Prefab" + suffix).c_str())
                .is_a(m_BasePrefabs[Prefabs::Factory])
                .set(Tempest::Components::StaticMesh{ meshIndex })
                .set(Tempest::Components::Faction{ faction })
                .set(Tempest::Components::Presence{ .Radius = 1.0f })
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
            .add<Tempest::Tags::SimpleMovement>()
            .set(Tempest::Components::MovementInfo{ 1.0f })
            .set(Tempest::Components::Presence{ .Radius = 1.0f })
            .set(Tempest::Components::AttackInfo{ .DamageAmount = 1.0f, .Range = 0.5f, .Speed = 1.0f, .AwarenessRadius = 5.0f })
            .emplace_auto_override<Tempest::Components::Transform>()
            .emplace_auto_override<Tempest::Components::Movement>()
            .emplace_auto_override<Tempest::Components::Attacking>()
            .emplace_auto_override<Tempest::Components::Health>(Tempest::Components::Health{ .CurrentHealth = 3.0f, .MaxHealth = 3.0f })
            ;

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
                .set(Tempest::Components::Health{ .CurrentHealth = 10.0f, .MaxHealth = 10.0f })
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
            const auto cameraPos = glm::vec3(0.0f, 20.0f, 20.0f);
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

        // Set Navigation
        NavigationBuilder::NavigationBuilder builder;
        auto& leftLane = builder.NewLine();
        leftLane.AddPoint(glm::vec3(20.0f, 0.0f, 0.0f));
        leftLane.AddPoint(glm::vec3(10.0f, 0.0f, 10.0f));
        leftLane.AddPoint(glm::vec3(-10.0f, 0.0f, 10.0f));
        leftLane.AddPoint(glm::vec3(-20.0f, 0.0f, 0.0f));

        auto& rightLane = builder.NewLine();
        rightLane.AddPoint(glm::vec3(20.0f, 0.0f, 0.0f));
        rightLane.AddPoint(glm::vec3(10.0f, 0.0f, -10.0f));
        rightLane.AddPoint(glm::vec3(-10.0f, 0.0f, -10.0f));
        rightLane.AddPoint(glm::vec3(-20.0f, 0.0f, 0.0f));

        m_ECS.m_EntityWorld.set<Tempest::Components::NavigationData>({});
        builder.Build(*m_ECS.m_EntityWorld.get_mut<Tempest::Components::NavigationData>());
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
