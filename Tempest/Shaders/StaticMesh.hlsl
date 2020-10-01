struct VertexLayout
{
	float3 Position;
};

struct VertexOutput
{
	float4 Position : SV_POSITION;
};

cbuffer VertexPushs : register(b0)
{
	//float4x4 WorldViewProjectionMatrix;
	//float4x4 WorldMatrix;

	// TOOD: This could be lower bits and packed with something else probably
	uint vertexBufferIndex;
	uint vertexBufferOffset;
};

ByteAddressBuffer vertexBuffers[] : register(t0);

VertexOutput VertexShaderMain(uint vertexId : SV_VertexID)
{
	VertexLayout vertexData = vertexBuffers[vertexBufferIndex].Load<VertexLayout>(vertexBufferOffset + vertexId * sizeof(VertexLayout));

	VertexOutput result;
	//result.Position = mul(WorldViewProjectionMatrix, float4(input.Position, 1.0));
	result.Position = float4(vertexData.Position, 1.0);

	return result;
}

float4 PixelShaderMain(VertexOutput input) : SV_TARGET
{
	// Ugly magenta
	return float4(1.0, 0.0, 1.0, 1.0);
}
