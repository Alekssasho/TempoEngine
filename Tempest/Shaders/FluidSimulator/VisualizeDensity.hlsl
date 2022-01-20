struct VSOutput {
    float4 position: SV_POSITION;
    float2 uv: TEXCOORD0;
};

struct ConstantsData {
    uint2 grid_size;
};

ConstantBuffer<ConstantsData> g_constant_data : register(b0, space0);

VSOutput VertexShaderMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 PixelShaderMain(VSOutput input) : SV_TARGET {
    StructuredBuffer<float> g_density_field = ResourceDescriptorHeap[1];
    uint2 position_in_grid = input.uv * g_constant_data.grid_size;
    return float4(g_density_field[position_in_grid.x + position_in_grid.y * g_constant_data.grid_size.x], 0.0, 0.0, 1.0);
}
