//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Includes
//=================================================================================================
#include <Constants.hlsl>
#include "EVSM.hlsl"
#include <SH.hlsl>
#include "AppSettings.hlsl"
#include "SG.hlsl"

//=================================================================================================
// Constants
//=================================================================================================
static const uint NumCascades = 4;

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer VSConstants : register(b0)
{
    float4x4 World;
	float4x4 View;
    float4x4 WorldViewProjection;
    float4x4 PrevWorldViewProjection;
}

cbuffer PSConstants : register(b1)
{
    float3 SunDirectionWS;
    float CosSunAngularRadius;
    float3 SunIlluminance;
    float SinSunAngularRadius;
    float3 CameraPosWS;
	float4x4 ShadowMatrix;
	float4 CascadeSplits;
    float4 CascadeOffsets[NumCascades];
    float4 CascadeScales[NumCascades];
    float OffsetScale;
    float PositiveExponent;
    float NegativeExponent;
    float LightBleedingReduction;
    float2 RTSize;
    float2 JitterOffset;
    float4 SGDirections[MaxSGCount];
    float SGSharpness;
    float3 SceneMinBounds;
    float3 SceneMaxBounds;
}

//=================================================================================================
// Resources
//=================================================================================================
Texture2D<float4> AlbedoMap : register(t0);
Texture2D<float2> NormalMap : register(t1);
Texture2D<float> RoughnessMap : register(t2);
Texture2D<float> MetallicMap : register(t3);
Texture2DArray SunShadowMap : register(t4);
Texture2DArray<float4> BakedLightingMap : register(t5);
TextureCube<float> AreaLightShadowMap : register(t6);
Texture3D<float4> SHSpecularLookupA : register(t7);
Texture3D<float2> SHSpecularLookupB : register(t8);
TextureCubeArray<float4> ProbeIrradianceCubeMaps : register(t9);
TextureCubeArray<float2> ProbeDistanceCubeMaps : register(t10);
Texture3D<float4> VoxelRadiance : register(t11);
Texture3D<float4> VoxelRadianceMips[6] : register(t12);
Texture3D<float4> ProbeVolumeMaps[MaxBasisCount];
Texture3D<float2> ProbeDistanceVolumeMaps[MaxBasisCount];

SamplerState AnisoSampler : register(s0);
SamplerState EVSMSampler : register(s1);
SamplerState LinearSampler : register(s2);
SamplerComparisonState PCFSampler : register(s3);
SamplerState PointSampler : register(s4);

#if Voxelize_
    RasterizerOrderedTexture3D<float4> VoxelRadianceOutput : register(u0);
#endif

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSInput
{
    float3 PositionOS 		    : POSITION;
    float3 NormalOS 		    : NORMAL;
    float2 TexCoord 		    : TEXCOORD0;
    float2 LightMapUV           : TEXCOORD1;
	float3 TangentOS 		    : TANGENT;
	float3 BitangentOS		    : BITANGENT;
};

struct VSOutput
{
    float4 PositionCS 		    : SV_Position;

    float3 NormalWS 		    : NORMALWS;
    float3 PositionWS           : POSITIONWS;
    float DepthVS               : DEPTHVS;
	float3 TangentWS 		    : TANGENTWS;
	float3 BitangentWS 		    : BITANGENTWS;
	float2 TexCoord 		    : TEXCOORD;
    float2 LightMapUV           : LIGHTMAPUV;
    float3 PrevPosition         : PREVPOSITION;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;

    float3 NormalWS 		    : NORMALWS;
    float3 PositionWS           : POSITIONWS;
    float DepthVS               : DEPTHVS;
    float3 TangentWS 		    : TANGENTWS;
	float3 BitangentWS 		    : BITANGENTWS;
    float2 TexCoord 		    : TEXCOORD;
    float2 LightMapUV           : LIGHTMAPUV;
    float3 PrevPosition         : PREVPOSITION;
};

#if Voxelize_
    #define PSOutput void
#else
    struct PSOutput
    {
        float4 Lighting             : SV_Target0;
        #if ProbeRendering_
            float2 ProbeDistance    : SV_Target1;
        #else
            float2 Velocity         : SV_Target1;
        #endif
    };
#endif

//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input, in uint VertexID : SV_VertexID)
{
    VSOutput output;

    float3 positionOS = input.PositionOS;

    // Calc the world-space position
    output.PositionWS = mul(float4(positionOS, 1.0f), World).xyz;

    // Calc the clip-space position
    output.PositionCS = mul(float4(positionOS, 1.0f), WorldViewProjection);

    #if Voxelize_
        output.DepthVS = output.PositionWS.z - SceneMinBounds.z;
    #else
        output.DepthVS = output.PositionCS.w;
    #endif

	// Rotate the normal into world space
    output.NormalWS = normalize(mul(input.NormalOS, (float3x3)World));

	// Rotate the rest of the tangent frame into world space
	output.TangentWS = normalize(mul(input.TangentOS, (float3x3)World));
	output.BitangentWS = normalize(mul(input.BitangentOS, (float3x3)World));

    // Pass along the texture coordinates
    output.TexCoord = input.TexCoord;
    output.LightMapUV = input.LightMapUV;

    output.PrevPosition = mul(float4(input.PositionOS, 1.0f), PrevWorldViewProjection).xyw;

    return output;
}

//-------------------------------------------------------------------------------------------------
// Samples the EVSM shadow map
//-------------------------------------------------------------------------------------------------
float SampleShadowMapEVSM(in float3 shadowPos, in float3 shadowPosDX,
                          in float3 shadowPosDY, uint cascadeIdx)
{
    float2 exponents = GetEVSMExponents(PositiveExponent, NegativeExponent,
                                        CascadeScales[cascadeIdx].xyz);
    float2 warpedDepth = WarpDepth(shadowPos.z, exponents);

    float4 occluder = SunShadowMap.SampleGrad(EVSMSampler, float3(shadowPos.xy, cascadeIdx),
                                            shadowPosDX.xy, shadowPosDY.xy);

    // Derivative of warping at depth
    float2 depthScale = 0.0001f * exponents * warpedDepth;
    float2 minVariance = depthScale * depthScale;

    float posContrib = ChebyshevUpperBound(occluder.xz, warpedDepth.x, minVariance.x, LightBleedingReduction);
    float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y, LightBleedingReduction);
    float shadowContrib = posContrib;
    shadowContrib = min(shadowContrib, negContrib);

    return shadowContrib;
}

//-------------------------------------------------------------------------------------------------
// Samples the appropriate shadow map cascade
//-------------------------------------------------------------------------------------------------
float3 SampleShadowCascade(in float3 shadowPosition, in float3 shadowPosDX,
                           in float3 shadowPosDY, in uint cascadeIdx)
{
    shadowPosition += CascadeOffsets[cascadeIdx].xyz;
    shadowPosition *= CascadeScales[cascadeIdx].xyz;

    shadowPosDX *= CascadeScales[cascadeIdx].xyz;
    shadowPosDY *= CascadeScales[cascadeIdx].xyz;

    float3 cascadeColor = 1.0f;

    float shadow = SampleShadowMapEVSM(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);

    return shadow * cascadeColor;
}

//--------------------------------------------------------------------------------------
// Computes the sun visibility term by performing the shadow test
//--------------------------------------------------------------------------------------
float3 SunShadowVisibility(in float3 positionWS, in float depthVS)
{
	float3 shadowVisibility = 1.0f;
	uint cascadeIdx = 0;

    // Project into shadow space
    float3 samplePos = positionWS;
	float3 shadowPosition = mul(float4(samplePos, 1.0f), ShadowMatrix).xyz;
    float3 shadowPosDX = ddx(shadowPosition);
    float3 shadowPosDY = ddy(shadowPosition);

	// Figure out which cascade to sample from
	[unroll]
	for(uint i = 0; i < NumCascades - 1; ++i)
	{
		[flatten]
		if(depthVS > CascadeSplits[i])
			cascadeIdx = i + 1;
	}

	shadowVisibility = SampleShadowCascade(shadowPosition, shadowPosDX, shadowPosDY,
                                           cascadeIdx);

	return shadowVisibility;
}

//-------------------------------------------------------------------------------------------------
// Computes the area light shadow visibility term
//-------------------------------------------------------------------------------------------------
float AreaLightShadowVisibility(in float3 positionWS)
{
    float3 shadowPos = positionWS - float3(AreaLightX, AreaLightY, AreaLightZ);
    float3 shadowDistance = length(shadowPos);
    float3 shadowDir = normalize(shadowPos);

    // Doing the max of the components tells us 2 things: which cubemap face we're going to use,
    // and also what the projected distance is onto the major axis for that face.
    float projectedDistance = max(max(abs(shadowPos.x), abs(shadowPos.y)), abs(shadowPos.z));

    // Compute the project depth value that matches what would be stored in the depth buffer
    // for the current cube map face. Note that we use a reversed infinite projection.
    float nearClip = AreaLightSize;
    float a = 0.0f;
    float b = nearClip;
    float z = projectedDistance * a + b;
    float dbDistance = z / projectedDistance;

    return AreaLightShadowMap.SampleCmpLevelZero(PCFSampler, shadowDir, dbDistance + AreaLightShadowBias);
}

//-------------------------------------------------------------------------------------------------
// Calculates the lighting result for an analytical light source
//-------------------------------------------------------------------------------------------------
float3 CalcLighting(in float3 normal, in float3 lightDir, in float3 lightColor,
					in float3 diffuseAlbedo, in float3 specularAlbedo, in float roughness,
					in float3 positionWS, inout float3 irradiance)
{
    float3 lighting = diffuseAlbedo * (1.0f / 3.14159f);

    float3 view = normalize(CameraPosWS - positionWS);
    const float nDotL = saturate(dot(normal, lightDir));
    if(nDotL > 0.0f)
    {
        const float nDotV = saturate(dot(normal, view));
        float3 h = normalize(view + lightDir);

        float3 fresnel = Fresnel(specularAlbedo, h, lightDir);

        float specular = GGX_Specular(roughness, normal, h, view, lightDir);
        lighting += specular * fresnel;
    }

    irradiance += nDotL * lightColor;
    return lighting * nDotL * lightColor;
}

// ------------------------------------------------------------------------------------------------
// Computes the irradiance for an SG light source using the selected approximation
// ------------------------------------------------------------------------------------------------
float3 SGIrradiance(in SG lightingLobe, in float3 normal, in int diffuseMode)
{
    if(diffuseMode == SGDiffuseModes_Punctual)
        return SGIrradiancePunctual(lightingLobe, normal);
    else if(diffuseMode == SGDiffuseModes_Fitted)
        return SGIrradianceFitted(lightingLobe, normal);
    else
        return SGIrradianceInnerProduct(lightingLobe, normal);
}

// ------------------------------------------------------------------------------------------------
// Computes the specular contribution from an SG light source
// ------------------------------------------------------------------------------------------------
float3 SpecularTermSG(in SG light, in float3 normal, in float roughness,
                          in float3 view, in float3 specAlbedo)
{
    if(UseASGWarp)
        return SpecularTermASGWarp(light, normal, roughness, view, specAlbedo);
    else
        return SpecularTermSGWarp(light, normal, roughness, view, specAlbedo);
}

// ------------------------------------------------------------------------------------------------
// Determine the exit radiance towards the eye from the SG's stored in the lightmap
// ------------------------------------------------------------------------------------------------
void ComputeSGContribution(in Texture2DArray<float4> bakedLightingMap, in float2 lightMapUV, in float3 normalTS,
                          in float3 specularAlbedo, in float roughness, in float3 viewTS, in uint numSGs,
                          out float3 irradiance, out float3 specular)
{
    irradiance = 0.0f;
    specular = 0.0f;

    for(uint i = 0; i < numSGs; ++i)
    {
        SG sg;
        sg.Amplitude = bakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, i), 0.0f).xyz;
        sg.Axis = SGDirections[i].xyz;
        sg.Sharpness = SGSharpness;

        irradiance += SGIrradiance(sg, normalTS, SGDiffuseMode);
        specular += SpecularTermSG(sg, normalTS, roughness, viewTS, specularAlbedo);
    }
}

SH9Color GetSHSpecularBRDF(in float3 viewTS, in float3x3 tangentToWorld, in float3 normalTS,
                           in float3 specularAlbedo, in float sqrtRoughness)
{
    // Make a local coordinate frame in tangent space, with the x-axis
    // aligned with the view direction and the z-axis aligned with the normal
    float3 zBasis = normalTS;
    float3 yBasis = normalize(cross(zBasis, viewTS));
    float3 xBasis = normalize(cross(yBasis, zBasis));
    float3x3 localFrame = float3x3(xBasis, yBasis, zBasis);
    float viewAngle = saturate(dot(normalTS, viewTS));

    // Look up coefficients from the SH lookup texture to make the SH BRDF
    SH9Color shBrdf = (SH9Color)0.0f;

    [unroll]
    for(uint i = 0; i < 3; ++i)
    {
        float4 t0 = SHSpecularLookupA.SampleLevel(LinearSampler, float3(viewAngle, sqrtRoughness, specularAlbedo[i]), 0.0f);
        float2 t1 = SHSpecularLookupB.SampleLevel(LinearSampler, float3(viewAngle, sqrtRoughness, specularAlbedo[i]), 0.0f);
        shBrdf.c[0][i] = t0.x;
        shBrdf.c[2][i] = t0.y;
        shBrdf.c[3][i] = t0.z;
        shBrdf.c[6][i] = t0.w;
        shBrdf.c[7][i] = t1.x;
        shBrdf.c[8][i] = t1.y;
    }

    // Transform the SH BRDF to tangent space
    return RotateSH9(shBrdf, localFrame);
}

struct SurfaceContext
{
    float3 ViewWS;
    float3x3 TangentToWorld;
    float3 NormalTS;
    float3 NormalWS;
    float3 VtxNormalWS;
    float3 PositionWS;
    float3 DiffuseAlbedo;
    float3 SpecularAlbedo;
    float SqrtRoughness;
    float Roughness;
};

void ComputeIndirectFromLightmap(in SurfaceContext surface, in float2 lightMapUV,
                                 out float3 indirectIrradiance, out float3 indirectSpecular)
{
    indirectIrradiance = 0.0f;
    indirectSpecular = 0.0f;

    float3 viewTS = mul(surface.ViewWS, transpose(surface.TangentToWorld));

    if(BakeMode == BakeModes_Diffuse)
    {
        indirectIrradiance = BakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, 0.0f), 0.0f).xyz * Pi;
    }
    else if(BakeMode == BakeModes_HL2)
    {
        const float3 BasisDirs[3] =
        {
            float3(-1.0f / sqrt(6.0f), -1.0f / sqrt(2.0f), 1.0f / sqrt(3.0f)),
            float3(-1.0f / sqrt(6.0f), 1.0f / sqrt(2.0f), 1.0f / sqrt(3.0f)),
            float3(sqrt(2.0f / 3.0f), 0.0f, 1.0f / sqrt(3.0f)),
        };

        float weightSum = 0.0f;

        [unroll]
        for(uint i = 0; i < 3; ++i)
        {
            float3 lightMap = BakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, i), 0.0f).xyz;
            indirectIrradiance += saturate(dot(surface.NormalTS, BasisDirs[i])) * lightMap;
        }
    }
    else if(BakeMode == BakeModes_SH4)
    {
        SH4Color shRadiance;

        [unroll]
        for(uint i = 0; i < 4; ++i)
            shRadiance.c[i] = BakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, i), 0.0f).xyz;

        indirectIrradiance = EvalSH4Irradiance(surface.NormalTS, shRadiance);

        SH4Color shSpecularBRDF = ConvertToSH4(GetSHSpecularBRDF(viewTS, surface.TangentToWorld, surface.NormalTS, surface.SpecularAlbedo, surface.SqrtRoughness));
        indirectSpecular = SHDotProduct(shSpecularBRDF, shRadiance);
    }
    else if(BakeMode == BakeModes_SH9)
    {
        SH9Color shRadiance;

        [unroll]
        for(uint i = 0; i < 9; ++i)
            shRadiance.c[i] = BakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, i), 0.0f).xyz;

        indirectIrradiance = EvalSH9Irradiance(surface.NormalTS, shRadiance);

        SH9Color shSpecularBRDF = GetSHSpecularBRDF(viewTS, surface.TangentToWorld, surface.NormalTS, surface.SpecularAlbedo, surface.SqrtRoughness);
        indirectSpecular = SHDotProduct(shSpecularBRDF, shRadiance);
    }
    else if(BakeMode == BakeModes_H4)
    {
        H4Color hbIrradiance;

        [unroll]
        for(uint i = 0; i < 4; ++i)
            hbIrradiance.c[i] = BakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, i), 0.0f).xyz;

        indirectIrradiance = EvalH4(surface.NormalTS, hbIrradiance);
    }
    else if(BakeMode == BakeModes_H6)
    {
        H6Color hbIrradiance;

        [unroll]
        for(uint i = 0; i < 6; ++i)
            hbIrradiance.c[i] = BakedLightingMap.SampleLevel(LinearSampler, float3(lightMapUV, i), 0.0f).xyz;

        indirectIrradiance = EvalH6(surface.NormalTS, hbIrradiance);
    }
    else if(BakeMode == BakeModes_SG5)
    {
        ComputeSGContribution(BakedLightingMap, lightMapUV, surface.NormalTS, surface.SpecularAlbedo, surface.Roughness,
                              viewTS, 5, indirectIrradiance, indirectSpecular);
    }
    else if(BakeMode == BakeModes_SG6)
    {
        ComputeSGContribution(BakedLightingMap, lightMapUV, surface.NormalTS, surface.SpecularAlbedo, surface.Roughness,
                              viewTS, 6, indirectIrradiance, indirectSpecular);
    }
    else if(BakeMode == BakeModes_SG9)
    {
        ComputeSGContribution(BakedLightingMap, lightMapUV, surface.NormalTS, surface.SpecularAlbedo, surface.Roughness,
                              viewTS, 9, indirectIrradiance, indirectSpecular);
    }
    else if(BakeMode == BakeModes_SG12)
    {
        ComputeSGContribution(BakedLightingMap, lightMapUV, surface.NormalTS, surface.SpecularAlbedo, surface.Roughness,
                              viewTS, 12, indirectIrradiance, indirectSpecular);
    }
}

void ComputeIndirectFromProbeCubeMaps(in SurfaceContext surface, out float3 indirectIrradiance, out float3 indirectSpecular)
{
    float3 normalizedSamplePos = saturate((surface.PositionWS - SceneMinBounds) / (SceneMaxBounds - SceneMinBounds));
    float3 probeDims = float3(ProbeResX, ProbeResY, ProbeResZ);

    float3 halfTexelSize = 0.5f / probeDims;
    float3 baseSamplePos = saturate(normalizedSamplePos - halfTexelSize) * probeDims;
    float3 lerpAmts = frac(baseSamplePos);

    float3 irradianceSum = 0.0f;
    float weightSum = 0.0f;
    for(uint i = 0; i < 8; ++i)
    {
        uint3 sampleOffset = uint3(i, i >> 1, i >> 2) & 0x1;
        uint3 probeIndices = min(uint3(baseSamplePos) + sampleOffset, probeDims - 1.0f);
        float probeIdx = uint(probeIndices.z) * (ProbeResX * ProbeResY) + uint(probeIndices.y) * ProbeResX + uint(probeIndices.x);
        float3 triLinear = lerp(1.0f - lerpAmts, lerpAmts, float3(sampleOffset));
        float sampleWeight = triLinear.x * triLinear.y * triLinear.z;

        float3 probePosWS = lerp(SceneMinBounds, SceneMaxBounds, (probeIndices + 0.5f) / probeDims);
        float3 dirToProbe = normalize(probePosWS - surface.PositionWS);
        float distToProbe = length(probePosWS - surface.PositionWS);

        if(WeightProbesByNormal)
            sampleWeight *= max(0.05f, dot(dirToProbe, surface.VtxNormalWS));

        if(WeightProbesByVisibility)
        {
            float maxDistance = length((SceneMaxBounds - SceneMinBounds) * rcp(float3(ProbeResX, ProbeResY, ProbeResZ)));
            float compareDistance = saturate(distToProbe * rcp(maxDistance));

            float2 distSample = ProbeDistanceCubeMaps.Sample(LinearSampler, float4(-dirToProbe, probeIdx));
            float visTerm = ChebyshevUpperBound(distSample, compareDistance, 0.0001f, 0.25f);
            sampleWeight *= visTerm;
        }

        sampleWeight = max(0.0002f, sampleWeight);

        float3 irradianceSample = ProbeIrradianceCubeMaps.Sample(LinearSampler, float4(surface.NormalWS, probeIdx)).xyz;
        irradianceSum += irradianceSample * sampleWeight;
        weightSum += sampleWeight;
    }

    irradianceSum *= 1.0f / weightSum;
    // irradianceSum *= 1.0f / max(weightSum, 0.0002f);

    indirectIrradiance = irradianceSum;
    indirectSpecular = 0.0f;
}

float3 EvaluateProbeIrradiance(in float3 basis[MaxBasisCount], in float3 dir)
{
    float3 output = 0.0f;

    if(ProbeMode == ProbeModes_AmbientCube)
    {
        float3 ambientCubeDirs[6] = { float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1), float3(-1, 0, 0), float3(0, -1, 0), float3(0, 0, -1) };
        float weightSum = 0.0f;

        [unroll]
        for(uint i = 0; i < 6; ++i)
        {
            float weight = saturate(dot(dir, ambientCubeDirs[i]));
            output += basis[i] * weight;
            weightSum += weight;
        }

        output /= weightSum;
    }
    else if(ProbeMode == ProbeModes_L1_SH)
    {
        SH4Color sh;

        [unroll]
        for(uint i = 0; i < 4; ++i)
            sh.c[i] = basis[i];

        output = EvalSH4Irradiance(dir, sh);
    }
    else if(ProbeMode == ProbeModes_L2_SH)
    {
        SH9Color sh;

        [unroll]
        for(uint i = 0; i < 9; ++i)
            sh.c[i] = basis[i];


        output = EvalSH9Irradiance(dir, sh);
    }

    return output;
}


void SampleVolumeTextures(in SurfaceContext surface, in float3 uvw, out float3 basis[MaxBasisCount])
{
    if(ProbeMode == ProbeModes_AmbientCube)
    {
        [unroll]
        for(uint i = 0; i < 6; ++i)
            basis[i] = ProbeVolumeMaps[i].SampleLevel(LinearSampler, uvw, 0.0f).xyz;
    }
    else if(ProbeMode == ProbeModes_L1_SH)
    {
        [unroll]
        for(uint i = 0; i < 4; ++i)
            basis[i] = ProbeVolumeMaps[i].SampleLevel(LinearSampler, uvw, 0.0f).xyz;
    }
    else if(ProbeMode == ProbeModes_L2_SH)
    {
        [unroll]
        for(uint i = 0; i < 9; ++i)
            basis[i] = ProbeVolumeMaps[i].SampleLevel(LinearSampler, uvw, 0.0f).xyz;
    }
}

void ComputeIndirectFromProbeVolumeMaps(in SurfaceContext surface, out float3 indirectIrradiance, out float3 indirectSpecular)
{
    float3 normalizedSamplePos = saturate((surface.PositionWS - SceneMinBounds) / (SceneMaxBounds - SceneMinBounds));
    float3 uvw = normalizedSamplePos;

    indirectIrradiance = 0.0f;
    indirectSpecular = 0.0f;

    float3 basis[MaxBasisCount];

    [unroll]
    for(uint i = 0; i < uint(MaxBasisCount); ++i)
        basis[i] = 0.0f;

    SampleVolumeTextures(surface, uvw, basis);

    indirectIrradiance = EvaluateProbeIrradiance(basis, surface.NormalWS);
}

// Convert to normalized [0, 1] space mapping to the voxel grid
float3 ToVoxelSpace(in float3 x)
{
    return (x - SceneMinBounds) / (SceneMaxBounds - SceneMinBounds);
}

float3 ComputeVoxelAO(in SurfaceContext surface)
{
    const float3 coneDirections[6] =
    {
        float3(0, 0, 1),
        float3(0, 0.866f, 0.5f),
        float3(0.8236f, 0.2676f, 0.5f),
        float3(0.51f, -0.7f, 0.5f),
        float3(-0.5f, -0.7f, 0.5f),
        float3(-0.82f, 0.5f, 0.267f),
    };

    const float coneWeights[6] = { Pi / 4, (3 * Pi) / 20, (3 * Pi) / 20, (3 * Pi) / 20, (3 * Pi) / 20, (3 * Pi) / 20 };

    const float coneSpread = 0.325f;

    const float3 voxelRes = float3(VoxelResX, VoxelResY, VoxelResZ) * 0.5f;
    const float3 voxelTexelSize = rcp(voxelRes);

    const float MaxMipLevel = 9.0f;

    float3 occlusion = 0.0f;
    float weightSum = 0.0f;

    [unroll]
    for(uint coneIdx = 0; coneIdx < 6; ++ coneIdx)
    {
        const float3 coneDirWS = mul(coneDirections[coneIdx], surface.TangentToWorld);
        const float3 startPosWS = surface.PositionWS + surface.VtxNormalWS * 0.01f;

        const float3 startPosVS = ToVoxelSpace(startPosWS);
        const float3 coneDirVS = normalize(ToVoxelSpace(startPosWS + coneDirWS) - startPosVS);

        float traceDistance = 0.01f;

        float coneOcclusion = 0.0f;

        const uint MaxIterations = 32;
        uint numIterations = 0;

        while(coneOcclusion < 1.0f && numIterations < MaxIterations)
        {
            const float3 tracePos = startPosVS + traceDistance * coneDirVS;
            if(any(tracePos > 1.0f) || any(abs(tracePos < 0.0f)))
                break;

            const float3 uvw = tracePos;

            const float mipLevel = log2(1.0f + coneSpread * 2.0f * traceDistance / voxelTexelSize.x);
            const float mipLevel2 = (mipLevel + 1) * (mipLevel + 1);

            float4 voxelSample = 0.0f;
            float3 weights = coneDirVS * coneDirVS;

            [unroll]
            for(uint i = 0; i < 3; ++i)
            {
                if(coneDirVS[i] >= 0.0f)
                    voxelSample += VoxelRadianceMips[i * 2 + 0].SampleLevel(LinearSampler, uvw, min(mipLevel, MaxMipLevel)) * weights[i];
                else
                    voxelSample += VoxelRadianceMips[i * 2 + 1].SampleLevel(LinearSampler, uvw, min(mipLevel, MaxMipLevel)) * weights[i];
            }

            // voxelSample = VoxelRadiance.SampleLevel(LinearSampler, uvw, 0.0f);

            coneOcclusion += (1.0f - coneOcclusion) * voxelSample.w;

            traceDistance += mipLevel2 * voxelTexelSize.x;

            ++numIterations;
        }

        occlusion += coneOcclusion * coneWeights[coneIdx];
        weightSum += coneWeights[coneIdx];

        // occlusion = traceDistance;
        // occlusion = numIterations;

        // occlusion = VoxelRadiance.SampleLevel(LinearSampler, startPosVS, 0.0f).w;
    }

    occlusion /= weightSum;

    occlusion = saturate(1.0f - occlusion);

    return occlusion;
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
PSOutput PS(in PSInput input, in bool isFrontFace : SV_IsFrontFace)
{
    SurfaceContext surface;
	surface.VtxNormalWS = normalize(input.NormalWS);
    surface.NormalWS = surface.VtxNormalWS;
    surface.PositionWS = input.PositionWS;
    surface.ViewWS = normalize(CameraPosWS - surface.PositionWS);

    float2 uv = input.TexCoord;

	float3 normalTS = float3(0, 0, 1);
	float3 tangentWS = normalize(input.TangentWS);
	float3 bitangentWS = normalize(input.BitangentWS);
	float3x3 tangentToWorld = float3x3(tangentWS, bitangentWS, surface.VtxNormalWS);

    if(EnableNormalMaps)
    {
        // Sample the normal map, and convert the normal to world space
        normalTS.xy = NormalMap.Sample(AnisoSampler, uv).xy * 2.0f - 1.0f;
        normalTS.z = sqrt(1.0f - saturate(normalTS.x * normalTS.x + normalTS.y * normalTS.y));
        normalTS = lerp(float3(0, 0, 1), normalTS, NormalMapIntensity);
        surface.NormalWS = normalize(mul(normalTS, tangentToWorld));
    }

    surface.NormalTS = normalTS;
    surface.TangentToWorld = tangentToWorld;

    // Gather material parameters
    float3 albedoMap = 1.0f;

    if(EnableAlbedoMaps)
        albedoMap = AlbedoMap.Sample(AnisoSampler, uv).xyz;

    float metallic = saturate(MetallicMap.Sample(AnisoSampler, uv));
    surface.DiffuseAlbedo = lerp(albedoMap.xyz, 0.0f, metallic) * DiffuseAlbedoScale * EnableDiffuse;
    surface.SpecularAlbedo = lerp(0.03f, albedoMap.xyz, metallic) * EnableSpecular;

    surface.SqrtRoughness = RoughnessMap.Sample(AnisoSampler, uv);
    surface.SqrtRoughness *= RoughnessScale;
    surface.Roughness = surface.SqrtRoughness * surface.SqrtRoughness;

    float depthVS = input.DepthVS;

    // Add in the primary directional light
    float3 lighting = 0.0f;
    float3 irradiance = 0.0f;

    if(EnableDirectLighting && EnableSun)
    {
        float3 sunIrradiance = 0.0f;
        float3 sunShadowVisibility = SunShadowVisibility(surface.PositionWS, depthVS);
        float3 sunDirection = SunDirectionWS;
        if(SunAreaLightApproximation)
        {
            float3 D = SunDirectionWS;
            float3 R = reflect(-surface.ViewWS, surface.NormalWS);
            float r = SinSunAngularRadius;
            float d = CosSunAngularRadius;
            float3 DDotR = dot(D, R);
            float3 S = R - DDotR * D;
            sunDirection = DDotR < d ? normalize(d * D + normalize(S) * r) : R;
        }
        lighting += CalcLighting(surface.NormalWS, sunDirection, SunIlluminance, surface.DiffuseAlbedo, surface.SpecularAlbedo,
                                 surface.Roughness, surface.PositionWS, sunIrradiance) * sunShadowVisibility;
        irradiance += sunIrradiance * sunShadowVisibility;
    }

    if(EnableDirectLighting && EnableAreaLight && BakeDirectAreaLight == false)
    {
        float3 areaLightPos = float3(AreaLightX, AreaLightY, AreaLightZ);
        float3 areaLightDir = normalize(areaLightPos - surface.PositionWS);
        float areaLightDist = length(areaLightPos - surface.PositionWS);
        SG lightLobe = MakeSphereSG(areaLightDir, AreaLightSize, AreaLightColor * FP16Scale, areaLightDist);
        float3 sgIrradiance = SGIrradiance(lightLobe, surface.NormalWS, SGDiffuseMode);

        float areaLightVisibility = 1.0f;
        if(EnableAreaLightShadows)
            areaLightVisibility = AreaLightShadowVisibility(surface.PositionWS);

        lighting += sgIrradiance * (surface.DiffuseAlbedo / Pi) * areaLightVisibility;
        lighting += SpecularTermSG(lightLobe, surface.NormalWS, surface.Roughness, surface.ViewWS, surface.SpecularAlbedo) * areaLightVisibility;
        irradiance += sgIrradiance * areaLightVisibility;
    }

	// Add in the indirect
    if((EnableIndirectLighting || ViewIndirectSpecular) && Voxelize_ == false)
    {
        float3 indirectIrradiance = 0.0f;
        float3 indirectSpecular = 0.0f;

        if(UseProbes || ProbeRendering_)
        {
            if(ProbeMode == ProbeModes_CubeMap)
                ComputeIndirectFromProbeCubeMaps(surface, indirectIrradiance, indirectSpecular);
            else
                ComputeIndirectFromProbeVolumeMaps(surface, indirectIrradiance, indirectSpecular);
        }
        else
            ComputeIndirectFromLightmap(surface, input.LightMapUV, indirectIrradiance, indirectSpecular);

        if(EnableIndirectDiffuse)
        {
            irradiance += indirectIrradiance;
            lighting += indirectIrradiance * (surface.DiffuseAlbedo / Pi);
        }

        if(EnableIndirectSpecular)
            lighting += indirectSpecular;

        if(ViewIndirectSpecular)
            lighting = indirectSpecular;
    }

    float illuminance = dot(irradiance, float3(0.2126f, 0.7152f, 0.0722f));

    #if Voxelize_
        float3 voxelRadiance = clamp(irradiance * surface.DiffuseAlbedo * InvPi, 0.0f, FP16Max);
        float3 voxelUVW = saturate((surface.PositionWS - SceneMinBounds) / (SceneMaxBounds - SceneMinBounds));
        int3 voxelCoord = int3(voxelUVW * float3(VoxelResX, VoxelResY, VoxelResZ));
        voxelCoord = clamp(voxelCoord, 0, int3(VoxelResX, VoxelResY, VoxelResZ) - 1);

        float3 prevVoxelRadiance = VoxelRadianceOutput[voxelCoord].xyz;
        float prevVoxelCount = VoxelRadianceOutput[voxelCoord].w;
        float3 newVoxelRadiance = lerp(prevVoxelRadiance, voxelRadiance, 1.0f / (prevVoxelCount + 1.0f));
        VoxelRadianceOutput[voxelCoord] = float4(newVoxelRadiance, prevVoxelCount + 1.0f);
    #else
        PSOutput output;
        output.Lighting = clamp(float4(lighting, illuminance), 0.0f, FP16Max);

        if(isFrontFace == false)
            output.Lighting = 0.0f;

        #if ProbeRendering_
            float distanceFromProbe = length(surface.PositionWS - CameraPosWS);
            float maxDistance = length((SceneMaxBounds - SceneMinBounds) * rcp(float3(ProbeResX, ProbeResY, ProbeResZ)));
            distanceFromProbe = saturate(distanceFromProbe * rcp(maxDistance));
            output.ProbeDistance = float2(distanceFromProbe, distanceFromProbe * distanceFromProbe);
        #else
            float2 prevPositionSS = (input.PrevPosition.xy / input.PrevPosition.z) * float2(0.5f, -0.5f) + 0.5f;
            prevPositionSS *= RTSize;
            output.Velocity = input.PositionSS.xy - prevPositionSS;
            output.Velocity -= JitterOffset;
            output.Velocity /= RTSize;

            output.Lighting.xyz = ComputeVoxelAO(surface) * 8;
        #endif

        return output;
    #endif
}
