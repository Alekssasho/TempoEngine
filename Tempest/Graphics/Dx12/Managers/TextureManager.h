#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Definition
{
struct TextureDatabase;
}
namespace Dx12
{
class Dx12Device;
struct UploadData;

enum class TextureType
{
	Texture2D,
};

enum class TextureFormat
{
	RGBA8,
};

struct TextureDescription
{
	TextureType Type;
	TextureFormat Format;
	uint32_t Width;
	uint32_t Height;
	size_t Size;
	const void* Data;
};

class TextureManager
{
public:
	TextureManager(Dx12Device& device);
	TextureHandle CreateTexture(const TextureDescription& description, UploadData* upload);
	ID3D12Resource* GetTexture(uint32_t textureHandle);
private:
	eastl::unordered_map<TextureHandle, ComPtr<ID3D12Resource>> m_Textures;
	TextureHandle m_NextHandle = 0;
	Dx12Device& m_Device;
};
}
}