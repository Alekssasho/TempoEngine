#pragma once

static const float M_PI = 3.141592653589793;

struct SceneSettings
{
	float4x4 ViewProjection;
	float4x4 LightShadowMatrix;
	float4 LightDirection;
	float4 LightColor;
	float3 CameraWorldPosition;
	uint LightShadowMapIndex;
};

ConstantBuffer<SceneSettings> g_Scene : register(b0, space0);

struct PBRMaterialComponents
{
	float3 BaseColor;
	float Roughness; // Non perceptual rougness
	float Metallic;
};

struct ShadingSurfaceInfo
{
	float NdotH;
	float NdotL;
	float NdotV;
	float VdotH;
};
