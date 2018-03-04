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
cbuffer VoxelBakeConstants : register(b0)
{
    uint BakeSampleStart;
    uint NumSamplesToBake;
    uint BasisCount;
    uint NumBakePoints;
    uint BakePointOffset;
    uint NumGutterTexels;

    SH9Color SkySH;

    float3 SceneMinBounds;
    float3 SceneMaxBounds;
}

//=================================================================================================
// Resources
//=================================================================================================
StructuredBuffer<BakePoint> BakePoints : register(t0);
Texture3D<float4> VoxelRadiance : register(t1);
TextureCube<float3> SkyRadiance : register(t2);
StructuredBuffer<GutterTexel> GutterTexels : register(t3);
RWTexture2DArray<float4> BakeResults : register(u0);
SamplerState PointSampler : register(s0);

uint CMJPermute(uint i, uint l, uint p)
{
    uint w = l - 1;
    w |= w >> 1;
    w |= w >> 2;
    w |= w >> 4;
    w |= w >> 8;
    w |= w >> 16;
    do
    {
        i ^= p; i *= 0xe170893d;
        i ^= p >> 16;
        i ^= (i & w) >> 4;
        i ^= p >> 8; i *= 0x0929eb3f;
        i ^= p >> 23;
        i ^= (i & w) >> 1; i *= 1 | p >> 27;
        i *= 0x6935fa69;
        i ^= (i & w) >> 11; i *= 0x74dcb303;
        i ^= (i & w) >> 2; i *= 0x9e501cc3;
        i ^= (i & w) >> 2; i *= 0xc860a3df;
        i &= w;
        i ^= i >> 5;
    }
    while (i >= l);
    return (i + p) % l;
}

float CMJRandFloat(uint i, uint p)
{
    i ^= p;
    i ^= i >> 17;
    i ^= i >> 10; i *= 0xb36534e5;
    i ^= i >> 12;
    i ^= i >> 21; i *= 0x93fc4795;
    i ^= 0xdf6e307f;
    i ^= i >> 17; i *= 1 | p >> 18;
    return i * (1.0f / 4294967808.0f);
}

 // Returns a 2D sample from a particular pattern using correlated multi-jittered sampling [Kensler 2013]
float2 SampleCMJ2D(uint sampleIdx, uint numSamplesX, uint numSamplesY, uint pattern)
{
    uint N = numSamplesX * numSamplesY;
    sampleIdx = CMJPermute(sampleIdx, N, pattern * 0x51633e2d);
    uint sx = CMJPermute(sampleIdx % numSamplesX, numSamplesX, pattern * 0x68bc21eb);
    uint sy = CMJPermute(sampleIdx / numSamplesX, numSamplesY, pattern * 0x02e5be93);
    float jx = CMJRandFloat(sampleIdx, pattern * 0x967a889b);
    float jy = CMJRandFloat(sampleIdx, pattern * 0x368cc8b7);
    return float2((sx + (sy + jx) / numSamplesY) / numSamplesX, (sampleIdx + jy) / N);
}

float2 SquareToConcentricDiskMapping(float x, float y)
{
    float phi = 0.0f;
    float r = 0.0f;

    // -- (a,b) is now on [-1,1]ˆ2
    float a = 2.0f * x - 1.0f;
    float b = 2.0f * y - 1.0f;

    if(a > -b)                      // region 1 or 2
    {
        if(a > b)                   // region 1, also |a| > |b|
        {
            r = a;
            phi = (Pi / 4.0f) * (b / a);
        }
        else                        // region 2, also |b| > |a|
        {
            r = b;
            phi = (Pi / 4.0f) * (2.0f - (a / b));
        }
    }
    else                            // region 3 or 4
    {
        if(a < b)                   // region 3, also |a| >= |b|, a != 0
        {
            r = -a;
            phi = (Pi / 4.0f) * (4.0f + (b / a));
        }
        else                        // region 4, |b| >= |a|, but a==0 and b==0 could occur.
        {
            r = -b;
            if(b != 0)
                phi = (Pi / 4.0f) * (6.0f - (a / b));
            else
                phi = 0;
        }
    }

    float2 result;
    result.x = r * cos(phi);
    result.y = r * sin(phi);
    return result;
}

// Returns a random direction on the hemisphere around z = 1
float3 SampleDirectionHemisphere(float u1, float u2)
{
    float z = u1;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float phi = 2 * Pi * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);

    return float3(x, y, z);
}

// Returns a random cosine-weighted direction on the hemisphere around z = 1
float3 SampleCosineHemisphere(float u1, float u2)
{
    float2 uv = SquareToConcentricDiskMapping(u1, u2);
    float u = uv.x;
    float v = uv.y;

    // Project samples on the disk to the hemisphere to get a
    // cosine weighted distribution
    float3 dir;
    float r = u * u + v * v;
    dir.x = u;
    dir.y = v;
    dir.z = sqrt(max(0.0f, 1.0f - r));

    return dir;
}

// Returns the final monte-carlo weighting factor using the PDF of a cosine-weighted hemisphere
float CosineWeightedMonteCarloFactor(uint numSamples)
{
    // Integrating the cosine factor about the hemisphere gives you Pi, and the PDF
    // of a cosine-weighted hemisphere function is 1 / Pi.
    // So the final monte-carlo weighting factor is 1 / NumSamples
    return (1.0f / numSamples);
}

float HemisphereMonteCarloFactor(uint numSamples)
{
    // The area of a unit hemisphere is 2 * Pi, so the PDF is 1 / (2 * Pi)
    return ((2.0f * Pi) / numSamples);
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

// TODO: optimize this
float DistancetoNextVoxel(in float3 posVS, in float3 dirVS, in float3 currVoxelMin, in float3 currVoxelMax)
{
    float distToNextVoxel = IntersectRayBox3D(posVS, dirVS, currVoxelMin, currVoxelMax);
    if(all(abs(dirVS.xy) < 0.01f))
        distToNextVoxel = IntersectRayBox1D(posVS.z, dirVS.z, currVoxelMin.z, currVoxelMax.z);
    else if(all(abs(dirVS.xz) < 0.01f))
        distToNextVoxel = IntersectRayBox1D(posVS.y, dirVS.y, currVoxelMin.y, currVoxelMax.y);
    else if(all(abs(dirVS.yz) < 0.01f))
        distToNextVoxel = IntersectRayBox1D(posVS.x, dirVS.x, currVoxelMin.x, currVoxelMax.x);
    else if(abs(dirVS.x) < 0.01f)
        distToNextVoxel = IntersectRayBox2D(posVS.yz, dirVS.yz, currVoxelMin.yz, currVoxelMax.yz);
    else if(abs(dirVS.y) < 0.01f)
        distToNextVoxel = IntersectRayBox2D(posVS.xz, dirVS.xz, currVoxelMin.xz, currVoxelMax.xz);
    else if(abs(dirVS.z) < 0.01f)
        distToNextVoxel = IntersectRayBox2D(posVS.xy, dirVS.xy, currVoxelMin.xy, currVoxelMax.xy);
    return distToNextVoxel;
}

float3 ToVoxelSpace(in float3 x)
{
    return (x - SceneMinBounds) / (SceneMaxBounds - SceneMinBounds);
}

float3 RayMarchVoxels(in float3 pos, in float3 dir, in float3 vtxNormal)
{
    const float voxelRes = float(VoxelResolution);
    const float voxelSize = 1.0f / voxelRes;

    // pos += dir * 0.1f;

    const float3 startPosVS = ToVoxelSpace(pos) * voxelRes;
    const float3 marchPosVS = ToVoxelSpace(pos + dir) * voxelRes;
    const float3 marchDirVS = normalize(marchPosVS - startPosVS);
    // const float bias = voxelSize * 0.25f;
    const float bias = 0.001f;

    float3 currPosVS = startPosVS;

    float3 normalPosVS = ToVoxelSpace(pos + vtxNormal) * voxelRes;
    float3 normalDirVS = normalize(normalPosVS - startPosVS);

    uint iteration = 0;
    while(VoxelRadiance.SampleLevel(PointSampler, currPosVS / voxelRes, 0.0f).w > 0.0f && iteration < 3)
    {
        const float distToNextVoxel = DistancetoNextVoxel(currPosVS, normalDirVS, floor(currPosVS), floor(currPosVS) + 1.0f);
        currPosVS += normalDirVS * (distToNextVoxel + bias);
        ++iteration;
    }

    // currPosVS += (0.5f + (1.0f - saturate(dot(vtxNormal, dir))) * 1.0f) * marchDirVS;

    const uint MaxSteps = 1024;

    float3 radiance = 0.0f;
    float opacity = 0.0f;

    for(uint i = 0; i < MaxSteps; ++i)
    {
        float3 uvw = currPosVS / voxelRes;
        if(any(abs(uvw * 2.0f - 1.0f) > 1.0001f) || opacity >= 0.999f)
            break;

        const float3 currVoxelMin = floor(currPosVS);
        const float3 currVoxelMax = currVoxelMin + 1.0f;
        const float3 currVoxelCenter = (currVoxelMin + currVoxelMax) / 2.0f;

        float4 voxelSample = VoxelRadiance.SampleLevel(PointSampler, uvw, 0.0f);
        voxelSample.w = saturate(voxelSample.w);
        // voxelSample.w = voxelSample.w > 0.0f ? 1.0f : 0.0f;

        radiance += (1.0f - opacity) * voxelSample.xyz;
        opacity += (1.0f - opacity) * voxelSample.w;

        const float distToNextVoxel = DistancetoNextVoxel(currPosVS, marchDirVS, currVoxelMin, currVoxelMax);
        currPosVS += marchDirVS * (distToNextVoxel + bias);
    }

    float3 skyRadiance = SkyRadiance.SampleLevel(PointSampler, dir, 0.0f);
    radiance += (1.0f - opacity) * skyRadiance;

    return radiance;
}

[numthreads(64, 1, 1)]
void VoxelBake(in uint dispatchThreadID : SV_DispatchThreadID)
{
    uint bakePointIdx = dispatchThreadID;
    bakePointIdx += BakePointOffset;
    if(bakePointIdx >= NumBakePoints)
        return;

    const BakePoint bakePoint = BakePoints[bakePointIdx];

    const float3x3 tangentToWorld = float3x3(bakePoint.Tangent, bakePoint.Bitangent, bakePoint.Normal);

    float4 currResults = BakeResults[uint3(bakePoint.TexelPos, 0)];

    for(uint i = 0; i < NumSamplesToBake; ++i)
    {
        const uint sampleIdx = BakeSampleStart + i;
        const float2 sampleCoord = SampleCMJ2D(sampleIdx, NumBakeSamples, NumBakeSamples, bakePointIdx);
        if(BakeMode == BakeModes_Diffuse)
        {
            const float3 sampleDirTS = SampleCosineHemisphere(sampleCoord.x, sampleCoord.y);
            const float3 sampleDirWS = mul(sampleDirTS, tangentToWorld);
            const float3 radiance = RayMarchVoxels(bakePoint.Position, sampleDirWS, bakePoint.Normal);

            const float lerpFactor = sampleIdx / (sampleIdx + 1.0f);
            float3 newSample = radiance * CosineWeightedMonteCarloFactor(1);
            float3 currValue = currResults.xyz;
            currValue = lerp(newSample, currValue, lerpFactor);
            currResults = float4(clamp(currValue, 0.0f, FP16Max), 1.0f);
        }
    }

    BakeResults[uint3(bakePoint.TexelPos, 0)] = currResults;
}

[numthreads(64, 1, 1)]
void FillGutters(in uint gutterIdx : SV_DispatchThreadID)
{
    if(gutterIdx >= NumGutterTexels)
        return;

    const GutterTexel gutterTexel = GutterTexels[gutterIdx];
    for(uint i = 0; i < BasisCount; ++i)
    {
        const float4 srcData = BakeResults[uint3(gutterTexel.NeighborPos, i)];
        BakeResults[uint3(gutterTexel.TexelPos, i)] = srcData;
    }
}