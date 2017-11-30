//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

RWTexture3D<float4> VoxelTextures[6];

[numthreads(4, 4, 4)]
void ClearVoxelTextures(in uint3 voxelCood : SV_DispatchThreadID)
{
    [unroll]
    for(uint i = 0; i < 6; ++i)
        VoxelTextures[i][voxelCood] = 0.0f;
}