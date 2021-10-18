#include <CommonIncludes.h>
#include <Graphics/RenderGraph.h>
#include <Graphics/Dx12/Managers/TextureManager.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <Graphics/Renderer.h>

namespace Tempest
{
// TODO: This should not be here
static ResourceState StateFromDx12State(D3D12_RESOURCE_STATES state)
{
	switch (state)
	{
	case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE: return ResourceState::PixelShaderRead;
	case D3D12_RESOURCE_STATE_DEPTH_WRITE: return ResourceState::DepthWrite;
	case D3D12_RESOURCE_STATE_COMMON: return ResourceState::Common;
	default:
		assert(false);
		return ResourceState::Common;
	}
}


RenderGraph::RenderGraph(Renderer& renderer, const FrameData& frameData, Dx12::ConstantBufferDataManager& constantManager, Dx12::TemporaryTextureManager& textureManager)
	: m_TextureManager(textureManager)
	, m_Blackboard(renderer, frameData, constantManager)
{
	// For now we don't have explicit handle for the backbuffer, so just use random value
	m_Textures[sBackbufferRenderTargetRenderGraphHandle] = { TextureHandle(-2), ResourceState::RenderTarget };
	m_Textures[sBackbufferDepthStencilRenderGraphHandle] = { TextureHandle(-2), ResourceState::DepthWrite };
}

RenderGraphResourceHandle RenderGraph::RequestTexture(const Dx12::TextureDescription& description)
{
	RenderGraphResourceHandle result = m_NextHandle;
	m_NextHandle++;

	auto&&[handle, state] = m_TextureManager.RequestTexture(description);

	m_Textures[result] = { handle, StateFromDx12State(state) };
	return result;
}

RendererCommandList RenderGraph::Compile()
{
	// TODO: Make Multi Threaded using the Job System (per pass for example)
	RendererCommandList commandList;

	bool passIsStarted = false;
	for (Pass& pass : m_Passes)
	{
		if (passIsStarted && pass.Description.StartNewPass)
		{
			RendererCommandEndRenderPass endRenderPassCommand;
			commandList.AddCommand(endRenderPassCommand);
		}

		for (auto& resource : pass.Description.UsedResources) {
			auto textureHandle = ResolveResourceToHandle(resource.Handle);
			if (ResourceNeedsBarrier(resource)) {
				RendererCommandBarrier transitionToDepthRead;
				transitionToDepthRead.TextureHandle = textureHandle;
				transitionToDepthRead.BeforeState = ResolveResourceCurrentState(resource.Handle);
				transitionToDepthRead.AfterState = ResolveResourceRequiredState(resource);
				commandList.AddCommand(transitionToDepthRead);

				m_Textures[resource.Handle].State = transitionToDepthRead.AfterState;
				m_TextureManager.UpdateCurrentState(transitionToDepthRead.TextureHandle, Dx12::Dx12StateFromState(transitionToDepthRead.AfterState));
			}

			if (resource.Usage == RenderGraphBuilder::ResourceUsage::Read
				&& m_Textures[textureHandle].ViewSlot == -1)
			{
				uint32_t textureSlot = m_Blackboard.GetRenderer().m_Backend->GetDevice()->m_MainDescriptorHeap.AllocateDynamicResource();

				D3D12_CPU_DESCRIPTOR_HANDLE handle(m_Blackboard.GetRenderer().m_Backend->GetDevice()->m_MainDescriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart());
				handle.ptr += textureSlot * m_Blackboard.GetRenderer().m_Backend->GetDevice()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				D3D12_SHADER_RESOURCE_VIEW_DESC desc;
				::ZeroMemory(&desc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
				desc.Format = DXGI_FORMAT_R32_FLOAT; // TODO: This should come from the texture description
				desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				desc.Texture2D.MostDetailedMip = 0;
				desc.Texture2D.MipLevels = 1;
				desc.Texture2D.PlaneSlice = 0;
				desc.Texture2D.ResourceMinLODClamp = 0.0f;

				m_Blackboard.GetRenderer().m_Backend->GetDevice()->GetDevice()->CreateShaderResourceView(
					m_Blackboard.GetRenderer().m_Backend->Managers.Texture.GetTexture(ResolveResourceToHandle(resource.Handle)),
					&desc,
					handle
				);
				m_Textures[textureHandle].ViewSlot = textureSlot;
				m_Blackboard.SetTextureSlot(resource.Handle, textureSlot);
			}
		}

		if (pass.Description.StartNewPass)
		{
			RendererCommandBeginRenderPass beginRenderPassCommand;
			beginRenderPassCommand.ColorTarget = {
				ResolveResourceToHandle(pass.Description.RenderTarget.Handle), pass.Description.RenderTarget.LoadAction, pass.Description.RenderTarget.StoreAction
			};
			beginRenderPassCommand.DepthStencilTarget = {
				ResolveResourceToHandle(pass.Description.DepthStencilTarget.Handle), pass.Description.DepthStencilTarget.LoadAction, pass.Description.DepthStencilTarget.StoreAction
			};
			commandList.AddCommand(beginRenderPassCommand);

			passIsStarted = true;
		}

		pass.CompileFunction(commandList, m_Blackboard);
	}

	if (passIsStarted)
	{
		RendererCommandEndRenderPass endRenderPassCommand;
		commandList.AddCommand(endRenderPassCommand);
	}

	return commandList;
}

TextureHandle RenderGraph::ResolveResourceToHandle(RenderGraphResourceHandle handle)
{
	if (handle == 0) {
		return sInvalidHandle;
	}
	assert(m_Textures.find(handle) != m_Textures.end());
	return m_Textures[handle].Handle;
}

ResourceState RenderGraph::ResolveResourceCurrentState(RenderGraphResourceHandle handle)
{
	assert(m_Textures.find(handle) != m_Textures.end());
	return m_Textures[handle].State;
}

ResourceState RenderGraph::ResolveResourceRequiredState(const RenderGraphBuilder::UsedResource& resource)
{
	switch (resource.Usage)
	{
	case RenderGraphBuilder::ResourceUsage::Write:
		// TODO: Wrong, sa we can write with UAVs
		return ResourceState::RenderTarget;
	case RenderGraphBuilder::ResourceUsage::Read:
		return ResourceState::PixelShaderRead;
	case RenderGraphBuilder::ResourceUsage::RenderTarget:
		return ResourceState::RenderTarget;
	case RenderGraphBuilder::ResourceUsage::DepthStencil:
		return ResourceState::DepthWrite;
	default:
		assert(false);
		return ResourceState::RenderTarget;
	}
}

bool RenderGraph::ResourceNeedsBarrier(const RenderGraphBuilder::UsedResource& resource)
{
	return ResolveResourceRequiredState(resource) != ResolveResourceCurrentState(resource.Handle);
}
}
