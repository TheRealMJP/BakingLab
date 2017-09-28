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

TextureCube<float4> RadianceMap : register(t0);
RWTexture2DArray<float4> IrradianceMap : register(u0);

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

inline float2 SquareToConcentricDiskMapping(float x, float y)
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

inline float3 SampleCosineHemisphere(in float u1, in float u2)
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

[numthreads(8, 8, 1)]
void IntegrateIrradiance(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID)
{
    uint2 outputPos = GroupID.xy * 8 + GroupThreadID.xy;
    uint outputFaceIdx = GroupID.z;
    uint outputSliceIdx = OutputSliceOffset + outputFaceIdx;

    float3 normal = ComputeCubemapDirection(outputPos, outputFaceIdx, OutputTextureSize);

    float3 tangent = 0.0f;
    if(outputFaceIdx == 0)
        tangent = float3(0.0f, 0.0f, -1.0f);
    else if(outputFaceIdx == 1)
        tangent = float3(0.0f, 0.0f, 1.0f);
    else if(outputFaceIdx == 2)
        tangent = float3(1.0f, 0.0f, 0.0f);
    else if(outputFaceIdx == 3)
        tangent = float3(1.0f, 0.0f, 0.0f);
    else if(outputFaceIdx == 4)
        tangent = float3(1.0f, 0.0f, 0.0f);
    else if(outputFaceIdx == 5)
        tangent = float3(-1.0f, 0.0f, 0.0f);

    float3 bitangent = normalize(cross(normal, tangent));
    tangent = normalize(cross(bitangent, normal));

    const uint numSamples = 64;
    float3 irradiance = 0.0f;
    for(uint i = 0; i < numSamples; ++i)
    {
        float2 samplePos = Hammersley2D(i, numSamples);
        float3 sampleDir = SampleCosineHemisphere(samplePos.x, samplePos.y);
        sampleDir = mul(sampleDir, float3x3(tangent, bitangent, normal));
        float3 radiance = RadianceMap.SampleLevel(LinearSampler, sampleDir, 0.0f).xyz;
        irradiance += radiance;
    }

    irradiance *= (Pi / numSamples);

    IrradianceMap[uint3(outputPos, outputSliceIdx)] = float4(irradiance, 0.0f);
}