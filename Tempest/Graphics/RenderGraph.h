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
class TemporaryTextureManager;
}

struct BlackboardIdentifier
{
	eastl::string Name; // TODO: change to hash of a string
	bool operator==(const BlackboardIdentifier& other) const {
		return Name == other.Name;
	}
};

class RenderGraphBlackboard : Utils::NonCopyable
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

	// TODO: This could be a Identifier instead of handle ?
	uint32_t GetTextureSlot(RenderGraphResourceHandle handle) const
	{
		auto findItr = m_TextureSlots.find(handle);
		assert(findItr != m_TextureSlots.end());
		return findItr->second;
	}

	uint32_t GetConstantDataOffset(BlackboardIdentifier identifier) const
	{
		auto findItr = m_ConstantDataOffsets.find(identifier);
		assert(findItr != m_ConstantDataOffsets.end());
		return findItr->second;
	}

	RenderPhase GetRenderPhase() const
	{
		return m_RenderPhase;
	}

	void SetConstantDataOffset(BlackboardIdentifier identifier, uint32_t dataOffset)
	{
		m_ConstantDataOffsets[identifier] = dataOffset;
	}

	void SetRenderPhase(RenderPhase phase)
	{
		m_RenderPhase = phase;
	}

	void SetTextureSlot(RenderGraphResourceHandle handle, uint32_t slot)
	{
		m_TextureSlots[handle] = slot;
	}
private:
	Renderer& m_Renderer;
	const FrameData& m_FrameData;
	Dx12::ConstantBufferDataManager& m_ConstantManager;

	struct BlackboardIdentifierHash
	{
		size_t operator()(const BlackboardIdentifier& p) const { eastl::hash<const char*> hash; return hash(p.Name.c_str()); }
	};
	eastl::unordered_map<BlackboardIdentifier, uint32_t, BlackboardIdentifierHash> m_ConstantDataOffsets;
	eastl::unordered_map<RenderGraphResourceHandle, uint32_t> m_TextureSlots;
	RenderPhase m_RenderPhase;
};

struct RenderGraphBuilder
{
	void UseDepthStencil(RenderGraphResourceHandle handle, TextureTargetLoadAction loadAction, TextureTargetStoreAction storeAction)
	{
		StartNewPass = true;
		DepthStencilTarget = { handle, loadAction, storeAction };
		UsedResources.push_back({ handle, ResourceUsage::DepthStencil });
	}

	void UseRenderTarget(RenderGraphResourceHandle handle, TextureTargetLoadAction loadAction, TextureTargetStoreAction storeAction)
	{
		StartNewPass = true;
		RenderTarget = { handle, loadAction, storeAction };
		UsedResources.push_back({ handle, ResourceUsage::RenderTarget });
	}

	void WriteTexture(RenderGraphResourceHandle handle)
	{
		UsedResources.push_back({ handle, ResourceUsage::Write });
	}

	void ReadTexture(RenderGraphResourceHandle handle)
	{
		UsedResources.push_back({ handle, ResourceUsage::Read });
	}
private:
	friend class RenderGraph;
	enum class ResourceUsage
	{
		Write,
		Read,
		RenderTarget,
		DepthStencil
	};

	struct Target
	{
		RenderGraphResourceHandle Handle = 0;
		TextureTargetLoadAction LoadAction;
		TextureTargetStoreAction StoreAction;
	};
	struct UsedResource
	{
		RenderGraphResourceHandle Handle;
		ResourceUsage Usage;
	};

	eastl::vector<UsedResource> UsedResources;
	Target RenderTarget;
	Target DepthStencilTarget;
	bool StartNewPass = false;
};

class RenderGraph : Utils::NonCopyable
{
public:
	RenderGraph(Renderer& renderer, const FrameData& frameData, Dx12::ConstantBufferDataManager& constantManager, Dx12::TemporaryTextureManager& textureManager);

	RenderGraphResourceHandle RequestTexture(const Dx12::TextureDescription& description);

	// TODO: UE4 is taking lambda by template and then moves the data into a linear allocator inside the graph, instead of doing allocations
	template<typename SetupLambda>
	void AddPass(const char* name, SetupLambda setup)
	{
		RenderGraphBuilder builder;
		auto&& compile = setup(builder, m_Blackboard);
		m_Passes.emplace_back(Pass{ name, eastl::move(builder), eastl::move(compile) });
	}

	RendererCommandList Compile();
private:
	Dx12::TemporaryTextureManager& m_TextureManager;

	// TODO: This probably needs to be per thread, in order to support MT compilation step. Probably it will need a base parent blackboard, and per thread one
	RenderGraphBlackboard m_Blackboard;

	struct Pass
	{
		const char* Name;
		RenderGraphBuilder Description;
		eastl::function<void(RendererCommandList&, RenderGraphBlackboard&)> CompileFunction;
	};
	eastl::vector<Pass> m_Passes;

	struct ResourceCurrentState
	{
		TextureHandle Handle;
		ResourceState State;
		uint32_t ViewSlot = uint32_t(-1);
	};
	eastl::unordered_map<RenderGraphResourceHandle, ResourceCurrentState> m_Textures;
	RenderGraphResourceHandle m_NextHandle = sBackbufferDepthStencilRenderGraphHandle + 1;

	TextureHandle ResolveResourceToHandle(RenderGraphResourceHandle handle);
	ResourceState ResolveResourceCurrentState(RenderGraphResourceHandle handle);
	ResourceState ResolveResourceRequiredState(const RenderGraphBuilder::UsedResource& resource);
	bool ResourceNeedsBarrier(const RenderGraphBuilder::UsedResource& resource);
};
}