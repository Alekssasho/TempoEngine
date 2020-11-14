#include "Common.hlsl"

struct VertexLayout
{
	float3 Position;
};

struct VertexOutput
{
	float4 Position : SV_POSITION;
	float3 PositionWorld : POSITION;
};

struct GeometryConstants
{
	float4x4 WorldMatrix;

	// TOOD: This could be lower bits and packed with something else probably
	uint vertexBufferIndex;
	uint vertexBufferOffset;
};

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space1);

ByteAddressBuffer vertexBuffers[] : register(t0, space1);

VertexOutput VertexShaderMain(uint vertexId : SV_VertexID)
{
	VertexLayout vertexData = vertexBuffers[g_Geometry.vertexBufferIndex].Load<VertexLayout>(g_Geometry.vertexBufferOffset + vertexId * sizeof(VertexLayout));

	float4x4 mvp = mul(g_Scene.ViewProjection, g_Geometry.WorldMatrix);
	VertexOutput result;
	result.Position = mul(mvp, float4(vertexData.Position, 1.0));
	result.PositionWorld = vertexData.Position;

	return result;
}

struct DirectionalLight
{
	float3 Direction;
};

float4 PixelShaderMain(VertexOutput input) : SV_TARGET
{
	DirectionalLight light;
	light.Direction = normalize(float3(-1.0f, -1.0f, 0.0f));

	float3 worldPositionDDX = ddx(input.PositionWorld);
	float3 worldPositionDDY = ddy(input.PositionWorld);
	float3 normal = normalize(cross(worldPositionDDX, worldPositionDDY));

	float diffuseFactor = saturate(dot(normal, -light.Direction));

	float ambientFactor = 0.15f;

	float4 color = 1.0f;

	return (color * diffuseFactor)
			+ (color * ambientFactor);
}
