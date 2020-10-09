#pragma once

#include <Defines.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <Platform/WindowsPlatform.h>
#include <Graphics/RendererTypes.h>
#include <Graphics/Managers/MeshManager.h>

namespace Tempest
{
class World;
struct RenderFeature;
// This is forward declare and used through a pointer to avoid pulling Dx12 headers into rest of the engine
namespace Dx12 { class Backend; }
namespace Definition { struct ShaderLibrary; }

struct PipelineStateDescription
{
	const char* ShaderName;
};

class Renderer
{
public:
	Renderer();
	~Renderer();
	bool CreateWindowSurface(WindowHandle handle);

	void LoadGeometryDatabase(const char* geometryDatabaseName);
	void InitializeAfterLevelLoad(const World& world);
	FrameData GatherWorldData(const World& world);
	void RenderFrame(const FrameData& data);

	void RegisterView();

	// TODO: potentially this could be moved someplace else
	PipelineStateHandle RequestPipelineState(const PipelineStateDescription& description);

	// Managers
	MeshManager Meshes;
private:
	eastl::unique_ptr<class Dx12::Backend> m_Backend;
	eastl::vector<eastl::unique_ptr<RenderFeature>> m_RenderFeatures;

	const Definition::ShaderLibrary* m_ShaderLibrary;

	BufferHandle m_VertexData;
};
}

