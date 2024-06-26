#include "Common.hlsl"
#include "BRDF.hlsl"

// TODO: Split into seperate streams
struct VertexLayout
{
	float3 Position;
	float3 Normal;
	float2 UV;
};

struct VertexOutput
{
	float4 Position : SV_POSITION;
	float3 PositionWorld : POSITION;
	float3 NormalWorld : NORMAL;
	float2 UV : TEX_COORD;
};

struct GeometryConstants
{
	float4x4 WorldMatrix;
	uint meshletOffset;
	uint materialIndex;
};

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space1);

struct Meshlet
{
	uint vertex_offset;
	uint vertex_count;
	uint triangle_offset;
	uint triangle_count;
};

struct Material
{
	float4 BaseColor;
	float Metallic;
	float Roughness;
	uint BaseColorTextureIndex;
	uint MetallicRoughnessTextureIndex;
};

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void MeshShaderMain(
	uint gid: SV_GroupID,
	uint gtid : SV_GroupThreadID,
	out indices uint3 tris[128],
	out vertices VertexOutput verts[128])
{
	StructuredBuffer<Meshlet> meshlets = ResourceDescriptorHeap[0];
	Buffer<uint> meshletsIndices = ResourceDescriptorHeap[1];
	StructuredBuffer<VertexLayout> meshletsVertices = ResourceDescriptorHeap[2];

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
		result.UV = vertexData.UV;

		verts[gtid] = result;
	}
}

// TODO: Use sampler descriptor heaps for thouse
SamplerState MaterialTextureSampler : register(s0, space0);
SamplerComparisonState ShadowMapSampler : register(s1, space0);

float4 PixelShaderMain(VertexOutput input) : SV_TARGET
{
	StructuredBuffer<Material> materials = ResourceDescriptorHeap[3];

	float ambientFactor = 0.05f;

	const float3 N = normalize(input.NormalWorld);
	const float3 V = normalize(g_Scene.CameraWorldPosition - input.PositionWorld);
	const float3 L = normalize(-g_Scene.LightDirection.xyz);
	const float3 H = normalize(L + V); // Halfway between light and view

	ShadingSurfaceInfo shadingInfo;
	shadingInfo.NdotH = saturate(dot(N, H));
	shadingInfo.NdotL = saturate(dot(N, L));
	shadingInfo.NdotV = saturate(dot(N, V));
	shadingInfo.VdotH = saturate(dot(V, H));

	PBRMaterialComponents material;

	{
		float4 color = materials[g_Geometry.materialIndex].BaseColor;
		if(materials[g_Geometry.materialIndex].BaseColorTextureIndex != -1) {
			Texture2D<float4> baseTexture = ResourceDescriptorHeap[4 + materials[g_Geometry.materialIndex].BaseColorTextureIndex];
			color *= baseTexture.Sample(MaterialTextureSampler, input.UV);
		}
		material.BaseColor = color.rgb;
	}

	{
		float metallic = materials[g_Geometry.materialIndex].Metallic;
		float peceptualRoughness = materials[g_Geometry.materialIndex].Roughness;
		if(materials[g_Geometry.materialIndex].MetallicRoughnessTextureIndex != -1) {
			Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[4 + materials[g_Geometry.materialIndex].MetallicRoughnessTextureIndex];
			float4 sampledValues = metallicRoughnessTexture.Sample(MaterialTextureSampler, input.UV);
			// In GLTF metallic and roughness are packed in B and G channel of a single texture
			metallic *= sampledValues.b;
			peceptualRoughness *= sampledValues.g;
		}
		material.Metallic = metallic;
		// TODO: remove the min value, when IBL or Area lights are added
		// We currently support only analytical light, so we need to clamp this to avoid divisions by zero
		// This value is taken from Filament document and it says it is taken from Frostbite engine
		material.Roughness = max(peceptualRoughness * peceptualRoughness, 0.045);
	}

	// TODO: no need for perspective divide, as directional lights are using ortho projection matrix
	float3 shadowMapCoords = mul(g_Scene.LightShadowMatrix, float4(input.PositionWorld, 1.0f)).xyz;
	// TODO: Bake this into the shadow matrix
	shadowMapCoords.xy = shadowMapCoords.xy * 0.5 + 0.5; // Clip space is [-1;1] so we need to convert it to uv space [0;1]
	shadowMapCoords.y = 1.0 - shadowMapCoords.y; // After uv space transform bottom is at 0 and top is 1, but texture space is opposite so invert

	Texture2D shadowMap = ResourceDescriptorHeap[g_Scene.LightShadowMapIndex];
	uint shadowMapWidth, shadowMapHeight;
	shadowMap.GetDimensions(shadowMapWidth, shadowMapHeight);

	// 16 tap PCF
	float sum = 0;
	for(float y = -1.5; y <= 1.5; y += 1.0)
	{
		for(float x = -1.5; x <= 1.5; x += 1.0)
		{
			// Sample CmpLevelZero is using linear sampler, so it is essentially 4 tap
			// We call it 4 times (per dimensions) to total of 16 taps
			sum += shadowMap.SampleCmpLevelZero(ShadowMapSampler, shadowMapCoords.xy + float2(x * 1.0 / shadowMapWidth, y * 1.0 / shadowMapHeight), shadowMapCoords.z);
		}
	}
	float shadowFactor = sum / 16.0;

	const float3 result = (material.BaseColor * ambientFactor) // ambient
		+ (BRDF_PBR(shadingInfo, material) * g_Scene.LightColor.rgb * shadowFactor * shadingInfo.NdotL); // Don't forget the cos of N and L factor which is outside of the BRDF in the integral
	return float4(result, 1.0);
}
