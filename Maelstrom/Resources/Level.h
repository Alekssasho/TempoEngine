#pragma once

#include "Resource.h"
#include "Mesh.h"

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

		for (int meshIndex : scene.m_Meshes)
		{
			meshResources.emplace_back(scene, meshIndex);
		}

		CompileResourceArray(meshResources, meshJobCounter);

		m_CompiledData = eastl::vector<uint8_t>();
	}

private:
	eastl::string m_Name;
};
