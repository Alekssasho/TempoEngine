#pragma once

#include "Resource.h"
#include "Texture.h"

#include "../GLTFScene.h"

#include <DataDefinitions/GeometryDatabase_generated.h>

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
		
	}

private:
	const Scene& m_Scene;
};
