#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <EngineCore.h>
#include <optick.h>

#include <Graphics/RendererCommandList.h>
#include <Graphics/FrameData.h>

// TODO: Find a better way than just to include them
#include <Graphics/Features/RectFeature.h>
#include <Graphics/Features/StaticMeshFeature.h>

#include <DataDefinitions/ShaderLibrary_generated.h>
#include <DataDefinitions/GeometryDatabase_generated.h>

namespace Tempest
{
Renderer::Renderer()
	: m_Backend(new Dx12::Backend)
{
	m_RenderFeatures.emplace_back(new GraphicsFeature::Rects);
	m_RenderFeatures.emplace_back(new GraphicsFeature::StaticMesh);
}

Renderer::~Renderer()
{
}

void Renderer::InitializeAfterLevelLoad(const World& world)
{
	OPTICK_EVENT();
	for (const auto& feature : m_RenderFeatures)
	{
		feature->Initialize(world, *this);
	}
}

bool Renderer::CreateWindowSurface(WindowHandle handle)
{
	m_Backend->Initialize(handle);

	m_ShaderLibrary = gEngine->GetResourceLoader().LoadResource<Definition::ShaderLibrary>("ShaderLibrary.tslb");

#ifdef _DEBUG
	// Debug code so you can inspect what shaders are there
	auto shaders = m_ShaderLibrary->shaders();
	eastl::vector<const char*> shaderNames;
	for (auto sh : *shaders) {
		shaderNames.push_back(sh->name()->c_str());
	}
#endif
	return true;
}

FrameData Renderer::GatherWorldData(const World& world)
{
	OPTICK_EVENT();
	// TODO: No need to return it.
	FrameData frameData;
	for (const auto& feature : m_RenderFeatures)
	{
		feature->GatherData(world, frameData);
	}
	return frameData;
}

void Renderer::RenderFrame(const FrameData& data)
{
	OPTICK_EVENT();

	RendererCommandList commandList;
	for (const auto& feature : m_RenderFeatures)
	{
		feature->GenerateCommands(data, commandList, *this);
	}

	m_Backend->RenderFrame(commandList);
}

PipelineStateHandle Renderer::RequestPipelineState(const PipelineStateDescription& description)
{
	eastl::string vsName = description.ShaderName;
	vsName += "-VS";
	auto vsShader = m_ShaderLibrary->shaders()->LookupByKey(vsName.c_str());
	assert(vsShader);

	eastl::string psName = description.ShaderName;
	psName += "-PS";
	auto psShader = m_ShaderLibrary->shaders()->LookupByKey(psName.c_str());
	assert(psShader);

	Dx12::GraphicsPipelineStateDescription desc;
	desc.VSCode = vsShader->code()->Data();
	desc.VSCodeSize = vsShader->code()->size();
	desc.PSCode = psShader->code()->Data();
	desc.PSCodeSize = psShader->code()->size();

	return m_Backend->Managers.Pipeline.CreateGraphicsPipeline(desc);
}

void Renderer::LoadGeometryDatabase(const char* geometryDatabaseName)
{
	const Definition::GeometryDatabase* geometryDatabase = gEngine->GetResourceLoader().LoadResource<Definition::GeometryDatabase>(geometryDatabaseName);
	if(!geometryDatabase)
	{
		LOG(Warning, Renderer, "Geometry Database is Invalid!");
		return;
	}

	Dx12::BufferDescription bufferDescription;
	bufferDescription.Size = geometryDatabase->vertex_buffer()->size();
	bufferDescription.Data = geometryDatabase->vertex_buffer()->data();
	m_VertexData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription);

	// TODO: This should not be here
	m_Backend->GetDevice()->AddBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_VertexData), uint32_t(bufferDescription.Size));

	for(const auto& meshMapping : *geometryDatabase->mappings())
	{
		// All static meshes should have the vertex buffer from the geometry database
		Meshes.CreateStaticMesh(
			MeshHandle(meshMapping->index()),
			{ m_VertexData, meshMapping->vertex_offset(), meshMapping->vertex_count() });
	}
}
}

