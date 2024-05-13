#pragma once
#include "Resource.h"
#include "../GLTFScene.h"

struct VertexLayout
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 UV;
};

struct PrimitiveMeshData
{
	eastl::vector<int> Meshlets;
	eastl::vector<VertexLayout> Vertices;
	eastl::vector<uint8_t> MeshletIndices;
	eastl::vector<uint32_t> WholeMeshIndices;
	eastl::vector<uint32_t> SimplyfiedMeshIndices;
	uint32_t MaterialIndex;
};

struct MeshResource : Resource<eastl::vector<PrimitiveMeshData>>
{
	MeshResource(const Scene& scene, int meshIndex)
		: m_Scene(scene), m_MeshIndex(meshIndex)
	{}

	void Compile() override
	{
		m_CompiledData = eastl::vector<PrimitiveMeshData>();
	}

	const Scene& m_Scene;
	int m_MeshIndex;
};