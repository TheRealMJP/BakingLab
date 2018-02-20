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
    float3 CameraPos;
    float MipVoxelRes;
}

Texture3D<float4> VoxelRadiance : register(t0);
Texture3D<float4> VoxelRadianceMips[6] : register(t1);
SamplerState PointSampler : register(s0);

struct VSOutputGeo
{
    float4 PositionCS : SV_Position;
    float3 VoxelRadiance  : VOXELRADIANCE;
};

//=================================================================================================
// Vertex shader for geometry-based voxel visualization
//=================================================================================================
VSOutputGeo GeoVS(in float3 VertexPosition : POSITION, in uint InstanceID : SV_InstanceID)
{
    const uint voxelIdx = InstanceID;
    const uint3 voxelCoord = uint3(voxelIdx % VoxelResolution,
                                   (voxelIdx / VoxelResolution) % VoxelResolution,
                                   voxelIdx / (VoxelResolution * VoxelResolution));

    float3 voxelRadiance = VoxelRadiance[voxelCoord].xyz;
    const float voxelOpacity = VoxelRadiance[voxelCoord].w;

    if(voxelOpacity <= 0.0f)
    {
        VSOutputGeo output;
        output.PositionCS = float4(-10000.0f, -10000.0f, -10000.0f, 1.0f);
        output.VoxelRadiance = 0.0f;
        return output;
    }

    const float3 uvw = float3((voxelCoord.x + 0.5f) / VoxelResolution,
                              (voxelCoord.y + 0.5f) / VoxelResolution,
                              (voxelCoord.z + 0.5f) / VoxelResolution);

    const float3 sceneSize = (SceneMaxBounds - SceneMinBounds);
    const float3 voxelSize = sceneSize / float(VoxelResolution);

    const float3 voxelCenter = SceneMinBounds + sceneSize * uvw;
    const float3 vtxPositionWS = voxelCenter + VertexPosition * voxelSize;

    VSOutputGeo output;
    output.PositionCS = mul(float4(vtxPositionWS, 1.0f), ViewProjection);
    output.VoxelRadiance = voxelRadiance;

    return output;
}

//=================================================================================================
// Pixel shader for geometry-based voxel visualization
//=================================================================================================
float4 GeoPS(in VSOutputGeo input) : SV_Target0
{
    return float4(input.VoxelRadiance, 1.0f);
}

struct VSOutputRayMarch
{
    float4 PositionCS : SV_Position;
    float3 PositionWS : WORLDPOS;
};

//=================================================================================================
// Vertex shader for ray-marching voxel visualization
//=================================================================================================
VSOutputRayMarch RayMarchVS(in float3 VertexPosition : POSITION)
{
    float3 vtxPositionWS = lerp(SceneMinBounds, SceneMaxBounds, VertexPosition + 0.5f);

    VSOutputRayMarch output;
    output.PositionCS = mul(float4(vtxPositionWS, 1.0f), ViewProjection);
    output.PositionWS = vtxPositionWS;

    return output;
}
float IntersectRayBox3D_Clamped(float3 rayOrg, float3 dir, float3 bbmin, float3 bbmax)
{
    float3 invDir = rcp(dir);
    float3 d0 = (bbmin - rayOrg) * invDir;
    float3 d1 = (bbmax - rayOrg) * invDir;

    float3 v0 = min(d0, d1);
    float3 v1 = max(d0, d1);

    float tmin = max(v0.x, max(v0.y, v0.z));
    float tmax = min(v1.x, min(v1.y, v1.z));

    return tmin;
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

//=================================================================================================
// Pixel shader for ray-marching voxel visualization
//=================================================================================================
float4 RayMarchPS(in VSOutputRayMarch input) : SV_Target0
{
    const float voxelRes = MipVoxelRes;
    const float3 sceneSize = SceneMaxBounds - SceneMinBounds;
    const float voxelSize = 1.0f / voxelRes;

    const float3 pixelPosVoxelSpace = ((input.PositionWS - SceneMinBounds) / sceneSize) * voxelRes;
    const float3 cameraPosVoxelSpace = ((CameraPos - SceneMinBounds) / sceneSize) * voxelRes;
    const float3 viewDir = normalize(pixelPosVoxelSpace - cameraPosVoxelSpace);
    // const float bias = voxelSize * 0.25f;
    const float bias = 0.001f;

    // const float opacityCorrection = voxelSize / rcp(VoxelResolution);

    float dist = max(IntersectRayBox3D_Clamped(cameraPosVoxelSpace, viewDir, 0.0f, voxelRes), 0.0f);
    float3 startPos = cameraPosVoxelSpace + viewDir * (dist + bias);
    float3 currPos = startPos;

    const float mipLevel = VoxelVisualizerMipLevel - 1.0f;

    const uint MaxSteps = 1024;

    float3 radiance = 0.0f;
    float opacity = 0.0f;
    for(uint i = 0; i < MaxSteps; ++i)
    {
        float3 uvw = currPos / voxelRes;
        if(any(abs(uvw * 2.0f - 1.0f) > 1.0001f) || opacity >= 0.999f)
            break;

        const float3 currVoxelMin = floor(currPos);
        const float3 currVoxelMax = currVoxelMin + 1.0f;
        const float3 currVoxelCenter = (currVoxelMin + currVoxelMax) / 2.0f;

        #if FirstMip_
            float4 voxelSample = VoxelRadiance.SampleLevel(PointSampler, uvw, 0.0f);
        #else
            float3 voxelToPos = currPos - currVoxelCenter;
            float largestComponent = max(max(abs(voxelToPos.x), abs(voxelToPos.y)), abs(voxelToPos.z));

            float4 voxelSample = 0.0f;
            if(largestComponent == voxelToPos.x)
                voxelSample = VoxelRadianceMips[0].SampleLevel(PointSampler, uvw, mipLevel);
            else if(largestComponent == -voxelToPos.x)
                voxelSample = VoxelRadianceMips[1].SampleLevel(PointSampler, uvw, mipLevel);
            else if(largestComponent == voxelToPos.y)
                voxelSample = VoxelRadianceMips[2].SampleLevel(PointSampler, uvw, mipLevel);
            else if(largestComponent == -voxelToPos.y)
                voxelSample = VoxelRadianceMips[3].SampleLevel(PointSampler, uvw, mipLevel);
            else if(largestComponent == voxelToPos.z)
                voxelSample = VoxelRadianceMips[4].SampleLevel(PointSampler, uvw, mipLevel);
            else if(largestComponent == -voxelToPos.z)
                voxelSample = VoxelRadianceMips[5].SampleLevel(PointSampler, uvw, mipLevel);
        #endif

        voxelSample.w = saturate(voxelSample.w);
        // voxelSample.w = pow(1.0f - voxelSample.w, opacityCorrection);

        radiance += (1.0f - opacity) * voxelSample.xyz;
        opacity += (1.0f - opacity) * voxelSample.w;

        float distToNextVoxel = IntersectRayBox3D(currPos, viewDir, currVoxelMin, currVoxelMax);
        if(all(abs(viewDir.xy) < 0.01f))
            distToNextVoxel = IntersectRayBox1D(currPos.z, viewDir.z, currVoxelMin.z, currVoxelMax.z);
        else if(all(abs(viewDir.xz) < 0.01f))
            distToNextVoxel = IntersectRayBox1D(currPos.y, viewDir.y, currVoxelMin.y, currVoxelMax.y);
        else if(all(abs(viewDir.yz) < 0.01f))
            distToNextVoxel = IntersectRayBox1D(currPos.x, viewDir.x, currVoxelMin.x, currVoxelMax.x);
        else if(abs(viewDir.x) < 0.01f)
            distToNextVoxel = IntersectRayBox2D(currPos.yz, viewDir.yz, currVoxelMin.yz, currVoxelMax.yz);
        else if(abs(viewDir.y) < 0.01f)
            distToNextVoxel = IntersectRayBox2D(currPos.xz, viewDir.xz, currVoxelMin.xz, currVoxelMax.xz);
        else if(abs(viewDir.z) < 0.01f)
            distToNextVoxel = IntersectRayBox2D(currPos.xy, viewDir.xy, currVoxelMin.xy, currVoxelMax.xy);

        currPos += viewDir * (distToNextVoxel + bias);
    }

    return float4(radiance, opacity);
}
