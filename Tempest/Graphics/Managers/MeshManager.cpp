#include <CommonIncludes.h>

#include <Graphics/Managers/MeshManager.h>

namespace Tempest
{
eastl::span<const Definition::PrimitiveMeshData> MeshManager::GetMeshData(MeshHandle handle) const
{
	auto findItr = m_StaticMeshes.find(handle);
	if(findItr != m_StaticMeshes.end())
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
		if(m_StaticMeshes.find(handle) != m_StaticMeshes.end())
		{
			LOG(Error, StaticMeshes, "Trying to insert a static mesh which is already registered!");
			assert(false);
		}
		m_StaticMeshes.emplace(eastl::make_pair(handle, meshMapping->mesh_data()));
	}
}
}
