//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include <Constants.hlsl>

cbuffer IntegrateConstants : register(b0)
{
    float2 OutputTextureSize;
    uint OutputSliceOffset;
}

SamplerState LinearSampler : register(s0);

float3 ComputeCubemapDirection(in uint2 pos, in uint face, in float2 textureSize)
{
    float u = ((pos.x + 0.5f) / textureSize.x) * 2.0f - 1.0f;
    float v = ((pos.y + 0.5f) / textureSize.y) * 2.0f - 1.0f;
    v *= -1.0f;

    float3 dir = 0.0f;

    // +x, -x, +y, -y, +z, -z
    switch(face)
    {
        case 0:
            dir = normalize(float3(1.0f, v, -u));
            break;
        case 1:
            dir = normalize(float3(-1.0f, v, u));
            break;
        case 2:
            dir = normalize(float3(u, 1.0f, -v));
            break;
        case 3:
            dir = normalize(float3(u, -1.0f, v));
            break;
        case 4:
            dir = normalize(float3(u, v, 1.0f));
            break;
        case 5:
            dir = normalize(float3(-u, v, -1.0f));
            break;
    }

    return dir;
}

// Computes a radical inverse with base 2 using crazy bit-twiddling from "Hacker's Delight"
float RadicalInverseBase2(in uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

// Returns a single 2D point in a Hammersley sequence of length "numSamples", using base 1 and base 2
float2 Hammersley2D(in uint sampleIdx, in uint numSamples)
{
    return float2(float(sampleIdx) / float(numSamples), RadicalInverseBase2(sampleIdx));
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

float3 SampleCosineHemisphere(in float u1, in float u2)
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

float3x3 MakeTangentFrame(in float3 normal, in uint faceIdx)
{
    float3 tangent = 0.0f;
    if(faceIdx == 0)
        tangent = float3(0.0f, 0.0f, -1.0f);
    else if(faceIdx == 1)
        tangent = float3(0.0f, 0.0f, 1.0f);
    else if(faceIdx == 2)
        tangent = float3(1.0f, 0.0f, 0.0f);
    else if(faceIdx == 3)
        tangent = float3(1.0f, 0.0f, 0.0f);
    else if(faceIdx == 4)
        tangent = float3(1.0f, 0.0f, 0.0f);
    else if(faceIdx == 5)
        tangent = float3(-1.0f, 0.0f, 0.0f);

    float3 bitangent = normalize(cross(normal, tangent));
    tangent = normalize(cross(bitangent, normal));

    return float3x3(tangent, bitangent, normal);
}

TextureCube<float4> RadianceCaptureMap : register(t0);
RWTexture2DArray<float4> IrradianceMap : register(u0);

[numthreads(8, 8, 1)]
void IntegrateIrradiance(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
    uint2 outputPos = GroupID.xy * 8 + GroupThreadID.xy;
    uint outputFaceIdx = GroupID.z;
    uint outputSliceIdx = OutputSliceOffset + outputFaceIdx;

    float3 normal = ComputeCubemapDirection(outputPos, outputFaceIdx, OutputTextureSize);
    float3x3 tangentFrame = MakeTangentFrame(normal, outputFaceIdx);

    const uint numSamples = 1024;
    float3 irradiance = 0.0f;
    for(uint i = 0; i < numSamples; ++i)
    {
        float2 samplePos = Hammersley2D(i, numSamples);
        float3 sampleDir = SampleCosineHemisphere(samplePos.x, samplePos.y);
        sampleDir = mul(sampleDir, tangentFrame);
        float3 radiance = RadianceCaptureMap.SampleLevel(LinearSampler, sampleDir, 0.0f).xyz;
        irradiance += radiance;
    }

    irradiance *= (Pi / numSamples);

    // irradiance = 16.0f;

    IrradianceMap[uint3(outputPos, outputSliceIdx)] = float4(irradiance, 0.0f);
}

TextureCube<float2> DistanceCaptureMap : register(t0);
RWTexture2DArray<float2> DistanceOutputMap : register(u0);

float3 SampleCosinePower(in float2 xi, in float n)
{
    float phi = 2.0f * Pi * xi.x;
    float theta = acos(pow(1.0f - xi.y, 1.0f /(n + 1.0f)));

    float3 dir;
    dir.x = sin(theta) * cos(phi);
    dir.y = sin(theta) * sin(phi);
    dir.z = cos(theta);

    return dir;
}

[numthreads(8, 8, 1)]
void IntegrateDistance(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
    uint2 outputPos = GroupID.xy * 8 + GroupThreadID.xy;
    uint outputFaceIdx = GroupID.z;
    uint outputSliceIdx = OutputSliceOffset + outputFaceIdx;

    float3 normal = ComputeCubemapDirection(outputPos, outputFaceIdx, OutputTextureSize);
    float3x3 tangentFrame = MakeTangentFrame(normal, outputFaceIdx);

    const uint numSamples = 256;
    const float cosinePower = exp2(12.0f);

    float2 outputDistance = 0.0f;
    float weightSum = 0.0f;
    for(uint i = 0; i < numSamples; ++i)
    {
        float2 samplePos = Hammersley2D(i, numSamples);
        float3 sampleDir = SampleCosinePower(samplePos, cosinePower);
        sampleDir = mul(sampleDir, tangentFrame);
        float2 sampleDistance = DistanceCaptureMap.SampleLevel(LinearSampler, sampleDir, 0.0f);

        float sampleWeight = ((cosinePower + 1.0f)  / Pi2) * pow(saturate(dot(sampleDir, normal)), cosinePower);
        outputDistance += sampleDistance * sampleWeight;
        weightSum += sampleWeight;
    }

    outputDistance *= (1.0f / weightSum);

    // outputDistance = DistanceCaptureMap.SampleLevel(LinearSampler, normal, 0.0f).xy;

    DistanceOutputMap[uint3(outputPos, outputSliceIdx)] = outputDistance;
}