#pragma once

#include "Resource.h"
#include "Texture.h"

#include "../GLTFScene.h"

#include <DataDefinitions/GeometryDatabase_generated.h>

struct TextureDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	TextureDatabaseResource(const Scene& scene, const eastl::vector<TextureRequest>& textureRequests)
		: m_Scene(scene)
		, m_TextureRequests(textureRequests)
	{}

	void Compile() override
	{
		eastl::vector<TextureResource> textures;
		textures.reserve(m_TextureRequests.size());
		for (const TextureRequest& request : m_TextureRequests)
		{
			textures.emplace_back(
				m_Scene,
				request
			);
		}

		Tempest::Job::Counter texturesCounter;
		CompileResourceArray(eastl::span(textures), texturesCounter);

		Tempest::gEngineCore->GetJobSystem().WaitForCounter(&texturesCounter, 0);

		eastl::vector<uint8_t> textureDataBuffer;
		eastl::vector<Tempest::Definition::TextureMapping> mappings;
		size_t sizeToReserve = 0;
		for (const auto& texture : textures)
		{
			sizeToReserve += texture.GetCompiledData().Data.size();
		}

		textureDataBuffer.reserve(sizeToReserve);
		mappings.reserve(textures.size());

		for (uint32_t index = 0; index < textures.size(); ++index)
		{
			const auto& textureData = textures[index].GetCompiledData();
			mappings.emplace_back(
				index,
				uint32_t(textureDataBuffer.size()),
				uint32_t(textureData.Data.size()),
				textureData.TextureInfo
			);

			textureDataBuffer.insert(textureDataBuffer.end(), textureData.Data.begin(), textureData.Data.end());
		}

		flatbuffers::FlatBufferBuilder builder(1024 * 1024);
		auto dataOffset = builder.CreateVector<uint8_t>(textureDataBuffer.data(), textureDataBuffer.size());
		auto mappingOffset = builder.CreateVectorOfSortedStructs<Tempest::Definition::TextureMapping>(mappings.data(), mappings.size());
		auto root = Tempest::Definition::CreateTextureDatabase(builder, dataOffset, mappingOffset);
		Tempest::Definition::FinishTextureDatabaseBuffer(builder, root);

		m_CompiledData.resize(builder.GetSize());
		memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
	}

private:
	const Scene& m_Scene;
	const eastl::vector<TextureRequest>& m_TextureRequests;
};
