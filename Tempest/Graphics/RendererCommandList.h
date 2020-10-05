#pragma once

#include <Graphics/RendererTypes.h>
#include <EASTL/vector.h>

namespace Tempest
{
enum class RendererCommandType : uint8_t
{
	DrawInstanced,
	Count
};

struct ShaderParameterView
{
	uint32_t GeometryConstantDataOffset;
};

template<RendererCommandType TType>
struct alignas(16) RendererCommand 
{
	RendererCommandType Type;

	RendererCommand()
		: Type(TType)
	{}
};

struct RendererCommandDrawInstanced : RendererCommand<RendererCommandType::DrawInstanced>
{
	PipelineStateHandle Pipeline;
	ShaderParameterView ParameterView;
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
};

struct RendererCommandList
{
	template<typename T>
	void AddCommand(const T& command)
	{
		m_DataBuffer.reserve(m_DataBuffer.size() + sizeof(T));
		m_DataBuffer.insert(m_DataBuffer.end(), reinterpret_cast<const uint8_t*>(&command), reinterpret_cast<const uint8_t*>(&command) + sizeof(T));
	}

	// Insert data into the constant buffer data and return offset into the constant data for later binding to shaders.
	template<typename T>
	uint32_t AddConstantData(const T& data)
	{
		uint32_t offset = uint32_t(m_ConstantBufferData.size());
		// TODO: this 256 should come from backend. Maybe there should be queried as well.
		const size_t alignedSize = ((sizeof(T) + 255) / 256) * 256;
		m_ConstantBufferData.resize(offset + alignedSize);
		memcpy(m_ConstantBufferData.data() + offset, &data, sizeof(T));
		return offset;
	}

//private:
	eastl::vector<uint8_t> m_DataBuffer;
	// TODO: This should probably be multiple per shader parameter space
	eastl::vector<uint8_t> m_ConstantBufferData;
};
}