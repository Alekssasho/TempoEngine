#include <Graphics/Managers/MeshManager.h>

namespace Tempest
{
MeshManager::MeshData MeshManager::GetMeshData(MeshHandle handle) const
{
	auto findItr = m_StaticMeshes.find(handle);
	if(findItr != m_StaticMeshes.end())
	{
		return findItr->second;
	}

	return {};
}

MeshHandle MeshManager::CreateStaticMesh(MeshData data)
{
	MeshHandle handle = m_Handle++;
	m_StaticMeshes.emplace(eastl::make_pair(handle, data));
	return handle;
}
}
