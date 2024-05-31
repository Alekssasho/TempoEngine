#pragma once

#include "Resource.h"

#include "../GLTFScene.h"

#include "compressonator.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <DataDefinitions/TextureDatabase_generated.h>

struct TextureRequest
{
	uint32_t TextureIndex;
	Tempest::Definition::ColorSpace ColorSpace;
	Tempest::Definition::TextureFormat TextureFormat;
};

struct TextureCompiledData
{
	eastl::vector<uint8_t> Data;
	Tempest::Definition::TextureData TextureInfo;
};

struct TextureResource : Resource<TextureCompiledData>
{
public:
    TextureResource(const Scene& scene, const TextureRequest& textureRequest)
		: m_Scene(scene)
		, m_TextureRequest(textureRequest)
	{

	}

	void Compile() override
	{
		const cgltf_texture& texture = m_Scene.m_Data->textures[m_TextureRequest.TextureIndex];
		const uint8_t* data = cgltf_buffer_view_data(texture.image->buffer_view);

		int width, height, components;
		uint8_t* decompressedImage = stbi_load_from_memory(data, int(texture.image->buffer_view->size), &width, &height, &components, 0);

		CMP_Texture srcTexture{ 0 };
		srcTexture.dwSize = sizeof(CMP_Texture);
		srcTexture.dwWidth = width;
		srcTexture.dwHeight = height;
		srcTexture.dwPitch = width * components;
		srcTexture.format = components == 3 ? CMP_FORMAT_RGB_888 : CMP_FORMAT_RGBA_8888;
		srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);
		srcTexture.pData = decompressedImage;

		CMP_Texture dstTexture{ 0 };
		dstTexture.dwSize = sizeof(CMP_Texture);
		dstTexture.dwWidth = srcTexture.dwWidth;
		dstTexture.dwHeight = srcTexture.dwHeight;
		dstTexture.dwPitch = 0;
		dstTexture.format = CMP_FORMAT_BC1; // TODO: Support other
		dstTexture.dwDataSize = CMP_CalculateBufferSize(&dstTexture);

		m_CompiledData.Data.resize(dstTexture.dwDataSize);
		dstTexture.pData = m_CompiledData.Data.data();

		CMP_CompressOptions options{ 0 };
		options.dwSize = sizeof(CMP_CompressOptions);
		options.fquality = 1.0f;
		options.bDisableMultiThreading = true;

		CMP_ERROR error = CMP_ConvertTexture(&srcTexture, &dstTexture, &options, nullptr);
		assert(error == CMP_OK);

		stbi_image_free(decompressedImage);

		m_CompiledData.TextureInfo = Tempest::Definition::TextureData(
			width,
			height,
			m_TextureRequest.TextureFormat,
			m_TextureRequest.ColorSpace
		);
	}

private:
    const Scene& m_Scene;
    const TextureRequest& m_TextureRequest;
};
