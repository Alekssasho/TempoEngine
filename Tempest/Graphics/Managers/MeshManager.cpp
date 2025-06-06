#include <CommonIncludes.h>

#include <Graphics/Managers/MeshManager.h>

namespace Tempest
{
eastl::span<const Definition::PrimitiveMeshData> MeshManager::GetMeshData(MeshHandle handle, Definition::MeshType type) const
{
	auto map = type == Definition::MeshType_StaticMesh ? &m_StaticMeshes : &m_SkeletonMeshes;

	auto findItr = map->find(handle);
	if(findItr != map->end())
	{
		return eastl::span<const Definition::PrimitiveMeshData>(
			&m_PrimitiveMeshes[findItr->second.primitive_mesh_offset()],
			findItr->second.primitive_mesh_count()
		);
	}

	return {};
}

void MeshManager::LoadFromDatabase(const Definition::GeometryDatabase* database)
{
	m_PrimitiveMeshes.reserve(database->primitive_meshes()->size());
	for(const auto& primitiveMesh : *database->primitive_meshes())
	{
		m_PrimitiveMeshes.push_back(*primitiveMesh);
	}

	for (const auto& meshMapping : *database->mappings())
	{
		// All static meshes should have the vertex buffer from the geometry database
		MeshHandle handle(meshMapping->index());
		auto map = meshMapping->mesh_type() == Definition::MeshType_StaticMesh ? &m_StaticMeshes : &m_SkeletonMeshes;

		if(map->find(handle) != map->end())
		{
			LOG(Error, Meshes, "Trying to insert a mesh which is already registered!");
			assert(false);
		}
		map->emplace(eastl::make_pair(handle, meshMapping->mesh_data()));
	}
}
}
