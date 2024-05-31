#pragma once

#include "Resource.h"

#include "../GLTFScene.h"

#include <World/World.h>
#include <World/Components/Components.h>

struct EntitiesData
{
	eastl::vector<uint8_t> EcsState;
	eastl::unordered_map<uint32_t, uint32_t> NodeToEntityMap;
};

struct EntitiesDatabaseResource : Resource<EntitiesData>
{
public:
	EntitiesDatabaseResource(const Scene& scene)
        : m_Scene(scene)
    {}

	void Compile() override
	{
		Tempest::WorldStorage ecs;

		// NB: template argument is not used but needed to compile
		m_Scene.WalkRootNodes<bool>([&](const cgltf_data* data, cgltf_node* node, const glm::mat4& transform) {
			if (node->mesh)
			{
				TRS trs(transform);

				uint32_t meshIndex = uint32_t(eastl::distance(m_Scene.m_Meshes.begin(), eastl::find(m_Scene.m_Meshes.begin(), m_Scene.m_Meshes.end(), node->mesh)));
				ecs.m_EntityWorld.entity(node->name)
					.set(Tempest::Components::StaticMesh{ meshIndex })
					.set(Tempest::Components::Transform{ trs.Rotation, trs.Translation, trs.Scale });
			}
			else if (node->light)
			{
				if (node->light->type == cgltf_light_type_directional)
				{
					auto changedShineDirectionWorldTransfrom = transform * glm::scale(glm::vec3(1.0f, 1.0f, -1.0f));
					TRS trs(changedShineDirectionWorldTransfrom);

					ecs.m_EntityWorld.entity(node->name)
						.set(Tempest::Components::Transform{ trs.Rotation, trs.Translation, trs.Scale })
						.set(Tempest::Components::LightColorInfo{
							glm::vec3(node->light->color[0], node->light->color[1], node->light->color[2]),
							node->light->intensity })
						.add<Tempest::Tags::DirectionalLight>();
				}
			}

			return eastl::nullopt;
		});

		auto jsonString = ecs.m_EntityWorld.to_json();
		m_CompiledData.EcsState.resize(jsonString.size());
		memcpy(m_CompiledData.EcsState.data(), jsonString.c_str(), jsonString.size());
	}

private:
	const Scene& m_Scene;
};
