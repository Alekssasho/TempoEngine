#pragma once

#include "Resource.h"
#include "Mesh.h"

#include "../GLTFScene.h"

#include <DataDefinitions/GeometryDatabase_generated.h>

struct GeometryDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	GeometryDatabaseResource(
		const eastl::vector<MeshResource>& meshes,
		const eastl::vector<Tempest::Definition::Material>& materials,
		const eastl::vector<MaterialRequest>& materialRequests,
		const eastl::vector<eastl::unordered_map<uint32_t, uint32_t>>& skeletonBoneMappings
	)
        : m_Meshes(meshes)
		, m_Materials(materials)
		, m_MaterialRequests(materialRequests)
		, m_SkeletonBoneMappings(skeletonBoneMappings)
    {}

	void Compile() override
	{
		eastl::vector<VertexLayout> vertexBuffer;
		eastl::vector<SkeletonVertexLayout> skeletonVertexBuffer;
		eastl::vector<uint8_t> meshletIndicesBuffer;
		eastl::vector<Tempest::Definition::Meshlet> meshlets;
		eastl::vector<Tempest::Definition::PrimitiveMeshData> primitiveMeshes;
		eastl::vector<Tempest::Definition::MeshMapping> mappings;

		uint32_t currentVertexBufferOffset[2] = { 0, 0 };
		uint32_t currentIndicesBufferOffset = 0;
		uint32_t currentMeshletBufferOffset = 0;
		for (uint32_t index = 0; index < m_Meshes.size(); ++index)
		{
			const auto& currentMeshPrimitiveData = m_Meshes[index].GetCompiledData();
			const auto meshType = m_Meshes[index].m_Type;
			const auto isStaticMesh = meshType == Tempest::Definition::MeshType_StaticMesh;
			mappings.emplace_back(index,
				Tempest::Definition::MeshData(
					uint32_t(primitiveMeshes.size()),
					uint32_t(currentMeshPrimitiveData.size())
				),
				meshType
			);

			primitiveMeshes.reserve(primitiveMeshes.size() + currentMeshPrimitiveData.size());
			for (const auto& primitiveMesh : currentMeshPrimitiveData)
			{
				meshlets.reserve(meshlets.size() + primitiveMesh.Meshlets.size());
				for (const auto& meshlet : primitiveMesh.Meshlets)
				{
					meshlets.emplace_back(
						currentVertexBufferOffset[meshType] + meshlet.vertex_offset,
						meshlet.vertex_count,
						currentIndicesBufferOffset + meshlet.triangle_offset,
						meshlet.triangle_count
					);
				}

				if (isStaticMesh)
				{
					vertexBuffer.insert(
						vertexBuffer.end(),
						reinterpret_cast<const VertexLayout*>(primitiveMesh.Vertices.begin()),
						reinterpret_cast<const VertexLayout*>(primitiveMesh.Vertices.end())
					);
				}
				else
				{
					auto verticesArray = reinterpret_cast<const SkeletonVertexLayout*>(primitiveMesh.Vertices.begin());
					auto vertexCount = primitiveMesh.Vertices.size() / sizeof(SkeletonVertexLayout);
					for (uint32_t i = 0; i < vertexCount; ++i)
					{
						SkeletonVertexLayout vertex = verticesArray[i];
                        vertex.Joints[0] = m_SkeletonBoneMappings[0].find(vertex.Joints[0])->second;
                        vertex.Joints[1] = m_SkeletonBoneMappings[0].find(vertex.Joints[1])->second;
                        vertex.Joints[2] = m_SkeletonBoneMappings[0].find(vertex.Joints[2])->second;
                        vertex.Joints[3] = m_SkeletonBoneMappings[0].find(vertex.Joints[3])->second;

						skeletonVertexBuffer.push_back(vertex);
					}
				}

				meshletIndicesBuffer.insert(
					meshletIndicesBuffer.end(),
					primitiveMesh.MeshletIndices.begin(),
					primitiveMesh.MeshletIndices.end()
				);

				// We need to remap material index as we can have multiple scenes
				MaterialRequest findRequest{ m_Meshes[index].m_SceneIndex, primitiveMesh.MaterialIndex };
				const uint32_t remmapedMaterialIndex = uint32_t(eastl::distance(m_MaterialRequests.begin(), eastl::find(m_MaterialRequests.begin(), m_MaterialRequests.end(), findRequest)));

				primitiveMeshes.emplace_back(
					currentMeshletBufferOffset,
					uint32_t(primitiveMesh.Meshlets.size()),
					remmapedMaterialIndex
				);

				currentMeshletBufferOffset += uint32_t(primitiveMesh.Meshlets.size());
				currentVertexBufferOffset[meshType] += uint32_t(primitiveMesh.Vertices.size() / (isStaticMesh ? sizeof(VertexLayout) : sizeof(SkeletonVertexLayout)));
				currentIndicesBufferOffset += uint32_t(primitiveMesh.MeshletIndices.size());
			}
		}

		flatbuffers::FlatBufferBuilder builder(1024 * 1024);
		auto staticMeshVertexBufferOffset = builder.CreateVector<uint8_t>(reinterpret_cast<const uint8_t*>(vertexBuffer.data()), vertexBuffer.size() * sizeof(VertexLayout));
		auto skeletonMeshVertexBufferOffset = builder.CreateVector<uint8_t>(reinterpret_cast<const uint8_t*>(skeletonVertexBuffer.data()), skeletonVertexBuffer.size() * sizeof(SkeletonVertexLayout));
		auto meshletIndicesBufferOffset = builder.CreateVector<uint8_t>(meshletIndicesBuffer.data(), meshletIndicesBuffer.size());
		auto meshletBufferOffset = builder.CreateVectorOfStructs<Tempest::Definition::Meshlet>(meshlets.data(), meshlets.size());
		auto primitiveMeshesOffset = builder.CreateVectorOfStructs<Tempest::Definition::PrimitiveMeshData>(primitiveMeshes.data(), primitiveMeshes.size());
		auto materialsOffset = builder.CreateVectorOfStructs<Tempest::Definition::Material>(m_Materials.data(), m_Materials.size());
		auto mappingsOffset = builder.CreateVectorOfSortedStructs<Tempest::Definition::MeshMapping>(mappings.data(), mappings.size());

		auto root = Tempest::Definition::CreateGeometryDatabase(
			builder,
			staticMeshVertexBufferOffset,
			skeletonMeshVertexBufferOffset,
			meshletIndicesBufferOffset,
			meshletBufferOffset,
			primitiveMeshesOffset,
			materialsOffset,
			mappingsOffset
		);

		Tempest::Definition::FinishGeometryDatabaseBuffer(builder, root);

        m_CompiledData.resize(builder.GetSize());
        memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
	}

private:
	const eastl::vector<MeshResource>& m_Meshes;
	const eastl::vector<Tempest::Definition::Material>& m_Materials;
	const eastl::vector<MaterialRequest>& m_MaterialRequests;
	const eastl::vector<eastl::unordered_map<uint32_t, uint32_t>>& m_SkeletonBoneMappings;
};
