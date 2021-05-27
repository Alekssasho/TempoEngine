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

struct Meshlet
{
	uint vertex_offset;
	uint vertex_count;
	uint triangle_offset;
	uint triangle_count;
};

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void MeshShaderMain(
	uint gid: SV_GroupID,
	uint gtid : SV_GroupThreadID,
	out indices uint3 tris[128],
	out vertices VertexOutput verts[128])
{
	StructuredBuffer<Meshlet> meshlets = ResourceDescriptorHeap[1];
	Buffer<uint> meshletsIndices = ResourceDescriptorHeap[2];
	StructuredBuffer<VertexLayout> meshletsVertices = ResourceDescriptorHeap[3];

	Meshlet meshlet = meshlets[gid + g_Geometry.meshletOffset];
	SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);
	if(gtid < meshlet.triangle_count)
	{
		uint indicesIndex = meshlet.triangle_offset + gtid * 3;
		tris[gtid] = uint3(meshletsIndices[indicesIndex], meshletsIndices[indicesIndex + 1], meshletsIndices[indicesIndex + 2]);
	}

	if (gtid < meshlet.vertex_count)
	{
		uint vertexIndex = meshlet.vertex_offset + gtid;
		VertexLayout vertexData = meshletsVertices[vertexIndex];

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
