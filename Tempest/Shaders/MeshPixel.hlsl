#include "MeshCommon.hlsl"

ConstantBuffer<GeometryConstants> g_Geometry : register(b0, space1);

// TODO: Use sampler descriptor heaps for thouse
SamplerState MaterialTextureSampler : register(s0, space0);
SamplerComparisonState ShadowMapSampler : register(s1, space0);

float4 PixelShaderMain(VertexOutput input) : SV_TARGET
{
	StructuredBuffer<Material> materials = ResourceDescriptorHeap[ShaderResourceSlot::Materials];

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
			Texture2D<float4> baseTexture = ResourceDescriptorHeap[ShaderResourceSlot::TextureStart + materials[g_Geometry.materialIndex].BaseColorTextureIndex];
			color *= baseTexture.Sample(MaterialTextureSampler, input.UV);
		}
		material.BaseColor = color.rgb;
	}

	{
		float metallic = materials[g_Geometry.materialIndex].Metallic;
		float peceptualRoughness = materials[g_Geometry.materialIndex].Roughness;
		if(materials[g_Geometry.materialIndex].MetallicRoughnessTextureIndex != -1) {
			Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[ShaderResourceSlot::TextureStart + materials[g_Geometry.materialIndex].MetallicRoughnessTextureIndex];
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
