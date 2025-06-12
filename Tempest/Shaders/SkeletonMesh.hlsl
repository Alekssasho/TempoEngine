#include "MeshCommon.hlsl"

// TODO: Split into seperate streams
struct VertexLayout
{
	float3 Position;
	float3 Normal;
	float2 UV;
	uint Joints;
	float4 Weights;
};

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space1);

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void MeshShaderMain(
	uint gid: SV_GroupID,
	uint gtid : SV_GroupThreadID,
	out indices uint3 tris[128],
	out vertices VertexOutput verts[128])
{
	StructuredBuffer<Meshlet> meshlets = ResourceDescriptorHeap[ShaderResourceSlot::Meshlets];
	Buffer<uint> meshletsIndices = ResourceDescriptorHeap[ShaderResourceSlot::MeshletIndices];
	StructuredBuffer<VertexLayout> meshletsVertices = ResourceDescriptorHeap[ShaderResourceSlot::MeshletSkeletonVertices];
	StructuredBuffer<float4x4> boneTransforms = ResourceDescriptorHeap[g_Geometry.extraDataIndex];

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

		uint joint4 = (vertexData.Joints >> 24);
		uint joint3 = ((vertexData.Joints & 0x00ff0000) >> 16);
		uint joint2 = ((vertexData.Joints & 0x0000ff00) >> 8);
		uint joint1 = ((vertexData.Joints & 0x000000ff));

		float4x4 skinMatrix = 
			mul(vertexData.Weights.x, boneTransforms[g_Geometry.extraDataStartIndex + joint1]) +
			mul(vertexData.Weights.y, boneTransforms[g_Geometry.extraDataStartIndex + joint2]) +
			mul(vertexData.Weights.z, boneTransforms[g_Geometry.extraDataStartIndex + joint3]) +
			mul(vertexData.Weights.w, boneTransforms[g_Geometry.extraDataStartIndex + joint4]);

		float4 skinnedPos = mul(skinMatrix, float4(vertexData.Position, 1.0));

		VertexOutput result;
		result.Position = mul(mvp, skinnedPos);
		result.PositionWorld = mul(g_Geometry.WorldMatrix, skinnedPos).xyz;
		// TODO: This should be inverse transpose of the world matrix
		result.NormalWorld = mul(g_Geometry.WorldMatrix, float4(vertexData.Normal, 0.0)).xyz;
		result.UV = vertexData.UV;

		verts[gtid] = result;
	}
}
