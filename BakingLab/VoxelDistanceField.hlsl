//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "AppSettings.hlsl"

cbuffer DistanceFieldConstants : register(b0)
{
    int StepSize;
}

Texture3D<float4> VoxelRadiance : register(t0);
RWTexture3D<uint4> JumpFloodTexture : register(u0);
RWTexture3D<float> DistanceTexture : register(u1);

[numthreads(4, 4, 4)]
void InitJumpFlood(in uint3 voxelCoord : SV_DispatchThreadID)
{
    uint3 output = 0xFFFF;
    float4 voxelSample = VoxelRadiance[voxelCoord];
    if(voxelSample.w > 0.0f)
        output = voxelCoord;

    JumpFloodTexture[voxelCoord] = uint4(output, 0);
}

[numthreads(4, 4, 4)]
void JumpFloodIteration(in uint3 voxelCoord : SV_DispatchThreadID)
{
    uint3 closestPoint = 0xFFFF;
    float closestDistance = 1000000000.0f;

    for(int z = -1; z <= 1; ++z)
    {
        for(int y = -1; y <= 1; ++y)
        {
            for(int x = -1; x <= 1; ++x)
            {
                int3 offset = int3(x, y, z) * StepSize;
                int3 sampleCoord = int3(voxelCoord) + offset;
                sampleCoord = clamp(sampleCoord, 0, VoxelResolution - 1);

                uint3 sampleClosestPoint = JumpFloodTexture[sampleCoord].xyz;
                float sampleDistance = length(float3(sampleClosestPoint) - float3(voxelCoord));
                if(sampleClosestPoint.x != 0xFFFF && sampleDistance < closestDistance)
                {
                    closestPoint = sampleClosestPoint;
                    closestDistance = sampleDistance;
                }
            }
        }
    }

    JumpFloodTexture[voxelCoord] = uint4(closestPoint, 0);
}

[numthreads(4, 4, 4)]
void FillDistanceTexture(in uint3 voxelCoord : SV_DispatchThreadID)
{
    uint3 closestPoint = JumpFloodTexture[voxelCoord].xyz;
    float closestDistance = -1.0f;
    if(closestPoint.x != 0xFFFF)
        closestDistance = length(float3(voxelCoord) - float3(closestPoint));

    DistanceTexture[voxelCoord] = closestDistance;
}