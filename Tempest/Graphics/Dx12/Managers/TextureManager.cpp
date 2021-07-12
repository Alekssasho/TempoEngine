#include <CommonIncludes.h>
#include <Graphics/Dx12/Managers/BufferManager.h>

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

void TextureManager::LoadDatabase(const Definition::TextureDatabase* database)
{
	for (const auto& texture : *database->mappings())
	{

	}
}

ComPtr<ID3D12Resource> TextureManager::CreateStagingBuffer(size_t size)
{
	D3D12_HEAP_PROPERTIES props;
	::ZeroMemory(&props, sizeof(D3D12_HEAP_PROPERTIES));
	props.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc;
	::ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ComPtr<ID3D12Resource> buffer;
	CHECK_SUCCESS(m_Device.GetDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&buffer)));
	return buffer;
}

void TextureManager::InitializeTextureData(ID3D12Resource* dst, size_t size, const void* data)
{
	ComPtr<ID3D12Resource> stagingBuffer = CreateStagingBuffer(size);
	D3D12_RANGE range;
	::ZeroMemory(&range, sizeof(D3D12_RANGE));
	void* stagingBufferData = nullptr;
	CHECK_SUCCESS(stagingBuffer->Map(0, &range, &stagingBufferData));

	memcpy(stagingBufferData, data, size);

	stagingBuffer->Unmap(0, nullptr);

	//m_Device.CopyResources(dst, stagingBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
}
}
}