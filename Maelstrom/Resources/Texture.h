#pragma once

#include "Resource.h"

#include "../GLTFScene.h"

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
	TextureResource()
	{

	}

	void Compile() override
	{
	}

private:
};
