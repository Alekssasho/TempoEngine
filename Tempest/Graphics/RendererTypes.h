#pragma once

#include <inttypes.h>

namespace Tempest
{
static const uint32_t sInvalidHandle = 0;
using PipelineStateHandle = uint32_t;

enum class RenderPhase : uint8_t
{
	Main,
	Shadow
};

class World;
struct FrameData;
struct RendererCommandList;

using MeshHandle = uint32_t;
using BufferHandle = uint32_t;
using TextureHandle = uint32_t;

// Render Graph Types
using RenderGraphResourceHandle = uint32_t;
// TODO: This could be imported explicitly probably
static const RenderGraphResourceHandle sBackbufferRenderTargetRenderGraphHandle = 1;
static const RenderGraphResourceHandle sBackbufferDepthStencilRenderGraphHandle = 2;
}
