#include <CommonIncludes.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <Graphics/Dx12/Managers/TextureManager.h>

namespace Tempest
{
namespace Dx12
{
TextureManager::TextureManager(Dx12Device& device)
	: m_Device(device)
{
}

TextureHandle TextureManager::CreateTexture(const TextureDescription& description, UploadData* upload)
{
	D3D12_HEAP_PROPERTIES props;
	::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
	props.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = description.Width;
	desc.Height = description.Height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	assert(description.Format == TextureFormat::RGBA8);
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	PipelineStateHandle resultHandle = m_NextHandle++;
	ComPtr<ID3D12Resource>& texture = m_Textures[resultHandle];
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
	CHECK_SUCCESS(m_Device.GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, NULL, IID_PPV_ARGS(&texture)));

	if (description.Data && upload)
	{
		memcpy(reinterpret_cast<uint8_t*>(upload->MappedData) + upload->CurrentOffset, description.Data, description.Size);

		D3D12_TEXTURE_COPY_LOCATION dstCopyLocation;
		::ZeroMemory(&dstCopyLocation, sizeof(D3D12_TEXTURE_COPY_LOCATION));
		dstCopyLocation.pResource = texture.Get();
		dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstCopyLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION srcCopyLocation;
		srcCopyLocation.pResource = upload->UploadHeap.Get();
		srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		m_Device.GetDevice()->GetCopyableFootprints(&desc, 0, 1, upload->CurrentOffset, &srcCopyLocation.PlacedFootprint, nullptr, nullptr, nullptr);

		upload->CommandList->CopyTextureRegion(&dstCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);

		upload->CurrentOffset += description.Size;

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = texture.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // TODO: If we need to read from non Pixel Shader must add another state as well
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// TODO: Batch those
		upload->CommandList->ResourceBarrier(1, &barrier);
	}

	return resultHandle;
}

ID3D12Resource* TextureManager::GetTexture(uint32_t textureHandle)
{
	return m_Textures[textureHandle].Get();
}
}
}