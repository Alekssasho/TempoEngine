#pragma once

float3 BRDF_Diffuse_Lambertian(float3 albedo)
{
    return albedo / M_PI;
}

// Smith Joint masking-shadowing function for GGX
// Visibility = G / (4 * NdotL * NdotV)
float GGX_Visibility(float NdotL, float NdotV, float roughnessSquare)
{
    const float GGXV = NdotL * sqrt(NdotV * NdotV * (1 - roughnessSquare) + roughnessSquare);
    const float GGXL = NdotV * sqrt(NdotL * NdotL * (1 - roughnessSquare) + roughnessSquare);

    const float GGX = GGXV + GGXL;
    if(GGX > 0.0) {
        return 0.5f / GGX;
    }
    return 0.0;
}

// Trowbridge-Reitz/GGX
float GGX_Distribution(float NdotH, float roughnessSquare)
{
    const float f = (NdotH * NdotH) * (roughnessSquare - 1.0) + 1.0;
    return roughnessSquare / (M_PI * f * f);
}


float3 BRDF_Specular_GGX(float NdotL, float NdotV, float NdotH, float roughness)
{
    const float roughnessSquare = roughness * roughness;
    // Visibility
    const float V = GGX_Visibility(NdotL, NdotV, roughnessSquare);
    // Microfacet Distribution
    const float D = GGX_Distribution(NdotH, roughnessSquare);

    return V * D;
}

float3 Fresnel_Schlick(float3 f0, float VdotH)
{
    return f0 + (1.0 - f0) * pow(1.0 - abs(VdotH), 5.0);
}

float3 BRDF_PBR(ShadingSurfaceInfo shadingInfo, PBRMaterialComponents material)
{
    // Black color is the albedo of the metal materials
    const float3 albedo = lerp(material.BaseColor, float3(0.0, 0.0, 0.0) /* Black */, material.Metallic);
    // Reflect color at 0 degrees incident angle
    // 0.04 is the reflective color for dielectric materials
    const float3 f0 = lerp(0.04, material.BaseColor, material.Metallic);

    const float3 fresnel = Fresnel_Schlick(f0, shadingInfo.VdotH);
    const float3 diffuseContribution = BRDF_Diffuse_Lambertian(albedo);
    const float3 specularContribution = BRDF_Specular_GGX(shadingInfo.NdotL, shadingInfo.NdotV, shadingInfo.NdotH, material.Roughness);

    return (1 - fresnel) * diffuseContribution + fresnel * specularContribution;
}
