#pragma once

#include <Graphics/RendererTypes.h>
#include <DataDefinitions/GeometryDatabase_generated.h>

namespace Tempest
{
namespace Definition {
	struct MeshData;
	struct PrimitiveMeshData;
	struct GeometryDatabase;
}

class MeshManager : Utils::NonCopyable
{
public:
	eastl::span<const Definition::PrimitiveMeshData> GetMeshData(MeshHandle handle, Definition::MeshType type) const;
	void LoadFromDatabase(const Definition::GeometryDatabase* database);
private:
	MeshHandle m_Handle = 0;
    eastl::unordered_map<MeshHandle, Definition::MeshData> m_StaticMeshes;
    eastl::unordered_map<MeshHandle, Definition::MeshData> m_SkeletonMeshes;
	eastl::vector<Definition::PrimitiveMeshData> m_PrimitiveMeshes;
};
}
