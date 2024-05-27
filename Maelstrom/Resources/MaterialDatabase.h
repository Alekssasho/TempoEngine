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

struct MaterialDatabaseResource : Resource<MaterialDatabaseCompiledData>
{
public:
	MaterialDatabaseResource(const Scene& scene)
        : m_Scene(scene)
    {}

	void Compile() override
	{
		eastl::vector<Tempest::Definition::Material> materials;
		eastl::vector<TextureRequest> requests;

		materials.reserve(m_Scene.m_Materials.size());

		for (const cgltf_material* material : m_Scene.m_Materials)
		{
			const cgltf_pbr_metallic_roughness& pbr = material->pbr_metallic_roughness;
			uint32_t baseTextureIndex = eastl::numeric_limits<uint32_t>::max();
			if (pbr.base_color_texture.texture)
			{
				auto textureIndex = pbr.base_color_texture.texture - m_Scene.m_Data->textures;
				requests.push_back(TextureRequest{
					uint32_t(textureIndex),
					Tempest::Definition::ColorSpace_sRGB,
					Tempest::Definition::TextureFormat::TextureFormat_BC1_RGB
				});
				baseTextureIndex = uint32_t(requests.size() - 1);
			}

			uint32_t metallicRoughnessTextureIndex = eastl::numeric_limits<uint32_t>::max();
			if (pbr.metallic_roughness_texture.texture)
			{
				auto textureIndex = pbr.metallic_roughness_texture.texture - m_Scene.m_Data->textures;
				requests.push_back(TextureRequest{
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

private:
	const Scene& m_Scene;
};
