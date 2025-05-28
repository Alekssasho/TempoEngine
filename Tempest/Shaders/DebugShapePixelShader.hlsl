#include "DebugShapeCommon.hlsl"

float4 PixelShaderMain(VertexOutput data) : SV_TARGET
{
	return data.color;
}