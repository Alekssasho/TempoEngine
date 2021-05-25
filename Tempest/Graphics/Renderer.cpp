#include <CommonIncludes.h>
#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <EngineCore.h>

#include <Graphics/RendererCommandList.h>
#include <Graphics/FrameData.h>

// TODO: Find a better way than just to include them
#include <Graphics/Features/RectFeature.h>
#include <Graphics/Features/StaticMeshFeature.h>
#include <Graphics/Features/LightsFeature.h>

#include <DataDefinitions/ShaderLibrary_generated.h>
#include <DataDefinitions/GeometryDatabase_generated.h>
#include <World/Camera.h>

namespace Tempest
{
Renderer::Renderer()
	: m_Backend(new Dx12::Backend)
{
	m_RenderFeatures.emplace_back(new GraphicsFeature::Rects);
	m_RenderFeatures.emplace_back(new GraphicsFeature::StaticMesh);
	m_RenderFeatures.emplace_back(new GraphicsFeature::Lights);
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
	assert(m_Views.size() == 1);
	frameData.ViewProjection = m_Views[0]->GetViewProjection();

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

	// TODO: more views
	m_Backend->RenderFrame(m_Views[0], data, commandList);
}

void Renderer::RegisterView(Camera* camera)
{
	m_Views.emplace_back(camera);
}

void Renderer::UnregisterView(Camera* camera)
{
	m_Views.erase(eastl::remove(m_Views.begin(), m_Views.end(), camera), m_Views.end());
}

PipelineStateHandle Renderer::RequestPipelineState(const PipelineStateDescription& description)
{
	eastl::string vsName = description.ShaderName;
	vsName += "-VS";
	auto vsShader = m_ShaderLibrary->shaders()->LookupByKey(vsName.c_str());

	eastl::string psName = description.ShaderName;
	psName += "-PS";
	auto psShader = m_ShaderLibrary->shaders()->LookupByKey(psName.c_str());
	assert(psShader);

	eastl::string msName = description.ShaderName;
	msName += "-MS";
	auto msShader = m_ShaderLibrary->shaders()->LookupByKey(msName.c_str());

	Dx12::GraphicsPipelineStateDescription desc;
	if(vsShader)
	{
		desc.VSCode = vsShader->code()->Data();
		desc.VSCodeSize = vsShader->code()->size();
	}
	if(msShader)
	{
		desc.MSCode = msShader->code()->Data();
		desc.MSCodeSize = msShader->code()->size();
	}
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

	// TODO: This should not be here
	{
		Dx12::BufferDescription bufferDescription;
		bufferDescription.Type = Dx12::BufferType::Vertex;
		bufferDescription.Size = geometryDatabase->vertex_buffer()->size();
		bufferDescription.Data = geometryDatabase->vertex_buffer()->data();
		m_VertexData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription);

		m_Backend->GetDevice()->AddBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_VertexData), uint32_t(bufferDescription.Size), Dx12::Dx12Device::ShaderResourceSlot::MeshletVertices);

		Dx12::BufferDescription bufferDescription2;
		bufferDescription2.Type = Dx12::BufferType::Vertex;
		bufferDescription2.Size = geometryDatabase->meshlet_buffer()->size();
		bufferDescription2.Data = geometryDatabase->meshlet_buffer()->data();
		m_MeshletData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription2);

		m_Backend->GetDevice()->AddBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_MeshletData), uint32_t(bufferDescription2.Size), Dx12::Dx12Device::ShaderResourceSlot::Meshlets);

		Dx12::BufferDescription bufferDescription3;
		bufferDescription3.Type = Dx12::BufferType::Vertex;
		bufferDescription3.Size = geometryDatabase->meshlet_indices_buffer()->size();
		bufferDescription3.Data = geometryDatabase->meshlet_indices_buffer()->data();
		m_MeshletIndicesData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription3);

		m_Backend->GetDevice()->AddBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_MeshletIndicesData), uint32_t(bufferDescription3.Size), Dx12::Dx12Device::ShaderResourceSlot::MeshletIndices);
	}


	for(const auto& meshMapping : *geometryDatabase->mappings())
	{
		// All static meshes should have the vertex buffer from the geometry database
		Meshes.CreateStaticMesh(
			MeshHandle(meshMapping->index()),
			{ m_VertexData, meshMapping->meshlets_offset(), meshMapping->meshlets_count() });
	}
}
}

