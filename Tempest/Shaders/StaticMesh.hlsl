#include "Common.hlsl"

// TODO: Split into seperate streams
struct VertexLayout
{
	float3 Position;
	float3 Normal;
};

struct VertexOutput
{
	float4 Position : SV_POSITION;
	float3 PositionWorld : POSITION;
	float3 NormalWorld : NORMAL;
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
	result.PositionWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Position, 1.0));
	// TODO: This should be inverse transpose of the world matrix
	result.NormalWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Normal, 0.0));

	return result;
}

float4 PixelShaderMain(VertexOutput input) : SV_TARGET
{
	float3 normal = input.NormalWorld;

	float diffuseFactor = saturate(dot(normal, -g_Scene.LightDirection.xyz));

	float ambientFactor = 0.15f;

	float4 color = 1.0f;

	return (color * diffuseFactor * g_Scene.LightColor)
			+ (color * ambientFactor);
}
