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
#include "AppSettings.hlsl"

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer Constants : register(b0)
{
    float4x4 ViewProjection;
    float3 SceneMinBounds;
    float3 SceneMaxBounds;
}

Texture3D<float4> VoxelRadiance : register(t0);

struct VSOutput
{
    float4 PositionCS : SV_Position;
    float3 VoxelRadiance  : VOXELRADIANCE;
};

//=================================================================================================
// Vertex shader for "raw" voxel visualization
//=================================================================================================
VSOutput VS(in float3 VertexPosition : POSITION, in uint InstanceID : SV_InstanceID)
{
    const uint voxelIdx = InstanceID;
    const uint3 voxelCoord = uint3(voxelIdx % VoxelResX,
                                   (voxelIdx / VoxelResX) % VoxelResY,
                                   voxelIdx / (VoxelResX * VoxelResY));

    const float3 voxelRadiance = VoxelRadiance[voxelCoord].xyz;
    const float voxelOccupancy = VoxelRadiance[voxelCoord].w;

    if(voxelOccupancy <= 0.0f)
    {
        VSOutput output;
        output.PositionCS = float4(-10000.0f, -10000.0f, -10000.0f, 1.0f);
        output.VoxelRadiance = 0.0f;
        return output;
    }

    const float3 uvw = float3((voxelCoord.x + 0.5f) / VoxelResX,
                              (voxelCoord.y + 0.5f) / VoxelResY,
                              (voxelCoord.z + 0.5f) / VoxelResZ);

    const float3 sceneSize = (SceneMaxBounds - SceneMinBounds);
    const float3 voxelSize = sceneSize / float3(VoxelResX, VoxelResY, VoxelResZ);

    const float3 voxelCenter = SceneMinBounds + sceneSize * uvw;
    const float3 vtxPositionWS = voxelCenter + VertexPosition * voxelSize;

    VSOutput output;
    output.PositionCS = mul(float4(vtxPositionWS, 1.0f), ViewProjection);
    output.VoxelRadiance = voxelRadiance;

    return output;
}

//=================================================================================================
// Pixel shader for "raw" voxel visualization
//=================================================================================================
float4 PS(in VSOutput input) : SV_Target0
{
    return float4(input.VoxelRadiance, 1.0f);
}