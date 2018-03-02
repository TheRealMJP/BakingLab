//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include <PCH.h>

#include <App.h>
#include <InterfacePointers.h>
#include <Input.h>
#include <Graphics/Camera.h>
#include <Graphics/Model.h>
#include <Graphics/SpriteFont.h>
#include <Graphics/SpriteRenderer.h>
#include <Graphics/Skybox.h>
#include <Graphics/GraphicsTypes.h>

#include "PostProcessor.h"
#include "MeshRenderer.h"
#include "MeshBaker.h"

using namespace SampleFramework11;

class BakingLab : public App
{

protected:

    FirstPersonCamera camera;

    SpriteFont font;
    SampleFramework11::SpriteRenderer spriteRenderer;
    Skybox skybox;
    ID3D11ShaderResourceViewPtr envMaps[AppSettings::NumCubeMaps];
    SH9Color envMapSH[AppSettings::NumCubeMaps];
    bool32 computedEnvMapSH[AppSettings::NumCubeMaps] = { };

    PostProcessor postProcessor;
    DepthStencilBuffer depthBuffer;
    RenderTarget2D colorTargetMSAA;
    RenderTarget2D colorResolveTarget;
    RenderTarget2D prevFrameTarget;
    RenderTarget2D velocityTargetMSAA;
    StagingTexture2D readbackTexture;

    // Model
    Model sceneModels[uint64(Scenes::NumValues)];
    Float3 sceneMins[uint64(Scenes::NumValues)];
    Float3 sceneMaxes[uint64(Scenes::NumValues)];
    Float3 currSceneMin;
    Float3 currSceneMax;
    MeshRenderer meshRenderer;
    MeshBaker meshBaker;

    MouseState mouseState;

    float GTSampleRateBuffer[64];
    uint64 GTSampleRateBufferIdx = 0;

    VertexShaderPtr resolveVS;
    PixelShaderPtr resolvePS[uint64(MSAAModes::NumValues)];

    VertexShaderPtr backgroundVelocityVS;
    PixelShaderPtr backgroundVelocityPS;
    Float4x4 prevViewProjection;

    Float2 jitterOffset;
    Float2 prevJitter;
    uint64 frameCount = 0;
    FirstPersonCamera unJitteredCamera;
    bool enableTAA = false;

    RenderTarget2D probeRadianceCubeMap;
    RenderTarget2D probeDistanceCubeMap;
    DepthStencilBuffer probeDepthBuffer;

    uint64 currProbeIdx = 0;

    RenderTarget3D voxelRadiance;
    RenderTarget3D voxelRadianceMips[6];
    ComputeShaderPtr clearVoxelRadiance;
    ComputeShaderPtr fillVoxelHolesX;
    ComputeShaderPtr fillVoxelHolesY;
    ComputeShaderPtr fillVoxelHolesZ;
    ComputeShaderPtr generateFirstVoxelMip;
    ComputeShaderPtr generateVoxelMips;
    uint32 numVoxelMips = 0;

    RenderTarget2D voxelBakeTextures[2];
    uint32 voxelBakePass = 0;
    uint32 voxelBakePointOffset = 0;
    float voxelBakeProgress = 0.0f;
    ComputeShaderPtr voxelBakeCS;
    ComputeShaderPtr fillGuttersCS;

    struct ResolveConstants
    {
        uint32 SampleRadius;
        bool32 EnableTemporalAA;
        Float2 TextureSize;
    };

    struct BackgroundVelocityConstants
    {
        Float4x4 InvViewProjection;
        Float4x4 PrevViewProjection;
        Float2 RTSize;
        Float2 JitterOffset;
    };

    struct GenerateMipConstants
    {
        float SrcMipTexelSize;
        float DstMipTexelSize;
    };

    struct VoxelBakeConstants
    {
        uint32 BakeSampleStart;
        uint32 NumSamplesToBake;
        uint32 BasisCount;
        uint32 NumBakePoints;
        uint32 BakePointOffset;
        uint32 NumGutterTexels;

        Float4Align ShaderSH9Color SkySH;

        Float4Align Float3 SceneMinBounds;
        Float4Align Float3 SceneMaxBounds;
    };

    ConstantBuffer<ResolveConstants> resolveConstants;
    ConstantBuffer<BackgroundVelocityConstants> backgroundVelocityConstants;
    ConstantBuffer<GenerateMipConstants> generateMipConstants;
    ConstantBuffer<VoxelBakeConstants> voxelBakeConstants;

    virtual void Initialize() override;
    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();

    void RenderProbes(MeshBakerStatus& status);
    void VoxelizeScene(MeshBakerStatus& status);
    void BakeWithVoxels(MeshBakerStatus& status);
    void RenderScene(const MeshBakerStatus& status, ID3D11RenderTargetView* colorTarget, ID3D11RenderTargetView* secondRT,
                     const DepthStencilBuffer& depth, const Camera& cam, bool32 showBakeDataVisualizer, bool32 showProbeVisualizer,
                     bool32 renderAreaLight, bool32 showVoxelVisualizer, bool32 enableSkySun, bool32 probeRendering);
    void RenderAA();
    void RenderBackgroundVelocity();
    void RenderHUD(const Timer& timer, float groundTruthProgress, float bakeProgress,
                   uint64 groundTruthSampleCount, float probeBakeProgress);

public:

    BakingLab();
};
