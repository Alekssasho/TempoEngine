#include <CommonIncludes.h>
#include <Graphics/Renderer.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <Engine.h>

#include <Graphics/RendererCommandList.h>
#include <Graphics/FrameData.h>
#include <Graphics/RenderGraph.h>

// TODO: Find a better way than just to include them
#include <Graphics/Features/RectFeature.h>
#include <Graphics/Features/StaticMeshFeature.h>
#include <Graphics/Features/LightsFeature.h>

#include <DataDefinitions/ShaderLibrary_generated.h>
#include <DataDefinitions/GeometryDatabase_generated.h>
#include <DataDefinitions/TextureDatabase_generated.h>
#include <World/Camera.h>

namespace Tempest
{
Renderer::Renderer()
	: m_Backend(new Dx12::Backend)
{
	//m_RenderFeatures.emplace_back(new GraphicsFeature::Rects);
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

	//m_ShadowTexture = m_Backend->Managers.Texture.CreateTexture(Dx12::TextureDescription{
	//		Dx12::TextureType::Texture2D,
	//		DXGI_FORMAT_D32_FLOAT,
	//		2048,
	//		2048,
	//		0,
	//		nullptr
	//	},
	//	D3D12_RESOURCE_STATE_DEPTH_WRITE,
	//	nullptr);
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

// TODO: this will be good to be shared between the shader code and C++ if possible
struct SceneConstantData
{
	glm::mat4x4 ViewProjection;
	glm::mat4x4 LightShadowMatrix;
	glm::vec4 LightDirection;
	glm::vec4 LightColor;
	glm::vec3 CameraWorldPosition;
	uint32_t LightShadowMapIndex;
};

void Renderer::RenderFrame(const FrameData& data)
{
	OPTICK_EVENT();

	RenderGraph graph(*this, data, m_Backend->GetDevice()->GetConstantDataManager(), m_Backend->Managers.TemporaryTexture);

	glm::mat4 shadowMatrix;
	auto projectionMatrix = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 1.0f + 120.0f);
	auto viewMatrix = glm::lookAt(-data.DirectionalLights[0].Direction * 60.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
	shadowMatrix = projectionMatrix * viewMatrix;

	auto shadowTextureId = graph.RequestTexture(Dx12::TextureDescription{
		Dx12::TextureType::Texture2D,
		DXGI_FORMAT_D32_FLOAT,
		2048,
		2048,
		0,
		nullptr
	});

	graph.AddPass("Shadow Directional Light", [shadowTextureId, &shadowMatrix](RenderGraphBuilder& builder, RenderGraphBlackboard& blackboard) {
		builder.UseDepthStencil(shadowTextureId, TextureTargetLoadAction::Clear, TextureTargetStoreAction::Store);

		return [&shadowMatrix](RendererCommandList& commandList, RenderGraphBlackboard& blackboard) {
			const FrameData& data = blackboard.GetFrameData();
			SceneConstantData sceneData {
				shadowMatrix,
				shadowMatrix,
				glm::vec4(data.DirectionalLights[0].Direction, 0.0f),
				glm::vec4(data.DirectionalLights[0].Color, 1.0f),
				blackboard.GetRenderer().m_Views[0]->Position,
				0,
			};

			blackboard.SetConstantDataOffset(BlackboardIdentifier{ "SceneData" }, blackboard.GetConstantDataManager().AddData(sceneData));
			blackboard.SetRenderPhase(RenderPhase::Shadow);

			for (const auto& feature : blackboard.GetRenderer().m_RenderFeatures)
			{
				feature->GenerateCommands(data, commandList, blackboard);
			}
		};
	});

	graph.AddPass("Main Pass", [shadowTextureId, &shadowMatrix](RenderGraphBuilder& builder, RenderGraphBlackboard& blackboard) {
		builder.UseRenderTarget(sBackbufferRenderTargetRenderGraphHandle, TextureTargetLoadAction::Clear, TextureTargetStoreAction::Store);
		builder.UseDepthStencil(sBackbufferDepthStencilRenderGraphHandle, TextureTargetLoadAction::Clear, TextureTargetStoreAction::DoNotCare);
		builder.ReadTexture(shadowTextureId);

		return [&shadowMatrix, shadowTextureId](RendererCommandList& commandList, RenderGraphBlackboard& blackboard) {
			const FrameData& data = blackboard.GetFrameData();
			SceneConstantData sceneData{
				blackboard.GetRenderer().m_Views[0]->GetViewProjection(),
				shadowMatrix,
				glm::vec4(data.DirectionalLights[0].Direction, 0.0f),
				glm::vec4(data.DirectionalLights[0].Color, 1.0f),
				blackboard.GetRenderer().m_Views[0]->Position,
				blackboard.GetTextureSlot(shadowTextureId)
			};
			blackboard.SetConstantDataOffset(BlackboardIdentifier{ "SceneData" }, blackboard.GetConstantDataManager().AddData(sceneData));
			blackboard.SetRenderPhase(RenderPhase::Main);

			for (const auto& feature : blackboard.GetRenderer().m_RenderFeatures)
			{
				feature->GenerateCommands(data, commandList, blackboard);
			}
		};
	});

	auto commandList = graph.Compile();

	m_Backend->RenderFrame(commandList);
}

void Renderer::RegisterView(const Camera* camera)
{
	m_Views.emplace_back(camera);
}

void Renderer::UnregisterView(const Camera* camera)
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
	// Shadow phase does not use Pixel Shaders
	if (description.Phase == RenderPhase::Main)
	{
		desc.PSCode = psShader->code()->Data();
		desc.PSCodeSize = psShader->code()->size();
	}

	if(description.Phase == RenderPhase::Shadow)
	{
		// TODO: When we switch to reverse Z this needs to be reversed as well
		desc.DepthBias = 0.001f;
	}

	return m_Backend->Managers.Pipeline.CreateGraphicsPipeline(desc);
}

struct LoadGeometryStaticFunctionData
{
	Renderer* object;
	const char* databaseName;
};

void Renderer::LoadGeometryAndTextureDatabase(const char* geometryDatabaseName, const char* textureDatabaseName)
{
	// First just load texture database, as we need to determine the descriptor heap size
	// Afterwards start a Job to load the geometry, and continue with loading texture database in current job

	const Definition::TextureDatabase* textureDatabase = gEngine->GetResourceLoader().LoadResource<Definition::TextureDatabase>(textureDatabaseName);
	if (!textureDatabase)
	{
		LOG(Warning, Renderer, "Texture Database is Invalid!");
		return;
	}

	const int numTextures = textureDatabase->mappings()->size();
	m_Backend->GetDevice()->AllocateMainDescriptorHeap(numTextures);

	LoadGeometryStaticFunctionData jobData{
		this,
		geometryDatabaseName
	};
	Job::JobDecl loadGeometryJob {
	[](uint32_t, void* dataPtr) {
		LoadGeometryStaticFunctionData* data = (LoadGeometryStaticFunctionData*)dataPtr;
		data->object->LoadGeometryDatabase(data->databaseName);
		}, &jobData
	};
	Job::Counter counter;
	gEngine->GetJobSystem().RunJobs("Load Geometry Database", &loadGeometryJob, 1, &counter);

	// Now the actual loading of texture database
	if(numTextures == 0)
	{
		// Just wait for the geometry and return
		gEngine->GetJobSystem().WaitForCounter(&counter, 0);
		return;
	}

	Dx12::UploadData uploadData = m_Backend->PrepareUpload(textureDatabase->texture_data_buffer()->size());
	for (uint32_t i = 0; i < textureDatabase->mappings()->size(); ++i)
	{
		const auto& texture = textureDatabase->mappings()->Get(i);
		Dx12::TextureDescription textureDescription;
		textureDescription.Type = Dx12::TextureType::Texture2D;
		textureDescription.Format = Dx12::DxFormatForStorageFromTextureFormat(texture->texture_data());
		textureDescription.Width = texture->texture_data().width();
		textureDescription.Height = texture->texture_data().height();
		textureDescription.Size = texture->texture_buffer_byte_count();
		textureDescription.Data = textureDatabase->texture_data_buffer()->data() + texture->texture_buffer_offset();

		TextureHandle handle = m_Backend->Managers.Texture.CreateTexture(textureDescription, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &uploadData);
		m_Backend->GetDevice()->AddStaticTextureDescriptor(m_Backend->Managers.Texture.GetTexture(handle), Dx12::DxFormatForViewFromTextureFormat(texture->texture_data()), 1, i);
	}

	gEngine->GetJobSystem().WaitForCounter(&counter, 0);
	// TODO: This should happen before the wait, but the method is currently thread unsafe and will fail, due to Fence objects used
	m_Backend->ExecuteUpload(uploadData);
}

void Renderer::LoadGeometryDatabase(const char* geometryDatabaseName)
{
	const Definition::GeometryDatabase* geometryDatabase = gEngine->GetResourceLoader().LoadResource<Definition::GeometryDatabase>(geometryDatabaseName);
	if(!geometryDatabase)
	{
		LOG(Warning, Renderer, "Geometry Database is Invalid!");
		return;
	}

	uint32_t totalGeometrySize = geometryDatabase->vertex_buffer()->size()
		+ geometryDatabase->meshlet_buffer()->size() * sizeof(Definition::Meshlet)
		+ geometryDatabase->meshlet_indices_buffer()->size()
		+ geometryDatabase->materials()->size() * sizeof(Definition::Material);
	Dx12::UploadData uploadData = m_Backend->PrepareUpload(totalGeometrySize);

	// TODO: This should not be here
	{
		Dx12::BufferDescription bufferDescription;
		bufferDescription.Type = Dx12::BufferType::Vertex;
		bufferDescription.Size = geometryDatabase->vertex_buffer()->size();
		bufferDescription.Data = geometryDatabase->vertex_buffer()->data();
		m_VertexData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription, &uploadData);

		// TODO: Add type for vertex layout
		const uint32_t vertexLayoutStride = sizeof(glm::vec3) * 2 + sizeof(glm::vec2);
		m_Backend->GetDevice()->AddStaticBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_VertexData), uint32_t(bufferDescription.Size) / vertexLayoutStride, vertexLayoutStride, Dx12::Dx12Device::ShaderResourceSlot::MeshletVertices);

		Dx12::BufferDescription bufferDescription2;
		bufferDescription2.Type = Dx12::BufferType::Vertex;
		bufferDescription2.Size = geometryDatabase->meshlet_buffer()->size() * sizeof(Definition::Meshlet);
		bufferDescription2.Data = geometryDatabase->meshlet_buffer()->data();
		m_MeshletData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription2, &uploadData);

		m_Backend->GetDevice()->AddStaticBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_MeshletData), geometryDatabase->meshlet_buffer()->size(), sizeof(Definition::Meshlet), Dx12::Dx12Device::ShaderResourceSlot::Meshlets);

		Dx12::BufferDescription bufferDescription3;
		bufferDescription3.Type = Dx12::BufferType::Vertex;
		bufferDescription3.Size = geometryDatabase->meshlet_indices_buffer()->size();
		bufferDescription3.Data = geometryDatabase->meshlet_indices_buffer()->data();
		m_MeshletIndicesData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription3, &uploadData);

		m_Backend->GetDevice()->AddStaticBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_MeshletIndicesData), uint32_t(bufferDescription3.Size), sizeof(uint8_t), Dx12::Dx12Device::ShaderResourceSlot::MeshletIndices);

		Dx12::BufferDescription bufferDescription4;
		bufferDescription4.Type = Dx12::BufferType::Vertex;
		bufferDescription4.Size = geometryDatabase->materials()->size() * sizeof(Definition::Material);
		bufferDescription4.Data = geometryDatabase->materials()->data();
		m_MaterialData = m_Backend->Managers.Buffer.CreateBuffer(bufferDescription4, &uploadData);

		m_Backend->GetDevice()->AddStaticBufferDescriptor(m_Backend->Managers.Buffer.GetBuffer(m_MaterialData), geometryDatabase->materials()->size(), sizeof(Definition::Material), Dx12::Dx12Device::ShaderResourceSlot::Materials);
	}

	m_Backend->ExecuteUpload(uploadData);

	Meshes.LoadFromDatabase(geometryDatabase);
}
}

