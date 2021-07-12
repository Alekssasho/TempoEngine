#pragma once

#include <Graphics/Dx12/Dx12Common.h>
#include <Graphics/Dx12/Dx12Device.h>
#include <Graphics/RendererTypes.h>

namespace Tempest
{
namespace Definition
{
struct TextureDatabase;
}
namespace Dx12
{

class TextureManager
{
public:
	TextureManager(Dx12Device& device);
	void LoadDatabase(const Definition::TextureDatabase* database);
private:
	ComPtr<ID3D12Resource> CreateStagingBuffer(size_t size);
	void InitializeTextureData(ID3D12Resource* dst, size_t size, const void* data);

	eastl::unordered_map<TextureHandle, ComPtr<ID3D12Resource>> m_Textures;
	TextureHandle m_NextHandle = 0;
	Dx12Device& m_Device;
};
}
}