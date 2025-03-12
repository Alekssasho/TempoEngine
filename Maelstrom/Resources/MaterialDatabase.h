#pragma once

#include "Resource.h"
#include "Texture.h"

#include "../GLTFScene.h"

#include <DataDefinitions/GeometryDatabase_generated.h>

struct MaterialDatabaseCompiledData
{
	eastl::vector<TextureRequest> TextureRequests;
	eastl::vector<Tempest::Definition::Material> Materials;
};

struct MaterialRequest
{
	uint32_t SceneIndex;
	uint32_t MaterialIndex;

	auto operator<=>(const MaterialRequest&) const = default;
};

struct MaterialDatabaseResource : Resource<MaterialDatabaseCompiledData>
{
public:
    MaterialDatabaseResource(const Scene& scene)
		: m_Scenes(&scene, 1)
    {
		m_MaterialRequests.reserve(scene.m_Materials.size());
		for (uint32_t i = 0; i < scene.m_Materials.size(); ++i)
		{
			m_MaterialRequests.emplace_back(0, i);
		}
    }

	MaterialDatabaseResource(eastl::span<const Scene> scenes, eastl::span<MaterialRequest> requests)
		: m_Scenes(scenes)
	{
		m_MaterialRequests.assign(requests.begin(), requests.end());
	}

	void Compile() override
	{
		eastl::vector<Tempest::Definition::Material> materials;
		eastl::vector<TextureRequest> requests;

		materials.reserve(m_MaterialRequests.size());

		for(const MaterialRequest& request : m_MaterialRequests)
		{
			const Scene& scene = m_Scenes[request.SceneIndex];
			const cgltf_material* material = scene.m_Materials[request.MaterialIndex];
			const cgltf_pbr_metallic_roughness& pbr = material->pbr_metallic_roughness;
			uint32_t baseTextureIndex = eastl::numeric_limits<uint32_t>::max();
			if (pbr.base_color_texture.texture)
			{
				auto textureIndex = pbr.base_color_texture.texture - scene.m_Data->textures;
				requests.push_back(TextureRequest{
					request.SceneIndex,
					uint32_t(textureIndex),
					Tempest::Definition::ColorSpace_sRGB,
					Tempest::Definition::TextureFormat::TextureFormat_BC1_RGB
				});
				baseTextureIndex = uint32_t(requests.size() - 1);
			}

			uint32_t metallicRoughnessTextureIndex = eastl::numeric_limits<uint32_t>::max();
			if (pbr.metallic_roughness_texture.texture)
			{
				auto textureIndex = pbr.metallic_roughness_texture.texture - scene.m_Data->textures;
				requests.push_back(TextureRequest{
					request.SceneIndex,
					uint32_t(textureIndex),
					Tempest::Definition::ColorSpace_Linear,
					Tempest::Definition::TextureFormat::TextureFormat_BC1_RGB
					});
				metallicRoughnessTextureIndex = uint32_t(requests.size() - 1);
			}

			materials.emplace_back(
				Common::Tempest::Color(
					pbr.base_color_factor[0],
					pbr.base_color_factor[1],
					pbr.base_color_factor[2],
					pbr.base_color_factor[3]
				),
				pbr.metallic_factor,
				pbr.roughness_factor,
				baseTextureIndex,
				metallicRoughnessTextureIndex
			);
		}

		m_CompiledData.TextureRequests.swap(requests);
		m_CompiledData.Materials.swap(materials);
	}

	eastl::vector<MaterialRequest> m_MaterialRequests;
private:
	eastl::span<const Scene> m_Scenes;
};
