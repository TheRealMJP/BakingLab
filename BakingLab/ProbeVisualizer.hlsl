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
#include "SharedConstants.h"
#include <SH.hlsl>
#include "AppSettings.hlsl"
#include "SG.hlsl"

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer Constants : register(b0)
{
    float4x4 ViewProjection;
    float4 SGDirections[MaxSGCount];
    float SGSharpness;
    float3 SceneMinBounds;
    float3 SceneMaxBounds;
}

//=================================================================================================
// Resources
//=================================================================================================
TextureCubeArray<float4> ProbeIrradianceCubeMap : register(t0);
TextureCubeArray<float2> ProbeDistanceCubeMap : register(t1);
Texture3D<float4> ProbeVolumeMaps[MaxBasisCount];
Texture3D<float2> ProbeDistanceVolumeMaps[MaxBasisCount];
SamplerState LinearSampler : register(s0);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSOutput
{
    float4 PositionCS       : SV_Position;
    float3 NormalWS         : NORMALWS;
    float3 ProbeUVW         : PROBEUVW;
    uint ProbeIdx           : PROBEIDX;
};

struct PSInput
{
    float4 PositionSS       : SV_Position;
    float3 NormalWS         : NORMALWS;
    float3 ProbeUVW         : PROBEUVW;
    uint ProbeIdx           : PROBEIDX;
};

//=================================================================================================
// Vertex shader
//=================================================================================================
VSOutput VS(in float3 SpherePosition : POSITION, in uint InstanceID : SV_InstanceID)
{
    VSOutput output;

    output.NormalWS = normalize(SpherePosition);
    output.ProbeIdx = InstanceID;

    uint probeX = InstanceID % ProbeResX;
    uint probeY = (InstanceID / ProbeResX) % ProbeResY;
    uint probeZ = InstanceID / (ProbeResX * ProbeResY);

    float3 probeUVW = (float3(probeX, probeY, probeZ) + 0.5f) / float3(ProbeResX, ProbeResY, ProbeResZ);
    float3 probePos = lerp(SceneMinBounds, SceneMaxBounds, probeUVW);

    float3 cellSizes = (SceneMaxBounds - SceneMinBounds) / float3(ProbeResX, ProbeResZ, ProbeResZ);
    float probeSize = 0.15f * min(min(cellSizes.x, cellSizes.y), cellSizes.z);

    float3 vtxPositionWS = (SpherePosition * probeSize) + probePos;
    output.PositionCS = mul(float4(vtxPositionWS, 1.0f), ViewProjection);
    output.ProbeUVW = probeUVW;

    return output;
}

//=================================================================================================
// Pixel shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target0
{
    float3 output = 0.0f;

    if(ProbeMode == ProbeModes_CubeMap)
    {
        output = ProbeIrradianceCubeMap.Sample(LinearSampler, float4(input.NormalWS, input.ProbeIdx)).xyz * InvPi;
    }
    else if(ProbeMode == ProbeModes_AmbientCube)
    {
        float3 ambientCubeDirs[6] = { float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1), float3(-1, 0, 0), float3(0, -1, 0), float3(0, 0, -1) };

        float weightSum = 0.0f;

        [unroll]
        for(uint i = 0; i < 6; ++i)
        {
            float weight = saturate(dot(input.NormalWS, ambientCubeDirs[i]));
            if(weight > 0.0f)
                output += ProbeVolumeMaps[i].SampleLevel(LinearSampler, input.ProbeUVW, 0.0f).xyz * weight;
            weightSum += weight;
        }

        output *= rcp(weightSum) * InvPi;
    }

    return float4(output, 1.0f);
}