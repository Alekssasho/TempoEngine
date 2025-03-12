#pragma once

#include "Resource.h"

#include "../GLTFScene.h"

#include "nvtt/nvtt.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <DataDefinitions/TextureDatabase_generated.h>

struct TextureRequest
{
	uint32_t SceneIndex;
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
        uint8_t* decompressedImage = stbi_load_from_memory(data, int(texture.image->buffer_view->size), &width, &height, &components, 4);

		// Convert RGBA -> BGRA
		for (size_t pixel = 0; pixel < (width * height); pixel++)
		{
			eastl::swap(decompressedImage[pixel * 4 + 0], decompressedImage[pixel * 4 + 2]);
		}

		nvtt::InputOptions inputOptions;
		inputOptions.setTextureLayout(nvtt::TextureType_2D, width, height);
		inputOptions.setMipmapData(decompressedImage, width, height);
		inputOptions.setAlphaMode(nvtt::AlphaMode_None);
		inputOptions.setMipmapGeneration(false);

		stbi_image_free(decompressedImage);

		switch (m_TextureRequest.ColorSpace)
		{
		case Tempest::Definition::ColorSpace_Linear:
			inputOptions.setGamma(1.0f, 1.0f);
			break;
		case Tempest::Definition::ColorSpace_sRGB:
			inputOptions.setGamma(2.2f, 2.2f);
			break;
		default:
			assert(false);
			break;
		}

		struct OutputHandler : public nvtt::OutputHandler
		{
			TextureResource& m_Texture;

			OutputHandler(TextureResource& texture)
				: m_Texture(texture)
			{}

			void beginImage(int size, int width, int height, int depth, int face, int miplevel)
			{
				m_Texture.m_CompiledData.Data.resize(size);
			}

			bool writeData(const void* data, int size) override
			{
				memcpy(m_Texture.m_CompiledData.Data.data(), data, size);

				return true;
			}

			void endImage() override
			{
			}
		} handler(*this);

		nvtt::OutputOptions outOptions;
		outOptions.setOutputHandler(&handler);
		outOptions.setOutputHeader(false);

		nvtt::CompressionOptions compressionOptions;
		compressionOptions.setFormat(nvtt::Format_BC1); // TODO: Support other

		nvtt::Compressor compressor;
		compressor.process(inputOptions, compressionOptions, outOptions);

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
