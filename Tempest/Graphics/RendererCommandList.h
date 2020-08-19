#pragma once

#include <Graphics/RendererTypes.h>
#include <EASTL/vector.h>

namespace Tempest
{
enum class RendererCommandType : uint8_t
{
	DrawRect, // TODO: Remove me
	Count
};

template<RendererCommandType TType>
struct alignas(16) RendererCommand 
{
	RendererCommandType Type;

	RendererCommand()
		: Type(TType)
	{}
};

struct RendererCommandDrawRect : RendererCommand<RendererCommandType::DrawRect>
{
	RectData Data;
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