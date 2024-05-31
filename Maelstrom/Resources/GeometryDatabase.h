#pragma once

#include "Resource.h"
#include "Mesh.h"

#include "../GLTFScene.h"

#include <DataDefinitions/GeometryDatabase_generated.h>

struct GeometryDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	GeometryDatabaseResource(const Scene& scene, const eastl::vector<MeshResource>& meshes, const eastl::vector<Tempest::Definition::Material>& materials)
        : m_Scene(scene)
		, m_Meshes(meshes)
		, m_Materials(materials)
    {}

	void Compile() override
	{
		eastl::vector<VertexLayout> vertexBuffer;
		eastl::vector<uint8_t> meshletIndicesBuffer;
		eastl::vector<Tempest::Definition::Meshlet> meshlets;
		eastl::vector<Tempest::Definition::PrimitiveMeshData> primitiveMeshes;
		eastl::vector<Tempest::Definition::MeshMapping> mappings;

		uint32_t currentVertexBufferOffset = 0;
		uint32_t currentIndicesBufferOffset = 0;
		uint32_t currentMeshletBufferOffset = 0;
		for (uint32_t index = 0; index < m_Meshes.size(); ++index)
		{
			const auto& currentMeshPrimitiveData = m_Meshes[index].GetCompiledData();
			mappings.emplace_back(index,
				Tempest::Definition::MeshData(
					uint32_t(primitiveMeshes.size()),
					uint32_t(currentMeshPrimitiveData.size())
				)
			);

			primitiveMeshes.reserve(primitiveMeshes.size() + currentMeshPrimitiveData.size());
			for (const auto& primitiveMesh : currentMeshPrimitiveData)
			{
				meshlets.reserve(meshlets.size() + primitiveMesh.Meshlets.size());
				for (const auto& meshlet : primitiveMesh.Meshlets)
				{
					meshlets.emplace_back(
						currentVertexBufferOffset + meshlet.vertex_offset,
						meshlet.vertex_count,
						currentIndicesBufferOffset + meshlet.triangle_offset,
						meshlet.triangle_count
					);
				}

				vertexBuffer.insert(
					vertexBuffer.end(),
					primitiveMesh.Vertices.begin(),
					primitiveMesh.Vertices.end()
				);

				meshletIndicesBuffer.insert(
					meshletIndicesBuffer.end(),
					primitiveMesh.MeshletIndices.begin(),
					primitiveMesh.MeshletIndices.end()
				);

				primitiveMeshes.emplace_back(
					currentMeshletBufferOffset,
					uint32_t(primitiveMesh.Meshlets.size()),
					primitiveMesh.MaterialIndex
				);

				currentMeshletBufferOffset += uint32_t(primitiveMesh.Meshlets.size());
				currentVertexBufferOffset += uint32_t(primitiveMesh.Vertices.size());
				currentIndicesBufferOffset += uint32_t(primitiveMesh.MeshletIndices.size());
			}
		}

		flatbuffers::FlatBufferBuilder builder(1024 * 1024);
		auto vertexBufferOffset = builder.CreateVector<uint8_t>(reinterpret_cast<const uint8_t*>(vertexBuffer.data()), vertexBuffer.size() * sizeof(VertexLayout));
		auto meshletIndicesBufferOffset = builder.CreateVector<uint8_t>(meshletIndicesBuffer.data(), meshletIndicesBuffer.size());
		auto meshletBufferOffset = builder.CreateVectorOfStructs<Tempest::Definition::Meshlet>(meshlets.data(), meshlets.size());
		auto primitiveMeshesOffset = builder.CreateVectorOfStructs<Tempest::Definition::PrimitiveMeshData>(primitiveMeshes.data(), primitiveMeshes.size());
		auto materialsOffset = builder.CreateVectorOfStructs<Tempest::Definition::Material>(m_Materials.data(), m_Materials.size());
		auto mappingsOffset = builder.CreateVectorOfSortedStructs<Tempest::Definition::MeshMapping>(mappings.data(), mappings.size());

		auto root = Tempest::Definition::CreateGeometryDatabase(
			builder,
			vertexBufferOffset,
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
	const Scene& m_Scene;
	const eastl::vector<MeshResource>& m_Meshes;
	const eastl::vector<Tempest::Definition::Material>& m_Materials;
};
