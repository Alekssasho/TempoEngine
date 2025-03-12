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

struct ScriptedLevelResource : Resource<eastl::vector<uint8_t>>
{
public:
    ScriptedLevelResource()
    {
    }

    eastl::string WriteFile(const char* extension, const eastl::vector<uint8_t>& data)
    {
        std::filesystem::path filename(gCompilerOptions->OutputFolder.c_str());
        filename /= GetName();
        filename.replace_extension(extension);

        std::ofstream stream(filename, std::ios::binary);
        stream.write(reinterpret_cast<const char*>(data.data()), data.size());

        return eastl::string(filename.filename().string().c_str());
    }

    void Compile() override
    {
        // First we construct the scene with our script
        ConstructScript();

        // Second we extract all the data that the script requested and bake it
        eastl::vector<Scene> loadedScenes;
        loadedScenes.reserve(m_SceneRequests.size());
        for (const eastl::string& sceneNameRequest : m_SceneRequests)
        {
            std::filesystem::path sceneFileName(gCompilerOptions->InputFolder.c_str());
            sceneFileName /= sceneNameRequest.c_str();
            sceneFileName.replace_extension("gltf");

            loadedScenes.emplace_back(sceneFileName);
        }

        eastl::vector<MeshResource> meshResources;
        Tempest::Job::Counter meshJobCounter;
        meshResources.reserve(m_MeshRequests.size());

        eastl::vector<MaterialRequest> materialRequests;

        for (const MeshRequest& meshRequest : m_MeshRequests)
        {
            const Scene& scene = loadedScenes[meshRequest.SceneIndex];
            const uint32_t meshIndexInsideScene = scene.MeshIndexFromName(meshRequest.MeshName.c_str());
            meshResources.emplace_back(scene, meshRequest.SceneIndex, meshIndexInsideScene);

            const uint32_t primitiveCount = scene.MeshPrimitiveCount(meshIndexInsideScene);
            for (uint32_t i = 0; i < primitiveCount; ++i)
            {
                const uint32_t materialIndexInScene = scene.MeshMaterialIndex(meshIndexInsideScene, i);

                MaterialRequest newRequest{ meshRequest.SceneIndex, materialIndexInScene };
                if (eastl::find(materialRequests.begin(), materialRequests.end(), newRequest) == materialRequests.end())
                {
                    materialRequests.push_back(newRequest);
                }
            }
        }

        CompileResourceArray(eastl::span(meshResources), meshJobCounter);

        MaterialDatabaseResource materialDatabaseResource(loadedScenes, materialRequests);
        auto asd = materialDatabaseResource.GetCompiledData().Materials;
        Tempest::Job::Counter materialDatabaseCounter;
        CompileResources(materialDatabaseCounter, materialDatabaseResource);

        Tempest::gEngineCore->GetJobSystem().WaitForCounter(&meshJobCounter, 0);
        Tempest::gEngineCore->GetJobSystem().WaitForCounter(&materialDatabaseCounter, 0);

        GeometryDatabaseResource geometryDatabaseResource(meshResources, materialDatabaseResource.GetCompiledData().Materials, materialRequests);
        TextureDatabaseResource textureDatabaseResource(loadedScenes, materialDatabaseResource.GetCompiledData().TextureRequests);
        AudioDatabaseResource audioDatabaseResource;

        Tempest::Job::Counter databaseCounter;
        CompileResources(databaseCounter, geometryDatabaseResource, textureDatabaseResource, audioDatabaseResource);

        // Compile ECS state, while other databases are being compiled
        eastl::vector<uint8_t> ecsState;
        auto jsonString = m_ECS.m_EntityWorld.to_json();
        ecsState.resize(jsonString.size());
        memcpy(ecsState.data(), jsonString.c_str(), jsonString.size());

        // Now wait for the databases to finish
        Tempest::gEngineCore->GetJobSystem().WaitForCounter(&databaseCounter, 0);

        // We are ready with all dependenacies so we just write the data
        auto geometryDatabaseName = WriteFile(Tempest::Definition::GeometryDatabaseExtension(), geometryDatabaseResource.GetCompiledData());
        auto audioDatabaseName = WriteFile(Tempest::Definition::AudioDatabaseExtension(), audioDatabaseResource.GetCompiledData());
        auto textureDatabaseName = WriteFile(Tempest::Definition::TextureDatabaseExtension(), textureDatabaseResource.GetCompiledData());

        flatbuffers::FlatBufferBuilder builder(1024 * 1024);
        auto nameOffset = builder.CreateString(/*m_Name.c_str()*/"");
        auto entitiesOffset = builder.CreateVector<uint8_t>(ecsState.data(), ecsState.size());
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
            &m_Camera
        );

        Tempest::Definition::FinishLevelBuffer(builder, root);

        m_CompiledData.resize(builder.GetSize());
        memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
    }

    virtual const char* GetName() = 0;

protected:
    virtual void ConstructScript() = 0;

    struct MeshRequest
    {
        uint32_t SceneIndex;
        eastl::string MeshName;
    };

    Tempest::WorldStorage m_ECS;
    Tempest::Definition::Camera m_Camera;

    eastl::vector<eastl::string> m_SceneRequests;
    eastl::vector<MeshRequest> m_MeshRequests;

    uint32_t AddMeshRequest(eastl::string sceneName, eastl::string meshName)
    {
        // Try to find if the scene is already requested
        auto findItr = eastl::find(m_SceneRequests.begin(), m_SceneRequests.end(), sceneName);
        if (findItr == m_SceneRequests.end())
        {
            m_SceneRequests.push_back(sceneName);
            findItr = &m_SceneRequests.back();
        }

        const uint32_t sceneIndex = uint32_t(eastl::distance(m_SceneRequests.begin(), findItr));

        // Check if we already requested this mesh
        auto findMeshItr = eastl::find_if(m_MeshRequests.begin(), m_MeshRequests.end(), [&sceneIndex, &meshName](const MeshRequest& request) {
            return (request.SceneIndex == sceneIndex && request.MeshName == meshName);
        });

        if (findMeshItr != m_MeshRequests.end())
        {
            return uint32_t(eastl::distance(m_MeshRequests.begin(), findMeshItr));
        }

        m_MeshRequests.emplace_back(sceneIndex, meshName);
        return uint32_t(m_MeshRequests.size() - 1);
    }
};
