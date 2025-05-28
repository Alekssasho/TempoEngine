#include "DebugShapeCommon.hlsl"

struct DebugRectShapeData
{
	float4x4 Transform;
	float4 Color;
};

ConstantBuffer<DebugRectShapeData> g_Geometry : register(b0, space1);

static float4 s_vertexPos[4] = {
	float4(0.0f, 0.0f, 0.0f, 1.0f),
	float4(1.0f, 0.0f, 0.0f, 1.0f),
	float4(1.0f, 0.0f, 1.0f, 1.0f),
	float4(0.0f, 0.0f, 1.0f, 1.0f)
};

[NumThreads(4, 1, 1)]
[OutputTopology("triangle")]
void MeshShaderMain(
	uint gid: SV_GroupID,
	uint gtid : SV_GroupThreadID,
	out indices uint3 tris[2],
	out vertices VertexOutput verts[4])
{
	SetMeshOutputCounts(4, 2);
	float4 vertex = s_vertexPos[gtid];
	DebugRectShapeData rectData = g_Geometry;

	float4x4 mvp = mul(g_Scene.ViewProjection, rectData.Transform);
	VertexOutput result;
	result.pos = mul(mvp, vertex);
	result.color = rectData.Color;

	verts[gtid] = result;

	if(gtid == 0)
	{
		tris[0] = uint3(0, 2, 1);
		tris[1] = uint3(0, 3, 2);
	}
}
