#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Definition
{
struct TextureData;
}
namespace Dx12
{
class Dx12Device;
struct UploadData;

enum class TextureType
{
	Texture2D,
};

struct TextureDescription
{
	TextureType Type;
	DXGI_FORMAT Format;
	uint32_t Width;
	uint32_t Height;
	size_t Size;
	const void* Data;
};

DXGI_FORMAT DxFormatForStorageFromTextureFormat(const Definition::TextureData& data);
DXGI_FORMAT DxFormatForViewFromTextureFormat(const Definition::TextureData& data);

class TextureManager : Utils::NonCopyable
{
public:
	TextureManager(Dx12Device& device);
	TextureHandle CreateTexture(const TextureDescription& description, D3D12_RESOURCE_STATES initialState, UploadData* upload);
	ID3D12Resource* GetTexture(TextureHandle textureHandle);
	glm::ivec2 GetTextureDimensions(TextureHandle textureHandle);
private:
	eastl::unordered_map<TextureHandle, eastl::pair<TextureDescription, ComPtr<ID3D12Resource>>> m_Textures;
	TextureHandle m_NextHandle = 1; // TODO: for now invalid handle is 0
	Dx12Device& m_Device;
};

// TODO: Move to seperate file
class TemporaryTextureManager : Utils::NonCopyable
{
public:
	TemporaryTextureManager(TextureManager& manager);

	eastl::pair<TextureHandle, D3D12_RESOURCE_STATES> RequestTexture(const TextureDescription& desc);
	void UpdateCurrentState(TextureHandle, D3D12_RESOURCE_STATES state);
private:
	TextureManager& m_TextureManager;
	struct TextureResource
	{
		TextureHandle Handle;
		TextureDescription Description;
		D3D12_RESOURCE_STATES CurrentState;
	};
	eastl::vector<TextureResource> m_Textures;
};
}
}