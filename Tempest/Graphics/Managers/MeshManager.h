#pragma once

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
	void CreateStaticMesh(MeshHandle handle, MeshData data);
private:
	MeshHandle m_Handle = 0;
	eastl::unordered_map<MeshHandle, MeshData> m_StaticMeshes;
};
}
