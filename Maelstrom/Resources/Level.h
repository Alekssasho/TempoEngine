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

    eastl::string WriteFile(const char* extension, const eastl::vector<uint8_t>& data)
    {
        std::filesystem::path filename(gCompilerOptions->OutputFolder.c_str());
        filename /= m_Name.c_str();
        filename.replace_extension(extension);

        std::ofstream stream(filename, std::ios::binary);
        stream.write(reinterpret_cast<const char*>(data.data()), data.size());

        return eastl::string(filename.filename().string().c_str());
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
        GeometryDatabaseResource geometryDatabaseResource(scene, meshResources, materialDatabaseResource.GetCompiledData().Materials);
        TextureDatabaseResource textureDatabaseResource(scene, materialDatabaseResource.GetCompiledData().TextureRequests);
        AudioDatabaseResource audioDatabaseResource;

        Tempest::Job::Counter databaseCounter;
        CompileResources(databaseCounter, entitiesDatabaseResource, geometryDatabaseResource, textureDatabaseResource, audioDatabaseResource);

        Tempest::gEngineCore->GetJobSystem().WaitForCounter(&databaseCounter, 0);

        auto geometryDatabaseName = WriteFile(Tempest::Definition::GeometryDatabaseExtension(), geometryDatabaseResource.GetCompiledData());
        auto audioDatabaseName = WriteFile(Tempest::Definition::AudioDatabaseExtension(), audioDatabaseResource.GetCompiledData());
        auto textureDatabaseName = WriteFile(Tempest::Definition::TextureDatabaseExtension(), textureDatabaseResource.GetCompiledData());

        flatbuffers::FlatBufferBuilder builder(1024 * 1024);
        auto nameOffset = builder.CreateString(m_Name.c_str());
        auto entitiesOffset = builder.CreateVector<uint8_t>(entitiesDatabaseResource.GetCompiledData().EcsState.data(), entitiesDatabaseResource.GetCompiledData().EcsState.size());
        auto physicsWorldOffset = 0;
        auto geometryDatabaseFileOffset = builder.CreateString(geometryDatabaseName.c_str());
        auto textureDatabaseFileOffset = builder.CreateString(textureDatabaseName.c_str());
        auto audioDatabaseFileOffset = builder.CreateString(audioDatabaseName.c_str());
        auto root = Tempest::Definition::CreateLevel(
            builder,
            nameOffset,
            entitiesOffset,
            physicsWorldOffset,
            geometryDatabaseFileOffset,
            textureDatabaseFileOffset,
            audioDatabaseFileOffset,
            &scene.m_Camera
        );

        Tempest::Definition::FinishLevelBuffer(builder, root);

        m_CompiledData.resize(builder.GetSize());
        memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
    }

private:
    eastl::string m_Name;
};
