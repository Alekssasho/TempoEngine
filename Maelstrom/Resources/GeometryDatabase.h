#pragma once

#include "Resource.h"
#include "Texture.h"

#include "../GLTFScene.h"

#include <DataDefinitions/GeometryDatabase_generated.h>

struct GeometryDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	GeometryDatabaseResource(const Scene& scene)
        : m_Scene(scene)
    {}

	void Compile() override
	{
	}

private:
	const Scene& m_Scene;
};