struct VertexLayout
{
	float3 Position;
};

struct VertexOutput
{
	float4 Position : SV_POSITION;
};

struct GeometryConstants
{
	//float4x4 WorldViewProjectionMatrix;
	//float4x4 WorldMatrix;

	// TOOD: This could be lower bits and packed with something else probably
	uint vertexBufferIndex;
	uint vertexBufferOffset;
};

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space0);

ByteAddressBuffer vertexBuffers[] : register(t0, space0);

VertexOutput VertexShaderMain(uint vertexId : SV_VertexID)
{
	VertexLayout vertexData = vertexBuffers[g_Geometry.vertexBufferIndex].Load<VertexLayout>(g_Geometry.vertexBufferOffset + vertexId * sizeof(VertexLayout));

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
