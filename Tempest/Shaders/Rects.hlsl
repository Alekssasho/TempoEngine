struct VertexInput
{
	uint index : SV_VertexID;
};

struct GeometryConstants
{
	float x;
	float y;
	float width;
	float height;
	float4 color;
};

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space1);

float4 VertexShaderMain(VertexInput i) : SV_POSITION
{
	if (i.index == 0)
	{
		return float4(g_Geometry.x, g_Geometry.y, 0.0, 1.0);
	}
	else if (i.index == 1)
	{
		return float4(g_Geometry.x + g_Geometry.width, g_Geometry.y, 0.0, 1.0);
	}
	else if (i.index == 2)
	{
		return float4(g_Geometry.x, g_Geometry.y + g_Geometry.height, 0.0, 1.0);
	}
	else
	{
		return float4(g_Geometry.x + g_Geometry.width, g_Geometry.y + g_Geometry.height, 0.0, 1.0);
	}
}

float4 PixelShaderMain(float4 pos : SV_POSITION) : SV_TARGET
{
	return g_Geometry.color;
}