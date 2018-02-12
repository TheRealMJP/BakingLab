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
    float3 CameraPosWS;
}

//=================================================================================================
// Resources
//=================================================================================================
TextureCubeArray<float4> ProbeRadianceCubeMap : register(t0);
TextureCubeArray<float2> ProbeDistanceCubeMap : register(t1);
SamplerState LinearSampler : register(s0);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSOutput
{
    float4 PositionCS       : SV_Position;
    float3 NormalWS         : NORMALWS;
    uint ProbeIdx           : PROBEIDX;
    float3 PositionWS       : POSITIONWS;
};

struct PSInput
{
    float4 PositionSS       : SV_Position;
    float3 NormalWS         : NORMALWS;
    uint ProbeIdx           : PROBEIDX;
    float3 PositionWS       : POSITIONWS;
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
    output.PositionWS = vtxPositionWS;

    return output;
}

//=================================================================================================
// Pixel shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target0
{
    float3 normalWS = normalize(input.NormalWS);
    float3 viewDirWS = normalize(input.PositionWS - CameraPosWS);
    float3 reflectDirWS = reflect(viewDirWS, normalWS);

    float3 output = 0.0f;
    output = ProbeRadianceCubeMap.Sample(LinearSampler, float4(reflectDirWS, input.ProbeIdx)).xyz;

    return float4(output, 1.0f);
}