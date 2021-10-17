#pragma once

#include <Graphics/RendererTypes.h>
#include <Graphics/RendererCommandList.h>
#include <EASTL/functional.h>

namespace Tempest
{
class Renderer;
// TODO: This should not be Dx12 specific
namespace Dx12
{
struct ConstantBufferDataManager;
struct TextureDescription;
}

struct BlackboardIdentifier
{
	eastl::string Name; // TODO: change to hash of a string
};

class RenderGraphBlackboard
{
public:
	RenderGraphBlackboard(Renderer& renderer, const FrameData& frameData, Dx12::ConstantBufferDataManager& constantManager)
		: m_Renderer(renderer)
		, m_FrameData(frameData)
		, m_ConstantManager(constantManager)
	{}

	const Renderer& GetRenderer() const
	{
		return m_Renderer;
	}

	const FrameData& GetFrameData() const
	{
		return m_FrameData;
	}

	Dx12::ConstantBufferDataManager& GetConstantDataManager() const
	{
		return m_ConstantManager;
	}

	RenderGraphResourceHandle GetResource(BlackboardIdentifier identifier) const;
	uint32_t GetTextureSlot(RenderGraphResourceHandle handle) const;
	uint32_t GetConstantDataOffset(BlackboardIdentifier identifier) const;
	RenderPhase GetRenderPhase() const;

	void SetConstantDataOffset(BlackboardIdentifier identifier, uint32_t dataOffset);
	void SetRenderPhase(RenderPhase phase);
private:
	Renderer& m_Renderer;
	const FrameData& m_FrameData;
	Dx12::ConstantBufferDataManager& m_ConstantManager;
};

struct RenderGraphBuilder
{
	void UseDepthStencil(RenderGraphResourceHandle handle, TextureTargetLoadAction loadAction, TextureTargetStoreAction storeAction)
	{
	}

	void UseRenderTarget(RenderGraphResourceHandle handle, TextureTargetLoadAction loadAction, TextureTargetStoreAction storeAction)
	{
	}

	void WriteTexture(RenderGraphResourceHandle handle);
	void ReadTexture(RenderGraphResourceHandle handle);
};

class RenderGraph
{
public:
	RenderGraph(Renderer& renderer, const FrameData& frameData, Dx12::ConstantBufferDataManager& constantManager)
		: m_Blackboard(renderer, frameData, constantManager)
	{}

	RenderGraphResourceHandle AllocateTexture(const Dx12::TextureDescription& description);

	// TODO: UE4 is taking lambda by template and then moves the data into a linear allocator inside the graph, instead of doing allocations
	template<typename SetupLambda>
	void AddPass(const char* name, SetupLambda setup)
	{
		RenderGraphBuilder builder;
		auto&& compile = setup(builder, m_Blackboard);
		m_Passes.emplace_back(Pass{ name, eastl::move(compile) });
	}

	RendererCommandList Compile();
private:
	// TODO: This probably needs to be per thread, in order to support MT compilation step. Probably it will need a base parent blackboard, and per thread one
	RenderGraphBlackboard m_Blackboard;

	struct Pass
	{
		const char* Name;
		eastl::function<void(RendererCommandList&, const FrameData&, const Renderer&)> CompileFunction;
	};
	eastl::vector<Pass> m_Passes;

	eastl::unordered_map<RenderGraphResourceHandle, TextureHandle> m_AllocatedTextures;
	RenderGraphResourceHandle m_NextHandle = sBackbufferDepthStencilRenderGraphHandle + 1;
};


}