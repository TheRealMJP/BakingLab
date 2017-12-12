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

RWTexture3D<float4> VoxelRadiance;

[numthreads(4, 4, 4)]
void ClearVoxelRadiance(in uint3 voxelCoord : SV_DispatchThreadID)
{
    VoxelRadiance[voxelCoord] = 0.0f;
}

[numthreads(8, 8, 1)]
void FillVoxelHoles(in uint2 planeCoord : SV_DispatchThreadID)
{
    #if Axis_ == 0
        uint3 voxelCoord = uint3(0, planeCoord.x, planeCoord.y);
        const uint3 marchDir = uint3(1, 0, 0);
        const uint voxelsToMarch = VoxelResX;
    #elif Axis_ == 1
        uint3 voxelCoord = uint3(planeCoord.x, 0, planeCoord.y);
        const uint3 marchDir = uint3(0, 1, 0);
        const uint voxelsToMarch = VoxelResY;
    #else
        uint3 voxelCoord = uint3(planeCoord.x, planeCoord.y, 0);
        const uint3 marchDir = uint3(0, 0, 1);
        const uint voxelsToMarch = VoxelResZ;
    #endif

    bool inside = false;

    for(uint v = 0; v < voxelsToMarch; ++v)
    {
        float4 voxelSample = VoxelRadiance[voxelCoord];

        if(voxelSample.w > 0.0f)
        {
            inside = !inside;
            voxelSample.w = saturate(voxelSample.w);
        }
        else
        {
            voxelSample.xyz = 0.0f;
            voxelSample.w += inside ? -1.0f : 0.0f;
        }

        VoxelRadiance[voxelCoord] = voxelSample;
        voxelCoord += marchDir;
    }

    if(inside)
    {
        voxelCoord -= marchDir;
        for(uint i = 0; i < voxelsToMarch; ++i)
        {
            float4 voxelSample = VoxelRadiance[voxelCoord];
            if(voxelSample.w > 0.0f)
                break;
            else
                voxelSample = 0.0f;

            VoxelRadiance[voxelCoord] = voxelSample;
            voxelCoord -= marchDir;
        }
    }

    #if Axis_ == 2
        voxelCoord = uint3(planeCoord.x, planeCoord.y, 0);
        for(uint i = 0; i < voxelsToMarch; ++i)
        {
            float4 voxelSample = VoxelRadiance[voxelCoord];
            if(voxelSample.w < 0.0f)
            {
                voxelSample = 0.0f;

                if(voxelSample.w == -3.0)
                {
                    // voxelSample.xyz = float3(0, 10.0f, 0.0f);
                    voxelSample.w = 1.0f;
                }

                VoxelRadiance[voxelCoord] = voxelSample;
            }
            voxelCoord += marchDir;
        }
    #endif
}