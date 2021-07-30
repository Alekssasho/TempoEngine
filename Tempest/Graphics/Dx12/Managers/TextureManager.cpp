#include <CommonIncludes.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/Dx12/Dx12Backend.h>
#include <Graphics/Dx12/Managers/TextureManager.h>
#include <DataDefinitions/TextureDatabase_generated.h>

namespace Tempest
{
namespace Dx12
{
TextureManager::TextureManager(Dx12Device& device)
	: m_Device(device)
{
}

TextureHandle TextureManager::CreateTexture(const TextureDescription& description, D3D12_RESOURCE_STATES initialState, UploadData* upload)
{
	const bool isDepth = description.Format == DXGI_FORMAT_D32_FLOAT;

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
	desc.Format = description.Format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = !isDepth ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // TODO: more types of depth

	PipelineStateHandle resultHandle = m_NextHandle++;
	eastl::pair<TextureDescription, ComPtr<ID3D12Resource>>& texture = m_Textures[resultHandle];
	texture.first = description;
	D3D12_RESOURCE_STATES state = (description.Data && upload) ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;

	D3D12_CLEAR_VALUE clearValue;
	::ZeroMemory(&clearValue, sizeof(D3D12_CLEAR_VALUE));
	clearValue.Format = description.Format;
	if(isDepth)
	{
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
	}
	CHECK_SUCCESS(m_Device.GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, isDepth ? &clearValue : nullptr, IID_PPV_ARGS(&texture.second)));

	if (description.Data && upload)
	{
		memcpy(reinterpret_cast<uint8_t*>(upload->MappedData) + upload->CurrentOffset, description.Data, description.Size);

		D3D12_TEXTURE_COPY_LOCATION dstCopyLocation;
		::ZeroMemory(&dstCopyLocation, sizeof(D3D12_TEXTURE_COPY_LOCATION));
		dstCopyLocation.pResource = texture.second.Get();
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
		barrier.Transition.pResource = texture.second.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = initialState;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		// TODO: Batch those
		upload->CommandList->ResourceBarrier(1, &barrier);
	}

	return resultHandle;
}

ID3D12Resource* TextureManager::GetTexture(uint32_t textureHandle)
{
	return m_Textures[textureHandle].second.Get();
}

glm::ivec2 TextureManager::GetTextureDimensions(TextureHandle textureHandle)
{
	return glm::ivec2(m_Textures[textureHandle].first.Width, m_Textures[textureHandle].first.Height);
}

DXGI_FORMAT DxFormatForStorageFromTextureFormat(const Definition::TextureData& data)
{
	switch (data.format())
	{
	case Definition::TextureFormat_RGBA8:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case Definition::TextureFormat_BC1_RGB:
		return DXGI_FORMAT_BC1_TYPELESS;
	case Definition::TextureFormat_BC7_RGBA:
		return DXGI_FORMAT_BC7_TYPELESS;
	default:
		assert(false);
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT DxFormatForViewFromTextureFormat(const Definition::TextureData& data)
{
	// TODO: If we add more than linear/srgb, change this to switch
	const bool isSrgb = data.color_space() == Definition::ColorSpace_sRGB;
	switch (data.format())
	{
	case Definition::TextureFormat_RGBA8:
		return isSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
	case Definition::TextureFormat_BC1_RGB:
		return isSrgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
	case Definition::TextureFormat_BC7_RGBA:
		return isSrgb ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
	default:
		assert(false);
		return DXGI_FORMAT_UNKNOWN;
	}
}
}
}