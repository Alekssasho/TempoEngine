#pragma once

#include <Graphics/RendererTypes.h>

namespace Tempest
{
enum class RendererCommandType : uint8_t
{
	DrawInstanced,
	DrawMeshlet,
	BeginRenderPass,
	EndRenderPass,
	Count
};

enum class ShaderParameterType : uint8_t
{
	Scene,
	Geometry,
	Count
};

struct ShaderParameterView
{
	uint32_t ConstantDataOffset;
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
	ShaderParameterView ParameterViews[size_t(ShaderParameterType::Count)];
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
};

struct RendererCommandDrawMeshlet : RendererCommand<RendererCommandType::DrawMeshlet>
{
	PipelineStateHandle Pipeline;
	ShaderParameterView ParameterViews[size_t(ShaderParameterType::Count)];
	uint32_t MeshletCount;
};


enum class TextureTargetStoreAction : uint8_t
{
	DoNotCare,
	Store
};

enum class TextureTargetLoadAction : uint8_t
{
	DoNotCare,
	Clear,
	Load,
};

struct RendererCommandBeginRenderPass : RendererCommand<RendererCommandType::BeginRenderPass>
{
	struct TextureTarget
	{
		TextureHandle Texture;
		TextureTargetLoadAction LoadAction;
		TextureTargetStoreAction StoreAction;
	};
	TextureTarget ColorTarget;
	TextureTarget DepthStencilTarget;
};

struct RendererCommandEndRenderPass : RendererCommand<RendererCommandType::EndRenderPass>
{
};

struct RendererCommandList
{
	template<typename T>
	void AddCommand(const T& command)
	{
		m_DataBuffer.reserve(m_DataBuffer.size() + sizeof(T));
		m_DataBuffer.insert(m_DataBuffer.end(), reinterpret_cast<const uint8_t*>(&command), reinterpret_cast<const uint8_t*>(&command) + sizeof(T));
	}

//private:
	eastl::vector<uint8_t> m_DataBuffer;
};
}