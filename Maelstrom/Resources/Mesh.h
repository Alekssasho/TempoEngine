#pragma once
#include "Resource.h"
#include "../GLTFScene.h"

#include <EASTL/numeric.h>

#include <meshoptimizer.h>

struct VertexLayout
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 UV;
};

struct PrimitiveMeshData
{
	eastl::vector<meshopt_Meshlet> Meshlets;
	eastl::vector<VertexLayout> Vertices;
	eastl::vector<uint8_t> MeshletIndices;
	eastl::vector<uint32_t> WholeMeshIndices;
	eastl::vector<uint32_t> SimplyfiedMeshIndices;
	uint32_t MaterialIndex;
};

struct MeshResource : Resource<eastl::vector<PrimitiveMeshData>>
{
	MeshResource(const Scene& scene, uint32_t sceneIndex, uint32_t meshIndex)
		: m_Scene(scene)
		, m_SceneIndex(sceneIndex)
		, m_MeshIndex(meshIndex)
	{}

	void Compile() override
	{
		auto perPrimitivePositionCounts = m_Scene.MeshPositionCountPerPrimitive(m_MeshIndex);
		auto primitiveCount = m_Scene.MeshPrimitiveCount(m_MeshIndex);

		eastl::vector<PrimitiveMeshData> primitiveMeshes(primitiveCount);
		for (int prim = 0; prim < int(primitiveCount); ++prim)
		{
			eastl::vector<VertexLayout> vertices;
			eastl::vector<uint32_t> indices;
			auto gltfIndicesData = m_Scene.MeshIndices(m_MeshIndex, prim);
			if (gltfIndicesData.has_value())
			{
				indices = eastl::move(gltfIndicesData.value());
			}
			else
			{
				indices.resize(perPrimitivePositionCounts[prim]);
				eastl::iota(indices.begin(), indices.end(), 0);
			}

			assert(indices.size() % 3 == 0);
			for (int i = 0; i < indices.size() / 3; ++i)
			{
				eastl::swap(indices[i * 3 + 1], indices[i * 3 + 2]);
			}

			auto positions = m_Scene.MeshPositions(m_MeshIndex, prim);
			auto normals = m_Scene.MeshNormals(m_MeshIndex, prim);
			auto uvs = m_Scene.MeshUVs(m_MeshIndex, prim);
			assert(positions.size() == normals.size() && normals.size() == uvs.size());

			vertices.resize(positions.size());
			for (int i = 0; i < positions.size(); ++i)
			{
				vertices[i] = VertexLayout{
					.Position = glm::vec3(-positions[i].x, positions[i].y, positions[i].z),
					.Normal = glm::vec3(-normals[i].x, normals[i].y, normals[i].z),
					.UV = uvs[i]
				};
			}

			eastl::vector<uint32_t> remapTable(vertices.size());
			meshopt_generateVertexRemap(remapTable.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(VertexLayout));

			meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remapTable.data());

			meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertices.size(), sizeof(VertexLayout), remapTable.data());

			meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
			meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(VertexLayout));

			const uint32_t maxTriangles = 128;
			const uint32_t maxVertices = 128;
			const size_t meshletCount = meshopt_buildMeshletsBound(indices.size(), maxVertices, maxTriangles);
			eastl::vector<meshopt_Meshlet> meshlets(meshletCount);
			eastl::vector<uint32_t> meshletVertices(meshletCount * maxTriangles);
			eastl::vector<uint8_t> meshletIndices(meshletCount * maxTriangles * 3);
			const size_t actualMeshletCount = meshopt_buildMeshlets(
				meshlets.data(),
				meshletVertices.data(),
				meshletIndices.data(),
				indices.data(), indices.size(),
				reinterpret_cast<const float*>(vertices.data()), vertices.size(), sizeof(VertexLayout),
				maxVertices,
				maxTriangles,
				0.0f
			);
			meshlets.resize(actualMeshletCount);
			meshletVertices.resize(meshlets.back().vertex_offset + meshlets.back().vertex_count);
			meshletIndices.resize(meshlets.back().triangle_offset + (meshlets.back().triangle_count * 3));

			{
				eastl::vector<VertexLayout> orderedVertices;
				orderedVertices.reserve(vertices.size());
				for (uint32_t vertexIndex : meshletVertices)
				{
					orderedVertices.push_back(vertices[vertexIndex]);
				}
				vertices.swap(orderedVertices);
			}

			eastl::vector<uint32_t> wholeMeshIndices;
			wholeMeshIndices.reserve(meshletIndices.size());
			for (const auto& meshlet : meshlets)
			{
				for (uint32_t index = meshlet.triangle_offset; index < (meshlet.triangle_offset + meshlet.triangle_count * 3); ++index)
				{
					wholeMeshIndices.push_back(meshlet.vertex_offset + meshletIndices[index]);
				}
			}

			eastl::vector<uint32_t> simplifiedIndices(wholeMeshIndices.size());
			const auto simplifiedIndicesCount = meshopt_simplifySloppy(
				simplifiedIndices.data(),
				wholeMeshIndices.data(),
				wholeMeshIndices.size(),
				reinterpret_cast<const float*>(vertices.data()),
				vertices.size(),
				sizeof(VertexLayout),
				std::min(size_t(256), wholeMeshIndices.size()),
				1.0,
				nullptr
			);
			simplifiedIndices.resize(simplifiedIndicesCount);

			primitiveMeshes[prim].Meshlets.swap(meshlets);
            primitiveMeshes[prim].Vertices.swap(vertices);
            primitiveMeshes[prim].MeshletIndices.swap(meshletIndices);
            primitiveMeshes[prim].WholeMeshIndices.swap(wholeMeshIndices);
            primitiveMeshes[prim].SimplyfiedMeshIndices.swap(simplifiedIndices);
			primitiveMeshes[prim].MaterialIndex = m_Scene.MeshMaterialIndex(m_MeshIndex, prim);
		}

		m_CompiledData.swap(primitiveMeshes);
	}

	const Scene& m_Scene;
	uint32_t m_SceneIndex; // This is needed for later remapping of materials
	uint32_t m_MeshIndex;
};