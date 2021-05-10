#pragma once

struct SceneSettings
{
	float4x4 ViewProjection;
	float4 LightDirection;
	float4 LightColor;
};

ConstantBuffer<SceneSettings> g_Scene : register(b0, space0);