#include <CommonIncludes.h>

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

void MeshManager::CreateStaticMesh(MeshHandle handle, MeshManager::MeshData data)
{
	if(m_StaticMeshes.find(handle) != m_StaticMeshes.end())
	{
		LOG(Error, StaticMeshes, "Trying to insert a static mesh which is already registered!");
		assert(false);
	}
	m_StaticMeshes.emplace(eastl::make_pair(handle, data));
}
}
