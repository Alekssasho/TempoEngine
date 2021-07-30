#pragma once

struct SceneSettings
{
	float4x4 ViewProjection;
	float4x4 LightShadowMatrix;
	float4 LightDirection;
	float4 LightColor;
	uint LightShadowMapIndex;
};

ConstantBuffer<SceneSettings> g_Scene : register(b0, space0);