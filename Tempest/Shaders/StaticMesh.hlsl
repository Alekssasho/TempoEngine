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
	uint meshletOffset;
};

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space1);

ByteAddressBuffer vertexBuffers[] : register(t0, space1);

VertexOutput VertexShaderMain(uint vertexId : SV_VertexID)
{
	//ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[g_Geometry.vertexBufferIndex];
	ByteAddressBuffer vertexBuffer = vertexBuffers[3];
	VertexLayout vertexData = vertexBuffer.Load<VertexLayout>(g_Geometry.meshletOffset + vertexId * sizeof(VertexLayout));

	float4x4 mvp = mul(g_Scene.ViewProjection, g_Geometry.WorldMatrix);
	VertexOutput result;
	result.Position = mul(mvp, float4(vertexData.Position, 1.0));
	result.PositionWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Position, 1.0)).xyz;
	// TODO: This should be inverse transpose of the world matrix
	result.NormalWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Normal, 0.0)).xyz;

	return result;
}

struct Meshlet
{
	uint vertex_offset;
	uint vertex_count;
	uint index_offset;
	uint index_count;
};

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void MeshShaderMain(
	uint gid: SV_GroupID,
	uint gtid : SV_GroupThreadID,
	out indices uint3 tris[256],
	out vertices VertexOutput verts[128])
{
	ByteAddressBuffer meshlets = vertexBuffers[1];
	ByteAddressBuffer meshletsIndices = vertexBuffers[2];
	ByteAddressBuffer meshletsVertices = vertexBuffers[3];

	Meshlet meshlet = meshlets.Load<Meshlet>((gid + g_Geometry.meshletOffset) * sizeof(Meshlet));
	SetMeshOutputCounts(meshlet.vertex_count, meshlet.index_count / 3);
	if(gtid < (meshlet.index_count / 3))
	{
		uint indicesIndex = meshlet.index_offset / 3 + gtid * 3;
		uint indicesPacked = meshletsIndices.Load<uint>(indicesIndex);
		tris[gtid] = uint3((indicesPacked & 0xFF000000) >> 24, (indicesPacked & 0x00FF0000) >> 16, (indicesPacked & 0x0000FF00) >> 8);
	}

	if (gtid < meshlet.vertex_count)
	{
		uint vertexIndex = meshlet.vertex_offset + gtid;
		VertexLayout vertexData = meshletsVertices.Load<VertexLayout>(vertexIndex * sizeof(VertexLayout));

		float4x4 mvp = mul(g_Scene.ViewProjection, g_Geometry.WorldMatrix);
		VertexOutput result;
		result.Position = mul(mvp, float4(vertexData.Position, 1.0));
		result.PositionWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Position, 1.0)).xyz;
		// TODO: This should be inverse transpose of the world matrix
		result.NormalWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Normal, 0.0)).xyz;

		verts[gtid] = result;
	}
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
