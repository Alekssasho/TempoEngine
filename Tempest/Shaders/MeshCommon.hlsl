#include "Common.hlsl"
#include "BRDF.hlsl"

struct VertexOutput
{
	float4 Position : SV_POSITION;
	float3 PositionWorld : POSITION;
	float3 NormalWorld : NORMAL;
	float2 UV : TEX_COORD;
};

struct GeometryConstants
{
	float4x4 WorldMatrix;
	uint meshletOffset;
	uint materialIndex;
};

struct Meshlet
{
	uint vertex_offset;
	uint vertex_count;
	uint triangle_offset;
	uint triangle_count;
};

struct Material
{
	float4 BaseColor;
	float Metallic;
	float Roughness;
	uint BaseColorTextureIndex;
	uint MetallicRoughnessTextureIndex;
};
