#pragma once

#include <Platform/WindowsPlatform.h>
#include <Graphics/RendererTypes.h>
#include <Graphics/Managers/MeshManager.h>

namespace Tempest
{
class World;
class Camera;
struct RenderFeature;
// This is forward declare and used through a pointer to avoid pulling Dx12 headers into rest of the engine
namespace Dx12 { class Backend; struct ConstantBufferDataManager; }
namespace Definition { struct ShaderLibrary; }

struct PipelineStateDescription
{
	const char* ShaderName;
	RenderPhase Phase;
};

class Renderer : Utils::NonCopyable
{
public:
	Renderer();
	~Renderer();
	bool CreateWindowSurface(WindowHandle handle);

	void LoadGeometryAndTextureDatabase(const char* geometryDatabaseName, const char* textureDatabaseName);
	void LoadGeometryDatabase(const char* geometryDatabaseName); // Don't use this directly, go through LoadGeometryAndTextureDatabase
	void InitializeAfterLevelLoad(const World& world);
	FrameData GatherWorldData(const World& world);
	void RenderFrame(const FrameData& data);

	void RegisterView(const Camera* camera);
	void UnregisterView(const Camera* camera);

	// TODO: potentially this could be moved someplace else
	PipelineStateHandle RequestPipelineState(const PipelineStateDescription& description);

	// Managers
	MeshManager Meshes;
	// TODO: Hide this
	eastl::unique_ptr<class Dx12::Backend> m_Backend;
private:
	eastl::vector<eastl::unique_ptr<RenderFeature>> m_RenderFeatures;
	eastl::vector<const Camera*> m_Views;

	const Definition::ShaderLibrary* m_ShaderLibrary;

	BufferHandle m_VertexData;
	BufferHandle m_MeshletData;
	BufferHandle m_MeshletIndicesData;
	BufferHandle m_MaterialData;
};
}

