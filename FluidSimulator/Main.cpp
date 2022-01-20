#include <EngineCore.h>
#include <Graphics/RenderGraph.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>
#include <Graphics/Dx12/Dx12Backend.h>

int main()
{
	using namespace Tempest;

	Tempest::EngineCoreOptions options;
	options.NumWorkerThreads = std::thread::hardware_concurrency();
	//options.NumWorkerThreads = 1;
	options.Width = 1280;
	options.Height = 720;
	// Make real resource folder
	options.ResourceFolder = "../../Tempest/Shaders/";
	//options.LevelToLoad = "Level_village.tlb";
	options.LevelToLoad = "Level_car3.tlb";

	static const int gridSizeX = 100;
	static const int gridSizeY = 100;

	struct FluidSimulatorFullScreen : CustomRenderer {
		void RenderFrame(RenderGraph& graph) override {
			graph.AddPass("Visualize Density", [this](RenderGraphBuilder& builder, RenderGraphBlackboard& blackboard) {
				builder.UseRenderTarget(sBackbufferRenderTargetRenderGraphHandle, TextureTargetLoadAction::Clear, TextureTargetStoreAction::Store);
				builder.ReadBuffer(m_Density);

				auto pipelineHandle = blackboard.GetRenderer().PipelineCache.GetHandle(PipelineStateDescription{
					"VisualizeDensity",
					RenderPhase::Main
				});

				return [pipelineHandle](RendererCommandList& commandList, RenderGraphBlackboard& blackboard) {
					Dx12::ConstantBufferDataManager& constantDataManager = blackboard.GetConstantDataManager();

					struct ConstantData {
						glm::uvec2 size;
					} constantData{
						{100, 100}
					};

					RendererCommandDrawInstanced command{};
					command.Pipeline = pipelineHandle;
					command.ParameterViews[size_t(ShaderParameterType::Scene)].ConstantDataOffset = constantDataManager.AddData(constantData);
					command.VertexCountPerInstance = 4;
					command.InstanceCount = 1;
					commandList.AddCommand(command);
				};
			});
		}

		void InitializeResources(Renderer& renderer) override {
			Dx12::BufferDescription bufferDescription;
			bufferDescription.Type = Dx12::BufferType::UAV;
			bufferDescription.Size = gridSizeX * gridSizeY * sizeof(float);
			bufferDescription.Data = nullptr;
			m_Density = renderer.m_Backend->Managers.Buffer.CreateBuffer(bufferDescription, nullptr);
		}

		BufferHandle m_Density;
	} customRenderer;

	options.Renderer.OverrideRenderer = &customRenderer;

	{
		Tempest::EngineCore engine(options);
		engine.StartEngineLoop();
	}
}