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
RWTexture3D<uint4> JumpFloodTexturePositive : register(u0);
RWTexture3D<uint4> JumpFloodTextureNegative : register(u1);
RWTexture3D<float> DistanceTexture : register(u2);

[numthreads(4, 4, 4)]
void InitJumpFlood(in uint3 voxelCoord : SV_DispatchThreadID)
{
    uint3 negativeOutput = 0xFFFF;
    uint3 positiveOutput = 0xFFFF;

    float voxelCoverage = VoxelRadiance[voxelCoord].w;
    if(voxelCoverage > 0.0f)
        positiveOutput = voxelCoord;
    else
        negativeOutput = voxelCoord;

    JumpFloodTextureNegative[voxelCoord] = uint4(negativeOutput, 0);
    JumpFloodTexturePositive[voxelCoord] = uint4(positiveOutput, 0);
}

[numthreads(4, 4, 4)]
void JumpFloodIteration(in uint3 voxelCoord : SV_DispatchThreadID)
{
    uint3 closestNegativePoint = 0xFFFF;
    float closestNegativeDistance = 1000000000.0f;
    uint3 closestPositivePoint = 0xFFFF;
    float closestPositiveDistance = 1000000000.0f;

    for(int z = -1; z <= 1; ++z)
    {
        for(int y = -1; y <= 1; ++y)
        {
            for(int x = -1; x <= 1; ++x)
            {
                int3 offset = int3(x, y, z) * StepSize;
                int3 sampleCoord = int3(voxelCoord) + offset;
                sampleCoord = clamp(sampleCoord, 0, VoxelResolution - 1);

                uint3 positiveSampleClosestPoint = JumpFloodTexturePositive[sampleCoord].xyz;
                float positiveSampleDistance = length(float3(positiveSampleClosestPoint) - float3(voxelCoord));

                if(positiveSampleClosestPoint.x != 0xFFFF && positiveSampleDistance < closestPositiveDistance)
                {
                    closestPositivePoint = positiveSampleClosestPoint;
                    closestPositiveDistance = positiveSampleDistance;
                }

                uint3 negativeSampleClosestPoint = JumpFloodTextureNegative[sampleCoord].xyz;
                float negativeSampleDistance = length(float3(negativeSampleClosestPoint) - float3(voxelCoord));

                if(negativeSampleClosestPoint.x != 0xFFFF && negativeSampleDistance < closestNegativeDistance)
                {
                    closestNegativePoint = negativeSampleClosestPoint;
                    closestNegativeDistance = negativeSampleDistance;
                }
            }
        }
    }

    JumpFloodTextureNegative[voxelCoord] = uint4(closestNegativePoint, 0);
    JumpFloodTexturePositive[voxelCoord] = uint4(closestPositivePoint, 0);
}

float IntersectRayBox3D(float3 rayOrg, float3 dir, float3 bbmin, float3 bbmax)
{
    float3 invDir = rcp(dir);
    float3 d0 = (bbmin - rayOrg) * invDir;
    float3 d1 = (bbmax - rayOrg) * invDir;

    float3 v0 = min(d0, d1);
    float3 v1 = max(d0, d1);

    float tmin = max(v0.x, max(v0.y, v0.z));
    float tmax = min(v1.x, min(v1.y, v1.z));

    return tmin >= 0.0f ? tmin : tmax;
}

float IntersectRayBox2D(float2 rayOrg, float2 dir, float2 bbmin, float2 bbmax)
{
    float2 invDir = rcp(dir);
    float2 d0 = (bbmin - rayOrg) * invDir;
    float2 d1 = (bbmax - rayOrg) * invDir;

    float2 v0 = min(d0, d1);
    float2 v1 = max(d0, d1);

    float tmin = max(v0.x, v0.y);
    float tmax = min(v1.x, v1.y);

    return tmin >= 0.0f ? tmin : tmax;
}

float IntersectRayBox1D(float rayOrg, float dir, float bbmin, float bbmax)
{
    float invDir = rcp(dir);
    float d0 = (bbmin - rayOrg) * invDir;
    float d1 = (bbmax - rayOrg) * invDir;

    float v0 = min(d0, d1);
    float v1 = max(d0, d1);

    float tmin = v0;
    float tmax = v1;

    return tmin >= 0.0f ? tmin : tmax;
}

float DistanceToVoxel(in uint3 startCoord, in uint3 voxelCoord)
{
    float3 posVS = float3(startCoord);
    float3 dirVS = normalize(voxelCoord - posVS);
    float distToNextVoxel = IntersectRayBox3D(posVS, dirVS, float3(voxelCoord), voxelCoord + 1.0f);
    if(all(abs(dirVS.xy) < 0.01f))
        distToNextVoxel = IntersectRayBox1D(posVS.z, dirVS.z, float(voxelCoord.z), voxelCoord.z + 1.0f);
    else if(all(abs(dirVS.xz) < 0.01f))
        distToNextVoxel = IntersectRayBox1D(posVS.y, dirVS.y, float(voxelCoord.y), voxelCoord.y + 1.0f);
    else if(all(abs(dirVS.yz) < 0.01f))
        distToNextVoxel = IntersectRayBox1D(posVS.x, dirVS.x, float(voxelCoord.x), voxelCoord.x + 1.0f);
    else if(abs(dirVS.x) < 0.01f)
        distToNextVoxel = IntersectRayBox2D(posVS.yz, dirVS.yz, float2(voxelCoord.yz), voxelCoord.yz + 1.0f);
    else if(abs(dirVS.y) < 0.01f)
        distToNextVoxel = IntersectRayBox2D(posVS.xz, dirVS.xz, float2(voxelCoord.xz), voxelCoord.xz + 1.0f);
    else if(abs(dirVS.z) < 0.01f)
        distToNextVoxel = IntersectRayBox2D(posVS.xy, dirVS.xy, float2(voxelCoord.xy), voxelCoord.xy + 1.0f);

    distToNextVoxel = length(float3(startCoord) - float3(voxelCoord));

    return distToNextVoxel;
}

[numthreads(4, 4, 4)]
void FillDistanceTexture(in uint3 voxelCoord : SV_DispatchThreadID)
{
    uint3 closestPositivePoint = JumpFloodTexturePositive[voxelCoord].xyz;
    uint3 closestNegativePoint = JumpFloodTextureNegative[voxelCoord].xyz;
    float voxelCoverage = VoxelRadiance[voxelCoord].w;
    float closestDistance = 0.0f;

    if(voxelCoverage == 0.0f && closestPositivePoint.x != 0xFFFF)
        closestDistance = DistanceToVoxel(voxelCoord, closestPositivePoint);
    /*else if(voxelCoverage > 0.0f && closestNegativePoint.x != 0xFFFF)
        closestDistance = -DistanceToVoxel(voxelCoord, closestNegativePoint);*/

    DistanceTexture[voxelCoord] = closestDistance;
}