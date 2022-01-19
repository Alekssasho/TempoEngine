#include <EngineCore.h>
#include <Graphics/RenderGraph.h>
#include <Graphics/Dx12/Managers/ConstantBufferDataManager.h>

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

	options.Renderer.OverrideRenderGraph = [](RenderGraph& graph) {
		graph.AddPass("Visualize Density", [](RenderGraphBuilder& builder, RenderGraphBlackboard& blackboard) {
			builder.UseRenderTarget(sBackbufferRenderTargetRenderGraphHandle, TextureTargetLoadAction::Clear, TextureTargetStoreAction::Store);

			auto pipelineHandle = blackboard.GetRenderer().PipelineCache.GetHandle(PipelineStateDescription{
				"VisualizeDensity",
				RenderPhase::Main
			});

			return [pipelineHandle](RendererCommandList& commandList, RenderGraphBlackboard& blackboard) {
				Dx12::ConstantBufferDataManager& constantDataManager = blackboard.GetConstantDataManager();

				struct ConstantData {
					glm::uvec2 size;
				} constantData {
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
	};

	{
		Tempest::EngineCore engine(options);
		engine.StartEngineLoop();
	}
}