#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <EngineCore.h>
#include <optick.h>

#include <Graphics/RendererCommandList.h>

// TODO: Find a better way than just to include them
#include <Graphics/Features/RectFeature.h>
#include <Graphics/Features/StaticMeshFeature.h>

#include <DataDefinitions/ShaderLibrary_generated.h>

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

	// TODO: Load geometry database from level
	glm::vec3 vertexData[] = {
		{0.0f, -0.5f, 0.0f}, {-0.5f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}
	};

	Dx12::BufferDescription bufferDescription;
	bufferDescription.Size = sizeof(3 * sizeof(glm::vec3));
	bufferDescription.Data = &vertexData;
	m_VertexData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription);
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
		feature->GenerateCommands(data, commandList);
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
}

