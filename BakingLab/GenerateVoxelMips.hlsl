//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

cbuffer GenerateMipConstants : register(b0)
{
    float3 SrcMipTexelSize;
    float3 DstMipTexelSize;
}

Texture3D<float4> SourceMip : register(t0);
RWTexture3D<float4> DestinationMip : register(u0);
SamplerState LinearClamp : register(s0);

[numthreads(4, 4, 4)]
void GenerateVoxelMips(in uint3 voxelCoord : SV_DispatchThreadID)
{
    float3 uvw = (voxelCoord + 0.5f) * DstMipTexelSize;
    float4 output = SourceMip.SampleLevel(LinearClamp, uvw, 0.0f);
    DestinationMip[voxelCoord] = output;
}
