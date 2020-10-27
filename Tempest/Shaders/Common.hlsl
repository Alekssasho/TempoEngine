#pragma once

struct SceneSettings
{
	float4x4 ViewProjection;
};

ConstantBuffer<SceneSettings> g_Scene : register(b0, space0);