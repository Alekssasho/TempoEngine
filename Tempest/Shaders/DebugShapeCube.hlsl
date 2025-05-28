#include "DebugShapeCommon.hlsl"

struct DebugRectShapeData
{
	float4x4 Transform;
	float4 Color;
};

ConstantBuffer<DebugRectShapeData> g_Geometry : register(b0, space1);

static float4 s_vertexPos[8] = {
	float4(0.0f, 0.0f, 0.0f, 1.0f), //0
	float4(1.0f, 0.0f, 0.0f, 1.0f), //1
	float4(1.0f, 0.0f, 1.0f, 1.0f), //2
	float4(0.0f, 0.0f, 1.0f, 1.0f), //3
	float4(0.0f, 1.0f, 0.0f, 1.0f), //4
	float4(1.0f, 1.0f, 0.0f, 1.0f), //5
	float4(1.0f, 1.0f, 1.0f, 1.0f), //6
	float4(0.0f, 1.0f, 1.0f, 1.0f)  //7
};

[NumThreads(8, 1, 1)]
[OutputTopology("triangle")]
void MeshShaderMain(
	uint gid: SV_GroupID,
	uint gtid : SV_GroupThreadID,
	out indices uint3 tris[12],
	out vertices VertexOutput verts[8])
{
	SetMeshOutputCounts(8, 12);
	float4 vertex = s_vertexPos[gtid];
	DebugRectShapeData rectData = g_Geometry;

	float4x4 mvp = mul(g_Scene.ViewProjection, rectData.Transform);
	VertexOutput result;
	result.pos = mul(mvp, vertex);
	result.color = rectData.Color;

	verts[gtid] = result;

	if(gtid == 0)
	{
		// bottom
		tris[0] = uint3(0, 1, 2);
		tris[1] = uint3(0, 2, 3);

		//top
		tris[2] = uint3(4, 6, 5);
		tris[3] = uint3(4, 7, 6);

		//front
		tris[4] = uint3(0, 5, 1);
		tris[5] = uint3(0, 4, 5);

		//back
		tris[6] = uint3(3, 6, 7);
		tris[7] = uint3(3, 2, 6);

		//left
		tris[8] = uint3(0, 3, 7);
		tris[9] = uint3(0, 7, 4);

		//right
		tris[10] = uint3(1, 5, 6);
		tris[11] = uint3(1, 6, 2);
	}
}
