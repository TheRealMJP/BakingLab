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
TextureCubeArray<float4> ProbeIrradiance : register(t0);
TextureCubeArray<float2> ProbeDistance : register(t1);
SamplerState LinearSampler : register(s0);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSOutput
{
    float4 PositionCS       : SV_Position;
    float3 NormalWS         : NORMALWS;
    uint ProbeIdx           : PROBEIDX;
};

struct PSInput
{
    float4 PositionSS       : SV_Position;
    float3 NormalWS         : NORMALWS;
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

    float3 probePos;
    probePos.x = lerp(SceneMinBounds.x, SceneMaxBounds.x, (probeX + 0.5f) / ProbeResX);
    probePos.y = lerp(SceneMinBounds.y, SceneMaxBounds.y, (probeY + 0.5f) / ProbeResY);
    probePos.z = lerp(SceneMinBounds.z, SceneMaxBounds.z, (probeZ + 0.5f) / ProbeResZ);

    float3 cellSizes = (SceneMaxBounds - SceneMinBounds) / float3(ProbeResX, ProbeResZ, ProbeResZ);
    float probeSize = 0.15f * min(min(cellSizes.x, cellSizes.y), cellSizes.z);

    float3 vtxPositionWS = (SpherePosition * probeSize) + probePos;
    output.PositionCS = mul(float4(vtxPositionWS, 1.0f), ViewProjection);

    return output;
}

//=================================================================================================
// Pixel shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target0
{
    float3 irradiance = ProbeIrradiance.Sample(LinearSampler, float4(input.NormalWS, input.ProbeIdx)).xyz;
    float2 distance = ProbeDistance.Sample(LinearSampler, float4(input.NormalWS, input.ProbeIdx));
    return float4(irradiance * InvPi, 0.0f);
    // return float4(distance * 0.01f, 0.0f, 1.0f);
}