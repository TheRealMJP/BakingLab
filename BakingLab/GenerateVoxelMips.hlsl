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

Texture3D<float4> SourceMips[6] : register(t0);
RWTexture3D<float4> DestinationMips[6] : register(u0);
SamplerState PointClamp : register(s0);


[numthreads(4, 4, 4)]
void GenerateVoxelMips(in uint3 voxelCoord : SV_DispatchThreadID)
{
    float3 uvw = (voxelCoord + 0.5f) * DstMipTexelSize;

    static const uint3 sampleIndices[6][8] =
    {
        {
            // +X
            uint3(1, 0, 0),
            uint3(0, 0, 0),
            uint3(1, 1, 0),
            uint3(0, 1, 0),
            uint3(1, 0, 1),
            uint3(0, 0, 1),
            uint3(1, 1, 1),
            uint3(0, 1, 1),
        },

        {
            // -X
            uint3(0, 0, 0),
            uint3(1, 0, 0),
            uint3(0, 1, 0),
            uint3(1, 1, 0),
            uint3(0, 0, 1),
            uint3(1, 0, 1),
            uint3(0, 1, 1),
            uint3(1, 1, 1),
        },

        {
            // +Y
            uint3(0, 1, 0),
            uint3(0, 0, 0),
            uint3(1, 1, 0),
            uint3(1, 0, 0),
            uint3(0, 1, 1),
            uint3(0, 0, 1),
            uint3(1, 1, 1),
            uint3(1, 0, 1),
        },

        {
            // -Y
            uint3(0, 0, 0),
            uint3(0, 1, 0),
            uint3(1, 0, 0),
            uint3(1, 1, 0),
            uint3(0, 0, 1),
            uint3(0, 1, 1),
            uint3(1, 0, 1),
            uint3(1, 1, 1),
        },

        {
            // +Z
            uint3(0, 0, 1),
            uint3(0, 0, 0),
            uint3(0, 1, 1),
            uint3(0, 1, 0),
            uint3(1, 0, 1),
            uint3(1, 0, 0),
            uint3(1, 1, 1),
            uint3(1, 1, 0),
        },

        {
            // -Z
            uint3(0, 0, 0),
            uint3(0, 0, 1),
            uint3(0, 1, 0),
            uint3(0, 1, 1),
            uint3(1, 0, 0),
            uint3(1, 0, 1),
            uint3(1, 1, 0),
            uint3(1, 1, 1),
        },
    };

    [unroll]
    for(uint face = 0; face < 6; ++face)
    {
        float4 samples[2][2][2];
        float4 sum = 0.0f;

        [unroll]
        for(int z = 0; z < 2; ++z)
        {
            [unroll]
            for(int y = 0; y < 2; ++y)
            {
                [unroll]
                for(int x = 0; x < 2; ++x)
                {
                    float3 offset = float3(x, y, z) - 0.5f;
                    offset *= SrcMipTexelSize;

                    #if FirstMip_
                        const uint srcTexIdx = 0;
                    #else
                        const uint srcTexIdx = face;
                    #endif

                    samples[x][y][z] = SourceMips[srcTexIdx].SampleLevel(PointClamp, uvw + offset, 0.0f);

                    sum += samples[x][y][z];
                }
            }
        }

        uint3 idx0 = sampleIndices[face][0];
        uint3 idx1 = sampleIndices[face][1];
        uint3 idx2 = sampleIndices[face][2];
        uint3 idx3 = sampleIndices[face][3];
        uint3 idx4 = sampleIndices[face][4];
        uint3 idx5 = sampleIndices[face][5];
        uint3 idx6 = sampleIndices[face][6];
        uint3 idx7 = sampleIndices[face][7];

        float4 sample0 = samples[idx0.x][idx0.y][idx0.z];
        float4 sample1 = samples[idx1.x][idx1.y][idx1.z];
        float4 sample01 = float4(sample0.xyz * sample0.w + sample1.xyz * (1.0f - sample0.w),
                                 sample0.w + sample1.w * (1.0f - sample0.w));

        float4 sample2 = samples[idx2.x][idx2.y][idx2.z];
        float4 sample3 = samples[idx3.x][idx3.y][idx3.z];
        float4 sample23 = float4(sample2.xyz * sample2.w + sample3.xyz * (1.0f - sample2.w),
                                 sample2.w + sample3.w * (1.0f - sample2.w));

        float4 sample0123 = (sample01 + sample23) * 0.5f;

        float4 sample4 = samples[idx4.x][idx4.y][idx4.z];
        float4 sample5 = samples[idx5.x][idx5.y][idx5.z];
        float4 sample45 = float4(sample4.xyz * sample4.w + sample5.xyz * (1.0f - sample4.w),
                                 sample4.w + sample5.w * (1.0f - sample4.w));

        float4 sample6 = samples[idx6.x][idx6.y][idx6.z];
        float4 sample7 = samples[idx7.x][idx7.y][idx7.z];
        float4 sample67 = float4(sample6.xyz * sample6.w + sample7.xyz * (1.0f - sample6.w),
                                 sample6.w + sample7.w * (1.0f - sample6.w));

        float4 sample4567 = (sample45 + sample67) * 0.5f;

        DestinationMips[face][voxelCoord] = (sample0123 + sample4567) * 0.5f;

        // DestinationMips[face][voxelCoord] = sum / 8.0f;
    }
}
