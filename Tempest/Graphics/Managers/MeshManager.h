#pragma once

#include <EASTL/unordered_map.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
class MeshManager
{
public:
	struct MeshData
	{
		BufferHandle VertexBuffer;
		uint32_t OffsetInVertexBuffer;
		uint32_t VertexCount;
	};

	MeshData GetMeshData(MeshHandle handle) const;
	// TODO: This should probably be loaded from geometry database and not have an API for creation
	MeshHandle CreateStaticMesh(MeshData data);
private:
	MeshHandle m_Handle = 0;
	eastl::unordered_map<MeshHandle, MeshData> m_StaticMeshes;
};
}
