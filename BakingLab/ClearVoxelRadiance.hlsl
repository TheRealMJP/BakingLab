//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

RWTexture3D<float4> VoxelRadiance;

[numthreads(4, 4, 4)]
void ClearVoxelRadiance(in uint3 voxelCoord : SV_DispatchThreadID)
{
    VoxelRadiance[voxelCoord] = 0.0f;
}