#pragma once

#include "Resource.h"
#include "Mesh.h"
#include "MaterialDatabase.h"
#include "GeometryDatabase.h"
#include "TextureDatabase.h"
#include "EntitiesDatabase.h"
#include "AudioDatabase.h"

#include "../GLTFScene.h"

#include <DataDefinitions/Level_generated.h>

struct LevelResource : Resource<eastl::vector<uint8_t>>
{
public:
	LevelResource(const eastl::string& name)
		: m_Name(name)
	{

	}

	void Compile() override
	{
		std::filesystem::path sceneFileName(gCompilerOptions->InputFolder.c_str());
		sceneFileName /= m_Name.c_str();
		sceneFileName.replace_extension("gltf");
		Scene scene(sceneFileName);

		eastl::vector<MeshResource> meshResources;
		Tempest::Job::Counter meshJobCounter;
		meshResources.reserve(scene.m_Meshes.size());

		for (int meshIndex = 0; meshIndex < scene.m_Meshes.size(); ++meshIndex)
		{
			meshResources.emplace_back(scene, meshIndex);
		}

		CompileResourceArray(eastl::span(meshResources), meshJobCounter);

        MaterialDatabaseResource materialDatabaseResource(scene);
        Tempest::Job::Counter materialDatabaseCounter;
        CompileResources(materialDatabaseCounter, materialDatabaseResource);

		Tempest::gEngineCore->GetJobSystem().WaitForCounter(&meshJobCounter, 0);
		Tempest::gEngineCore->GetJobSystem().WaitForCounter(&materialDatabaseCounter, 0);

		EntitiesDatabaseResource entitiesDatabaseResource(scene);
		GeometryDatabaseResource geometryDatabaseResource(scene/*, meshResources, materialDatabaseResource.GetCompiledData().Materials*/);
		TextureDatabaseResource textureDatabaseResource(scene/*, materialDatabaseResource.GetCompiledData().TextureRequests*/);
		AudioDatabaseResource audioDatabaseResource;

		Tempest::Job::Counter databaseCounter;
		CompileResources(databaseCounter, entitiesDatabaseResource, geometryDatabaseResource, textureDatabaseResource, audioDatabaseResource);

		Tempest::gEngineCore->GetJobSystem().WaitForCounter(&databaseCounter, 0);

		m_CompiledData = eastl::vector<uint8_t>();
	}

private:
	eastl::string m_Name;
};
