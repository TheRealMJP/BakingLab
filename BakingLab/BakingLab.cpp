//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include <PCH.h>

#include <InterfacePointers.h>
#include <Window.h>
#include <Graphics/DeviceManager.h>
#include <Input.h>
#include <Utility.h>
#include <Graphics/ShaderCompilation.h>
#include <Graphics/Profiler.h>
#include <Graphics/Sampling.h>
#include <Graphics/BRDF.h>
#include <FileIO.h>

#include "BakingLab.h"
#include "MeshBaker.h"
#include "SG.h"

#include "resource.h"

using namespace SampleFramework11;
using std::wstring;

const uint32 WindowWidth = 1280;
const uint32 WindowHeight = 720;
const float WindowWidthF = static_cast<float>(WindowWidth);
const float WindowHeightF = static_cast<float>(WindowHeight);

static const float NearClip = 0.01f;
static const float FarClip = 100.0f;

#define UseCachedLightmap_ (1)
#define WriteCachedLightmap_ (Release_ && UseCachedLightmap_)

// Model filenames
static const wchar* ScenePaths[] =
{
    L"..\\Content\\Models\\Box\\Box_Lightmap.fbx",
    L"..\\Content\\Models\\WhiteRoom\\WhiteRoom.fbx",
    L"..\\Content\\Models\\Sponza\\Sponza_Lightmap.fbx",
};

static const Float3 SceneCameraPositions[] = { Float3(0.0f, 2.5f, -15.0f), Float3(0.0f, 2.5f, 0.0f), Float3(-5.12373829f, 13.8305235f, -0.463505715f) };
static const Float2 SceneCameraRotations[] = { Float2(0.0f, 0.0f), Float2(0.0f, Pi), Float2(0.414238036f, 1.39585948f) };
static const float SceneAlbedoScales[] = { 0.5f, 0.5f, 1.0f };

static const Uint3 SceneDefaultProbeRes[] = { Uint3(4, 4, 4), Uint3(5, 3, 5), Uint3(5, 5, 5) };
// static const float SceneDefaultBoundsScales[] = { 1.5f, 1.65f, 1.0f };
static const float SceneDefaultBoundsScales[] = { 1.0f, 1.0f, 1.0f };

StaticAssert_(ArraySize_(ScenePaths) >= uint64(Scenes::NumValues));
StaticAssert_(ArraySize_(SceneCameraPositions) >= uint64(Scenes::NumValues));
StaticAssert_(ArraySize_(SceneCameraRotations) >= uint64(Scenes::NumValues));
StaticAssert_(ArraySize_(SceneAlbedoScales) >= uint64(Scenes::NumValues));
StaticAssert_(ArraySize_(SceneDefaultProbeRes) >= uint64(Scenes::NumValues));
StaticAssert_(ArraySize_(SceneDefaultBoundsScales) >= uint64(Scenes::NumValues));

static Setting* LightSettings[] =
{
    &AppSettings::EnableSun,
    &AppSettings::SunTintColor,
    &AppSettings::SunIntensityScale,
    &AppSettings::SunSize,
    &AppSettings::NormalizeSunIntensity,
    &AppSettings::SunDirection,
    &AppSettings::SunAzimuth,
    &AppSettings::SunElevation,
    &AppSettings::SkyMode,
    &AppSettings::SkyColor,
    &AppSettings::Turbidity,
    &AppSettings::GroundAlbedo,
    &AppSettings::EnableAreaLight,
    &AppSettings::EnableAreaLightShadows,
    &AppSettings::AreaLightColor,
    &AppSettings::AreaLightIlluminance,
    &AppSettings::AreaLightLuminousPower,
    &AppSettings::AreaLightEV100,
    &AppSettings::AreaLightIlluminanceDistance,
    &AppSettings::AreaLightSize,
    &AppSettings::AreaLightX,
    &AppSettings::AreaLightY,
    &AppSettings::AreaLightZ,
    &AppSettings::AreaLightShadowBias,
    &AppSettings::BakeDirectAreaLight,
    &AppSettings::AreaLightUnits,
};

static const uint64 NumLightSettings = ArraySize_(LightSettings);

struct SettingInfo
{
    std::string Name;
    uint64 DataSize = 0;

    template<typename TSerializer> void Serialize(TSerializer& serializer)
    {
        SerializeItem(serializer, Name);
        SerializeItem(serializer, DataSize);
    }
};

// Save lighting settings to a file
static void LoadLightSettings(HWND parentWindow)
{
    wchar currDirectory[MAX_PATH] = { 0 };
    GetCurrentDirectory(ArraySize_(currDirectory), currDirectory);

    wchar filePath[MAX_PATH] = { 0 };

    OPENFILENAME ofn;
    ZeroMemory(&ofn , sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parentWindow;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = ArraySize_(filePath);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0Light Settings (*.lts)\0*.lts\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Open Light Settings File..";
    ofn.lpstrDefExt = L"lts";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    bool succeeded = GetOpenFileName(&ofn) != 0;
    SetCurrentDirectory(currDirectory);

    if(succeeded)
    {
        try
        {
            FileReadSerializer serializer(filePath);

            std::vector<SettingInfo> settingInfo;
            SerializeItem(serializer, settingInfo);

            uint8 dummyBuffer[1024] = { 0 };
            for(uint64 i = 0; i < settingInfo.size(); ++i)
            {
                const SettingInfo& info = settingInfo[i];
                Setting* setting = Settings.FindSetting(info.Name);
                if(setting == nullptr || setting->SerializedValueSize() != info.DataSize)
                {
                    // Skip the data for this setting, it's out-of-date
                    Assert_(info.DataSize <= sizeof(dummyBuffer));
                    if(info.DataSize > 0)
                        serializer.SerializeData(info.DataSize, dummyBuffer);
                    continue;
                }

                setting->SerializeValue(serializer);
            }
        }
        catch(Exception e)
        {
            std::wstring errorString = L"Error occured while loading light settings file: " + e.GetMessage();
            MessageBox(parentWindow, errorString.c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }

    SetCurrentDirectory(currDirectory);
}

// Save lighting settings to a file
static void SaveLightSettings(HWND parentWindow)
{
    wchar currDirectory[MAX_PATH] = { 0 };
    GetCurrentDirectory(ArraySize_(currDirectory), currDirectory);

    wchar filePath[MAX_PATH] = { 0 };

    OPENFILENAME ofn;
    ZeroMemory(&ofn , sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parentWindow;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = ArraySize_(filePath);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0Light Settings (*.lts)\0*.lts\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Save Light Settings File As..";
    ofn.lpstrDefExt = L"lts";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    bool succeeded = GetSaveFileName(&ofn) != 0;
    SetCurrentDirectory(currDirectory);

    if(succeeded)
    {
        try
        {
            std::vector<SettingInfo> settingInfo;
            settingInfo.resize(NumLightSettings);
            for(uint64 i = 0; i < NumLightSettings; ++i)
            {
                // Serialize some metadata so that we can skip out of date settings on load
                settingInfo[i].Name = LightSettings[i]->Name();
                settingInfo[i].DataSize = LightSettings[i]->SerializedValueSize();
                Assert_(settingInfo[i].DataSize > 0);
            }

            FileWriteSerializer serializer(filePath);
            SerializeItem(serializer, settingInfo);

            for(uint64 i = 0; i < NumLightSettings; ++i)
                LightSettings[i]->SerializeValue(serializer);
        }
        catch(Exception e)
        {
            std::wstring errorString = L"Error occured while saving light settings file:\n" + e.GetMessage();
            MessageBox(parentWindow, errorString.c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }

    SetCurrentDirectory(currDirectory);
}

// Save a skydome texture as a DDS file
static void SaveEXRScreenshot(HWND parentWindow, ID3D11ShaderResourceView* screenSRV)
{
    // Read the texture data, and apply the inverse exposure scale
    ID3D11DevicePtr device;
    screenSRV->GetDevice(&device);

    TextureData<Float4> textureData;
    GetTextureData(device, screenSRV, textureData);

    const uint64 numTexels = textureData.Texels.size();
    for(uint64 i = 0; i < numTexels; ++i)
    {
        textureData.Texels[i] *= 1.0f / FP16Scale;
        textureData.Texels[i] = Float4::Clamp(textureData.Texels[i], 0.0f, FP16Max);
    }

    wchar currDirectory[MAX_PATH] = { 0 };
    GetCurrentDirectory(ArraySize_(currDirectory), currDirectory);

    wchar filePath[MAX_PATH] = { 0 };

    OPENFILENAME ofn;
    ZeroMemory(&ofn , sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parentWindow;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = ArraySize_(filePath);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0EXR Files (*.exr)\0*.exr\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Save Screenshot As..";
    ofn.lpstrDefExt = L"exr";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    bool succeeded = GetSaveFileName(&ofn) != 0;
    SetCurrentDirectory(currDirectory);

    if(succeeded)
    {
        try
        {
            SaveTextureAsEXR(textureData, filePath);
        }
        catch(Exception e)
        {
            std::wstring errorString = L"Error occured while saving screenshot as an EXR file:\n" + e.GetMessage();
            MessageBox(parentWindow, errorString.c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

// Bakes lookup textures for computing environment specular from radiance encoded as spherical harmonics.
static void GenerateSHSpecularLookupTextures(ID3D11Device* device)
{
    static const uint32 ViewResolution = 32;
    static const uint32 RoughnessResolution = 32;
    static const uint32 FresnelResolution = 32;
    #if Debug_
        static const uint64 SqrtNumSamples = 10;
    #else
        static const uint64 SqrtNumSamples = 25;
    #endif
    static const uint64 NumSamples = SqrtNumSamples * SqrtNumSamples;
    std::vector<Half4> texData0(ViewResolution * RoughnessResolution * FresnelResolution);
    std::vector<Half2> texData1(ViewResolution * RoughnessResolution * FresnelResolution);

    int32 pattern = 0;
    const Float3 n = Float3(0.0f, 0.0f, 1.0f);

    // Integrate the specular BRDF for a fixed value of Phi (camera lined up with the X-axis)
    // for a set of viewing angles and roughness values
    for(uint32 fIdx = 0; fIdx < FresnelResolution; ++fIdx)
    {
        const float specAlbedo = (fIdx + 0.5f) / FresnelResolution;
        for(uint32 mIdx = 0; mIdx < RoughnessResolution; ++mIdx)
        {
            const float SqrtRoughness = (mIdx + 0.5f) / RoughnessResolution;
            const float Roughness = SqrtRoughness * SqrtRoughness;
            for(uint32 vIdx = 0; vIdx < ViewResolution; ++vIdx)
            {
                Float3 v = 0.0f;
                v.z = (vIdx + 0.5f) / ViewResolution;
                v.x = std::sqrt(1.0f - Saturate(v.z * v.z));

                SH9 sh;

                SH9 accumulatedSH;
                uint32 accumulatedSamples = 0;
                for(uint64 sampleIdx = 0; sampleIdx < NumSamples; ++sampleIdx)
                {
                    ++accumulatedSamples;

                    Float2 sampleCoord = SampleCMJ2D(int32(sampleIdx), int32(SqrtNumSamples), int32(SqrtNumSamples), pattern++);
                    Float3 l = SampleDirectionGGX(v, n, Roughness, Float3x3(), sampleCoord.x, sampleCoord.y);
                    Float3 h = Float3::Normalize(v + l);
                    float nDotL = Saturate(l.z);

                    float pdf = GGX_PDF(n, h, v, Roughness);
                    float brdf = GGX_Specular(Roughness, n, h, v, l) * Fresnel(specAlbedo, h, l).x;
                    SH9 sh = ProjectOntoSH9(l) * brdf * nDotL / pdf;

                    accumulatedSH += sh;
                    if(accumulatedSamples >= 1000)
                    {
                        sh += accumulatedSH / float(NumSamples);
                        accumulatedSH = SH9();
                        accumulatedSamples = 0;
                    }
                }

                if(accumulatedSamples > 0)
                    sh += accumulatedSH / float(NumSamples);

                const uint64 idx = (fIdx * ViewResolution * RoughnessResolution) + (mIdx * ViewResolution) + vIdx;
                texData0[idx] = Half4(sh[0], sh[2], sh[3], sh[6]);
                texData1[idx] = Half2(sh[7], sh[8]);
            }
        }
    }

    // Make 2 3D textures
    D3D11_TEXTURE3D_DESC desc;
    desc.Width = ViewResolution;
    desc.Height = RoughnessResolution;
    desc.Depth = FresnelResolution;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.MipLevels = 1;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA srData;
    srData.pSysMem = texData0.data();
    srData.SysMemPitch = sizeof(Half4) * desc.Width;
    srData.SysMemSlicePitch = sizeof(Half4) * desc.Width * desc.Height;

    ID3D11Texture3DPtr texture0;
    DXCall(device->CreateTexture3D(&desc, &srData, &texture0));

    desc.Format = DXGI_FORMAT_R16G16_FLOAT;
    srData.pSysMem = texData1.data();
    srData.SysMemPitch = sizeof(Half2) * desc.Width;
    srData.SysMemSlicePitch = sizeof(Half2) * desc.Width * desc.Height;

    ID3D11Texture3DPtr texture1;
    DXCall(device->CreateTexture3D(&desc, &srData, &texture1));

    SaveTextureAsDDS(texture0, L"..\\Content\\Textures\\SHSpecularA.dds");
    SaveTextureAsDDS(texture1, L"..\\Content\\Textures\\SHSpecularB.dds");
}

BakingLab::BakingLab() : App(L"Baking Lab", MAKEINTRESOURCEW(IDI_DEFAULT)),
                         camera(16.0f / 9.0f, Pi_4 * 0.75f, NearClip, FarClip)
{
    deviceManager.SetMinFeatureLevel(D3D_FEATURE_LEVEL_11_0);

    for(uint64 i = 0; i < ArraySize_(GTSampleRateBuffer); ++i)
        GTSampleRateBuffer[i] = 0.0f;
}

void BakingLab::BeforeReset()
{
    App::BeforeReset();
}

void BakingLab::AfterReset()
{
    App::AfterReset();

    float aspect = static_cast<float>(deviceManager.BackBufferWidth()) / deviceManager.BackBufferHeight();
    camera.SetAspectRatio(aspect);

    CreateRenderTargets();

    postProcessor.AfterReset(deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}

void BakingLab::Initialize()
{
    App::Initialize();

    ID3D11DevicePtr device = deviceManager.Device();
    ID3D11DeviceContextPtr deviceContext = deviceManager.ImmediateContext();

    // Uncomment this line to re-generate the lookup textures for the SH specular BRDF
    // GenerateSHSpecularLookupTextures(device);

    // Create a font + SpriteRenderer
    font.Initialize(L"Arial", 18, SpriteFont::Regular, true, device);
    spriteRenderer.Initialize(device);

    // Load the scenes
    for(uint64 i = 0; i < uint64(Scenes::NumValues); ++i)
    {
        if(GetFileExtension(ScenePaths[i]) == L"meshdata")
            sceneModels[i].CreateFromMeshData(device, ScenePaths[i], true);
        else
            sceneModels[i].CreateWithAssimp(device, ScenePaths[i], true);

        // Compute the scene AABB
        sceneMins[i] = FLT_MAX;
        sceneMaxes[i] = -FLT_MAX;

        const uint64 numMeshes = sceneModels[i].Meshes().size();
        for(uint64 meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
        {
            const Mesh& mesh = sceneModels[i].Meshes()[meshIdx];
            const uint8* vertices = mesh.Vertices();
            const uint64 numVertices = mesh.NumVertices();
            const uint64 stride = mesh.VertexStride();
            for(uint64 vtxIdx = 0; vtxIdx < numVertices; ++vtxIdx)
            {
                const Float3& vtx = *(const Float3*)(vertices + vtxIdx * stride);
                sceneMins[i].x = Min(sceneMins[i].x, vtx.x);
                sceneMins[i].y = Min(sceneMins[i].y, vtx.y);
                sceneMins[i].z = Min(sceneMins[i].z, vtx.z);
                sceneMaxes[i].x = Max(sceneMaxes[i].x, vtx.x);
                sceneMaxes[i].y = Max(sceneMaxes[i].y, vtx.y);
                sceneMaxes[i].z = Max(sceneMaxes[i].z, vtx.z);
            }
        }
    }

    Model& currentModel = sceneModels[AppSettings::CurrentScene.Value()];
    meshRenderer.Initialize(device, deviceManager.ImmediateContext(), &currentModel);

    camera.SetPosition(Float3(0.0f, 2.5f, -15.0f));

    skybox.Initialize(device);

    for(uint64 i = 0; i < AppSettings::NumCubeMaps; ++i)
        envMaps[i] = LoadTexture(device, AppSettings::CubeMapPaths(i));

    // Load shaders
    for(uint32 msaaMode = 0; msaaMode < uint32(MSAAModes::NumValues); ++msaaMode)
    {
        CompileOptions opts;
        opts.Add("MSAASamples_", AppSettings::NumMSAASamples(MSAAModes(msaaMode)));
        resolvePS[msaaMode] = CompilePSFromFile(device, L"Resolve.hlsl", "ResolvePS", "ps_5_0", opts);
    }

    resolveVS = CompileVSFromFile(device, L"Resolve.hlsl", "ResolveVS");

    backgroundVelocityVS = CompileVSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityVS");
    backgroundVelocityPS = CompilePSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityPS");

    probeIntegrateIrradianceCubeMap = CompileCSFromFile(device, L"ProbeIntegrate.hlsl", "IntegrateIrradianceCubeMap");
    probeIntegrateDistanceCubeMap = CompileCSFromFile(device, L"ProbeIntegrate.hlsl", "IntegrateDistanceCubeMap");

    probeIntegrateVolumeMap = CompileCSFromFile(device, L"ProbeIntegrate.hlsl", "IntegrateVolumeMap");
    probeIntegrateDistanceVolumeMap = CompileCSFromFile(device, L"ProbeIntegrate.hlsl", "IntegrateDistanceVolumeMap");

    clearVoxelRadiance = CompileCSFromFile(device, L"ClearVoxelRadiance.hlsl", "ClearVoxelRadiance");
    fixVoxelRadianceOccupancy = CompileCSFromFile(device, L"ClearVoxelRadiance.hlsl", "FixVoxelRadianceOccupancy");
    generateVoxelMips = CompileCSFromFile(device, L"GenerateVoxelMips.hlsl", "GenerateVoxelMips");

    resolveConstants.Initialize(device);
    backgroundVelocityConstants.Initialize(device);
    integrateConstants.Initialize(device);
    generateMipConstants.Initialize(device);

    // Init the post processor
    postProcessor.Initialize(device);

    BakeInputData bakeInput;
    bakeInput.SceneModel = &currentModel;
    bakeInput.Device = device;
    for(uint64 i = 0; i < AppSettings::NumCubeMaps; ++i)
        bakeInput.EnvMaps[i] = envMaps[i];
    meshBaker.Initialize(bakeInput);

    // Camera setup
    AppSettings::UpdateHorizontalCoords();
}

// Creates all required render targets
void BakingLab::CreateRenderTargets()
{
    ID3D11Device* device = deviceManager.Device();
    uint32 width = deviceManager.BackBufferWidth();
    uint32 height = deviceManager.BackBufferHeight();

    const uint32 NumSamples = AppSettings::NumMSAASamples();
    const uint32 Quality = NumSamples > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
    colorTargetMSAA.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, NumSamples, Quality);
    depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D24_UNORM_S8_UINT, true, NumSamples, Quality);
    velocityTargetMSAA.Initialize(device, width, height, DXGI_FORMAT_R16G16_FLOAT, 1, NumSamples, Quality);

    colorResolveTarget.Initialize(device, width, height, colorTargetMSAA.Format, 1, 1, 0);
    prevFrameTarget.Initialize(device, width, height, colorTargetMSAA.Format, 1, 1, 0);
    readbackTexture.Initialize(device, width, height, colorTargetMSAA.Format, 1, 1, 0);

    meshRenderer.OnResize(width, height);
}

void BakingLab::Update(const Timer& timer)
{
    AppSettings::UpdateUI();

    if(AppSettings::LoadLightSettings)
        LoadLightSettings(window.GetHwnd());

    if(AppSettings::SaveLightSettings)
        SaveLightSettings(window.GetHwnd());

    if(AppSettings::CurrentScene.Changed() || frameCount == 0)
    {
        uint64 currSceneIdx = uint64(AppSettings::CurrentScene);
        meshRenderer.SetModel(&sceneModels[currSceneIdx]);
        camera.SetPosition(SceneCameraPositions[currSceneIdx]);
        camera.SetXRotation(SceneCameraRotations[currSceneIdx].x);
        camera.SetYRotation(SceneCameraRotations[currSceneIdx].y);
        AppSettings::DiffuseAlbedoScale.SetValue(SceneAlbedoScales[currSceneIdx]);
        AppSettings::ProbeResX.SetValue(SceneDefaultProbeRes[currSceneIdx].x);
        AppSettings::ProbeResY.SetValue(SceneDefaultProbeRes[currSceneIdx].y);
        AppSettings::ProbeResZ.SetValue(SceneDefaultProbeRes[currSceneIdx].z);
        AppSettings::SceneBoundsScale.SetValue(SceneDefaultBoundsScales[currSceneIdx]);
    }

    mouseState = MouseState::GetMouseState(window);
    KeyboardState kbState = KeyboardState::GetKeyboardState(window);

    if(kbState.IsKeyDown(KeyboardState::Escape))
        window.Destroy();

    float CamMoveSpeed = 5.0f * timer.DeltaSecondsF();
    const float CamRotSpeed = 0.180f * timer.DeltaSecondsF();
    const float MeshRotSpeed = 0.180f * timer.DeltaSecondsF();

    // Move the camera with keyboard input
    if(kbState.IsKeyDown(KeyboardState::LeftShift))
        CamMoveSpeed *= 0.25f;

    Float3 camPos = camera.Position();
    if(kbState.IsKeyDown(KeyboardState::W))
        camPos += camera.Forward() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::S))
        camPos += camera.Back() * CamMoveSpeed;
    if(kbState.IsKeyDown(KeyboardState::A))
        camPos += camera.Left() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::D))
        camPos += camera.Right() * CamMoveSpeed;
    if(kbState.IsKeyDown(KeyboardState::Q))
        camPos += camera.Up() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::E))
        camPos += camera.Down() * CamMoveSpeed;
    camera.SetPosition(camPos);

    // Rotate the camera with the mouse
    if(mouseState.RButton.Pressed && mouseState.IsOverWindow)
    {
        float xRot = camera.XRotation();
        float yRot = camera.YRotation();
        xRot += mouseState.DY * CamRotSpeed;
        yRot += mouseState.DX * CamRotSpeed;
        camera.SetXRotation(xRot);
        camera.SetYRotation(yRot);
    }

    camera.SetFieldOfView(AppSettings::VerticalFOV(camera.AspectRatio()));
    unJitteredCamera = camera;

    enableTAA = AppSettings::EnableTemporalAA &&
                AppSettings::VoxelVisualizerMode == VoxelVisualizerModes::None &&
                AppSettings::ShowProbeVisualizer == false &&
                AppSettings::ShowBakeDataVisualizer == false &&
                AppSettings::ShowGroundTruth == false;

    Float2 jitter = 0.0f;
    if(enableTAA && AppSettings::EnableLuminancePicker == false && AppSettings::JitterMode != JitterModes::None)
    {
        if(AppSettings::JitterMode == JitterModes::Uniform2x)
        {
            jitter = frameCount % 2 == 0 ? -0.5f : 0.5f;
        }
        else if(AppSettings::JitterMode == JitterModes::Hammersley4x)
        {
            uint64 idx = frameCount % 4;
            jitter = Hammersley2D(idx, 4) * 2.0f - Float2(1.0f);
        }
        else if(AppSettings::JitterMode == JitterModes::Hammersley8x)
        {
            uint64 idx = frameCount % 8;
            jitter = Hammersley2D(idx, 8) * 2.0f - Float2(1.0f);
        }
        else if(AppSettings::JitterMode == JitterModes::Hammersley16x)
        {
            uint64 idx = frameCount % 16;
            jitter = Hammersley2D(idx, 16) * 2.0f - Float2(1.0f);
        }

        jitter *= AppSettings::JitterScale;

        const float offsetX = jitter.x * (1.0f / colorTargetMSAA.Width);
        const float offsetY = jitter.y * (1.0f / colorTargetMSAA.Height);
        Float4x4 offsetMatrix = Float4x4::TranslationMatrix(Float3(offsetX, -offsetY, 0.0f));
        camera.SetProjection(camera.ProjectionMatrix() * offsetMatrix);
    }

    jitterOffset = (jitter - prevJitter) * 0.5f;
    prevJitter = jitter;

    // Toggle VSYNC
    if(kbState.RisingEdge(KeyboardState::V))
        deviceManager.SetVSYNCEnabled(!deviceManager.VSYNCEnabled());

    Float3 sceneCenter = (sceneMaxes[AppSettings::CurrentScene] + sceneMins[AppSettings::CurrentScene]) / 2.0f;;
    currSceneMin = Lerp(sceneCenter, sceneMins[AppSettings::CurrentScene], AppSettings::SceneBoundsScale);
    currSceneMax = Lerp(sceneCenter, sceneMaxes[AppSettings::CurrentScene], AppSettings::SceneBoundsScale);
    currSceneMin.x += AppSettings::SceneBoundsOffsetX;
    currSceneMin.y += AppSettings::SceneBoundsOffsetY;
    currSceneMin.z += AppSettings::SceneBoundsOffsetZ;
    currSceneMax.x += AppSettings::SceneBoundsOffsetX;
    currSceneMax.y += AppSettings::SceneBoundsOffsetY;
    currSceneMax.z += AppSettings::SceneBoundsOffsetZ;

    if(AppSettings::ProbeMode == ProbeModes::CubeMap)
    {
        // Make sure that our probes fit within a single texture resource
        const uint64 maxProbes = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6;
        uint64 numProbes = AppSettings::NumProbes();
        if(numProbes > maxProbes)
        {
            const uint64 biggestDimRes = Max(Max(AppSettings::ProbeResX, AppSettings::ProbeResY), AppSettings::ProbeResZ);
            const uint64 probesPerBiggestDimSlice = numProbes / biggestDimRes;
            const int32 newRes = int32(maxProbes / float(probesPerBiggestDimSlice));
            if(biggestDimRes == AppSettings::ProbeResX)
                AppSettings::ProbeResX.SetValue(newRes);
            else if(biggestDimRes == AppSettings::ProbeResY)
                AppSettings::ProbeResY.SetValue(newRes);
            else
                AppSettings::ProbeResZ.SetValue(newRes);
        }
    }

    meshRenderer.Update(camera, jitterOffset);
}

void BakingLab::RenderProbes(MeshBakerStatus& status)
{
    PIXEvent pixEvent(L"Render Probes");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();
    ID3D11DevicePtr device = deviceManager.Device();

    const uint64 currSceneIdx = AppSettings::CurrentScene;
    const uint32 numProbes = uint32(AppSettings::NumProbes());

    if(probeCaptureMap.Width == 0 || AppSettings::ProbeCubemapCaptureRes.Changed())
    {
        currProbeIdx = 0;

        const uint32 resolution = AppSettings::ProbeCubemapCaptureRes;
        probeCaptureMap.Initialize(device, resolution, resolution, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 0, false, false, 6, true);
        probeDistanceCaptureMap.Initialize(device, resolution, resolution, DXGI_FORMAT_R16G16_UNORM, 1, 1, 0, false, false, 6, true);
        probeDepthBuffer.Initialize(device, resolution, resolution, DXGI_FORMAT_D24_UNORM_S8_UINT, true);
    }

    if(AppSettings::ProbeResX.Changed() || AppSettings::ProbeResY.Changed() ||
       AppSettings::ProbeResZ.Changed() || probeIrradianceCubeMap.ArraySize == 0 ||
       AppSettings::ProbeIrradianceCubemapRes.Changed() || AppSettings::ProbeDistanceCubemapRes.Changed() ||
       AppSettings::ProbeMode.Changed())
    {
        currProbeIdx = 0;

        if(AppSettings::ProbeMode == ProbeModes::CubeMap)
        {
            const uint32 irrResolution = AppSettings::ProbeIrradianceCubemapRes;
            const uint32 distResolution = AppSettings::ProbeDistanceCubemapRes;
            probeIrradianceCubeMap.Initialize(device, irrResolution, irrResolution, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 0, false, true, numProbes * 6, true);
            probeDistanceCubeMap.Initialize(device, distResolution, distResolution, DXGI_FORMAT_R16G16_UNORM, 1, 1, 0, false, true, numProbes * 6, true);
        }
        else
        {
            for(uint64 i = 0; i < AppSettings::MaxBasisCount; ++i)
            {
                probeVolumeMaps[i].Initialize(device, AppSettings::ProbeResX, AppSettings::ProbeResY, AppSettings::ProbeResZ,
                                              DXGI_FORMAT_R16G16B16A16_FLOAT, 1, true);
                probeDistanceVolumeMaps[i].Initialize(device, AppSettings::ProbeResX, AppSettings::ProbeResY, AppSettings::ProbeResZ,
                                                      DXGI_FORMAT_R16G16_UNORM, 1, true);
            }
        }
    }

    if(status.BakingInvalidated || AppSettings::SceneBoundsScale.Changed() ||
       AppSettings::AlwaysRegenerateProbes.Changed() || AppSettings::SceneBoundsOffsetX.Changed() ||
       AppSettings::SceneBoundsOffsetY.Changed() || AppSettings::SceneBoundsOffsetZ.Changed() ||
       AppSettings::DistanceFilterSharpness.Changed() || AppSettings::ProbeIntegrationSamples.Changed() ||
       AppSettings::ProbeDistanceIntegrationSamples.Changed())
        currProbeIdx = 0;

    if(currProbeIdx >= numProbes)
    {
        status.ProbeBakeProgress = 1.0f;
        return;
    }

    if(AppSettings::UseProbes == false)
        return;

    bool32 prevUseProbes = AppSettings::UseProbes;
    AppSettings::UseProbes.SetValue(false);

    uint64 probeX = currProbeIdx % AppSettings::ProbeResX;
    uint64 probeY = (currProbeIdx / AppSettings::ProbeResX) % AppSettings::ProbeResY;
    uint64 probeZ = currProbeIdx / (AppSettings::ProbeResX * AppSettings::ProbeResY);

    Float3 probePos;
    probePos.x = Lerp(currSceneMin.x, currSceneMax.x, (probeX + 0.5f) / AppSettings::ProbeResX);
    probePos.y = Lerp(currSceneMin.y, currSceneMax.y, (probeY + 0.5f) / AppSettings::ProbeResY);
    probePos.z = Lerp(currSceneMin.z, currSceneMax.z, (probeZ + 0.5f) / AppSettings::ProbeResZ);

    PerspectiveCamera probeCam(1.0f, Pi_2, NearClip, FarClip);
    probeCam.SetPosition(probePos);

    for(uint64 i = 0; i < 6; ++i)
    {
        Quaternion orientation;
        if(i == 0)
            orientation = Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), Pi_2);
        else if(i == 1)
            orientation = Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), -Pi_2);
        else if(i == 2)
            orientation = Quaternion::FromAxisAngle(Float3(1.0f, 0.0f, 0.0f), -Pi_2);
        else if(i == 3)
            orientation = Quaternion::FromAxisAngle(Float3(1.0f, 0.0f, 0.0f), Pi_2);
        else if(i == 4)
            orientation = Quaternion();
        else
            orientation = Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), Pi);

        probeCam.SetOrientation(orientation);
        RenderScene(status, probeCaptureMap.RTVArraySlices[i], probeDistanceCaptureMap.RTVArraySlices[i], probeDepthBuffer, probeCam,
                    false, false, AppSettings::BakeDirectAreaLight, false, false, true);
    }

    AppSettings::UseProbes.SetValue(prevUseProbes);

    ID3D11RenderTargetView* rtvs[2] = { };
    context->OMSetRenderTargets(2, rtvs, nullptr);

    if(AppSettings::ProbeMode == ProbeModes::CubeMap)
    {
        SetCSInputs(context, probeCaptureMap.SRView);
        SetCSOutputs(context, probeIrradianceCubeMap.UAView);
        SetCSSamplers(context, samplerStates.LinearClamp());
        SetCSShader(context, probeIntegrateIrradianceCubeMap);

        integrateConstants.Data.OutputTextureSize.x = float(probeIrradianceCubeMap.Width);
        integrateConstants.Data.OutputTextureSize.y = float(probeIrradianceCubeMap.Height);
        integrateConstants.Data.OutputProbeIdx = uint32(currProbeIdx);
        integrateConstants.ApplyChanges(context);
        integrateConstants.SetCS(context, 0);

        context->Dispatch(DispatchSize(8, probeIrradianceCubeMap.Width), DispatchSize(8, probeIrradianceCubeMap.Height), 6);

        ClearCSInputs(context);
        ClearCSOutputs(context);

        SetCSInputs(context, probeDistanceCaptureMap.SRView);
        SetCSOutputs(context, probeDistanceCubeMap.UAView);
        SetCSShader(context, probeIntegrateDistanceCubeMap);

        integrateConstants.Data.OutputTextureSize.x = float(probeDistanceCubeMap.Width);
        integrateConstants.Data.OutputTextureSize.y = float(probeDistanceCubeMap.Height);
        integrateConstants.Data.OutputProbeIdx = uint32(currProbeIdx);
        integrateConstants.ApplyChanges(context);

        context->Dispatch(DispatchSize(8, probeDistanceCubeMap.Width), DispatchSize(8, probeDistanceCubeMap.Height), 6);
    }
    else
    {
        SetCSInputs(context, probeCaptureMap.SRView);
        SetCSSamplers(context, samplerStates.LinearClamp());
        SetCSShader(context, probeIntegrateVolumeMap);

        ID3D11UnorderedAccessView* uavs[AppSettings::MaxBasisCount] = { };
        for(uint64 i = 0; i < AppSettings::MaxBasisCount; ++i)
            uavs[i] = probeVolumeMaps[i].UAView;
        context->CSSetUnorderedAccessViews(0, AppSettings::MaxBasisCount, uavs, nullptr);

        integrateConstants.Data.OutputTextureSize.x = 0.0f;
        integrateConstants.Data.OutputTextureSize.y = 0.0f;
        integrateConstants.Data.OutputProbeIdx = uint32(currProbeIdx);
        integrateConstants.ApplyChanges(context);
        integrateConstants.SetCS(context, 0);

        context->Dispatch(1, 1, 1);

        ClearCSInputs(context);
        ClearCSOutputs(context);

        SetCSInputs(context, probeDistanceCaptureMap.SRView);
        SetCSShader(context, probeIntegrateDistanceVolumeMap);

        for(uint64 i = 0; i < AppSettings::MaxBasisCount; ++i)
            uavs[i] = probeDistanceVolumeMaps[i].UAView;
        context->CSSetUnorderedAccessViews(0, AppSettings::MaxBasisCount, uavs, nullptr);

        integrateConstants.Data.OutputTextureSize.x = 0.0f;
        integrateConstants.Data.OutputTextureSize.y = 0.0f;
        integrateConstants.Data.OutputProbeIdx = uint32(currProbeIdx);
        integrateConstants.ApplyChanges(context);

        context->Dispatch(1, 1, 1);
    }

    ClearCSInputs(context);

    ID3D11UnorderedAccessView* uavs[AppSettings::MaxBasisCount] = { };
    context->CSSetUnorderedAccessViews(0, AppSettings::MaxBasisCount, uavs, nullptr);

    ++currProbeIdx;

    status.ProbeBakeProgress = currProbeIdx / (numProbes - 1.0f);

    if(currProbeIdx == numProbes && AppSettings::AlwaysRegenerateProbes)
        currProbeIdx = 0;
}

void BakingLab::VoxelizeScene(MeshBakerStatus& status)
{
    ID3D11Device* device = deviceManager.Device();
    ID3D11DeviceContext* context = deviceManager.ImmediateContext();

    bool reVoxelize = false;

    if(status.BakingInvalidated || AppSettings::AlwaysRevoxelize)
        reVoxelize = true;

    if(voxelRadiance.Texture == nullptr || AppSettings::VoxelResX.Changed() || AppSettings::VoxelResY.Changed() || AppSettings::VoxelResZ.Changed())
    {
        reVoxelize = true;
        const uint32 numMips = NumMipLevels(AppSettings::VoxelResX, AppSettings::VoxelResY, AppSettings::VoxelResZ);
        voxelRadiance.Initialize(device, AppSettings::VoxelResX, AppSettings::VoxelResY, AppSettings::VoxelResZ,
                                 DXGI_FORMAT_R16G16B16A16_FLOAT, numMips, true);
    }

    if(reVoxelize == false)
        return;

    currVoxelIdx = 0;

    PIXEvent pixEvent(L"Voxelize Scene");

    // Clear the voxel radiance texture
    SetCSShader(context, clearVoxelRadiance);
    SetCSOutputs(context, voxelRadiance.UAView);

    Uint3 voxelDispatchSize = Uint3(DispatchSize(4, AppSettings::VoxelResX),
                                    DispatchSize(4, AppSettings::VoxelResY),
                                    DispatchSize(4, AppSettings::VoxelResZ));
    context->Dispatch(voxelDispatchSize.x, voxelDispatchSize.y, voxelDispatchSize.z);

    ClearCSOutputs(context);

    Float3 sceneCenter = (currSceneMin + currSceneMax) / 2.0f;
    Float3 sceneHalfExtents = (currSceneMax - currSceneMin) / 2.0f;

    OrthographicCamera voxelCameraX(-sceneHalfExtents.z, -sceneHalfExtents.y, sceneHalfExtents.z, sceneHalfExtents.y, 0.0f, sceneHalfExtents.x * 2.0f);
    voxelCameraX.SetPosition(Float3(currSceneMin.x, sceneCenter.y, sceneCenter.z));
    voxelCameraX.SetOrientation(Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), Pi_2));

    OrthographicCamera voxelCameraY(-sceneHalfExtents.x, -sceneHalfExtents.z, sceneHalfExtents.x, sceneHalfExtents.z, 0.0f, sceneHalfExtents.y * 2.0f);
    voxelCameraY.SetPosition(Float3(sceneCenter.x, currSceneMin.y, sceneCenter.z));
    voxelCameraY.SetOrientation(Quaternion::FromAxisAngle(Float3(1.0f, 0.0f, 0.0f), -Pi_2));

    OrthographicCamera voxelCameraZ(-sceneHalfExtents.x, -sceneHalfExtents.y, sceneHalfExtents.x, sceneHalfExtents.y, 0.0f, sceneHalfExtents.z * 2.0f);
    voxelCameraZ.SetPosition(Float3(sceneCenter.x, sceneCenter.y, currSceneMin.z));

    if(AppSettings::EnableSun)
        meshRenderer.RenderSunShadowMap(context, voxelCameraZ, false);

    if(AppSettings::EnableAreaLight)
        meshRenderer.RenderAreaLightShadowMap(context, voxelCameraZ);

    ID3D11UnorderedAccessView* uavs[] = { voxelRadiance.UAView };
    context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, ArraySize_(uavs), uavs, nullptr);

    for(uint64 i = 0; i < 3; ++i)
    {
        OrthographicCamera* voxelCamera = nullptr;

        if(i == 0)
        {
            voxelCamera = &voxelCameraX;
            SetViewport(context, AppSettings::VoxelResZ, AppSettings::VoxelResY);
        }
        else if(i == 1)
        {
            voxelCamera = &voxelCameraY;
            SetViewport(context, AppSettings::VoxelResX, AppSettings::VoxelResZ);
        }
        else if(i == 2)
        {
            voxelCamera = &voxelCameraZ;
            SetViewport(context, AppSettings::VoxelResX, AppSettings::VoxelResY);
        }

        meshRenderer.RenderMainPass(context, *voxelCamera, status, false, true);
    }

    context->OMSetRenderTargets(0, nullptr, nullptr);

    // Clamp the occupancy to [0, 1]
    SetCSShader(context, fixVoxelRadianceOccupancy);
    SetCSOutputs(context, voxelRadiance.UAView);

    context->Dispatch(voxelDispatchSize.x, voxelDispatchSize.y, voxelDispatchSize.z);

    ClearCSOutputs(context);

    SetCSShader(context, generateVoxelMips);
    SetCSSamplers(context, samplerStates.LinearClamp());

    const uint32 numMips = voxelRadiance.NumMipLevels;
    Uint3 srcMipSize = Uint3(AppSettings::VoxelResX, AppSettings::VoxelResY, AppSettings::VoxelResZ);
    for(uint32 dstMipLevel = 1; dstMipLevel < numMips; ++dstMipLevel)
    {
        SetCSInputs(context, voxelRadiance.MipSRVs[dstMipLevel - 1]);
        SetCSOutputs(context, voxelRadiance.MipUAVs[dstMipLevel]);

        Uint3 dstMipSize = srcMipSize;
        dstMipSize.x = Max(dstMipSize.x / 2, 1u);
        dstMipSize.y = Max(dstMipSize.y / 2, 1u);
        dstMipSize.z = Max(dstMipSize.z / 2, 1u);

        generateMipConstants.Data.SrcMipTexelSize = Float3(1.0f / srcMipSize.x, 1.0f / srcMipSize.y, 1.0f / srcMipSize.z);
        generateMipConstants.Data.DstMipTexelSize = Float3(1.0f / dstMipSize.x, 1.0f / dstMipSize.y, 1.0f / dstMipSize.z);
        generateMipConstants.ApplyChanges(context);
        generateMipConstants.SetCS(context, 0);

        context->Dispatch(DispatchSize(4, dstMipSize.x), DispatchSize(4, dstMipSize.y), DispatchSize(4, dstMipSize.z));

        srcMipSize = dstMipSize;

        ClearCSOutputs(context);
    }
}

void BakingLab::Render(const Timer& timer)
{
    if(AppSettings::MSAAMode.Changed())
        CreateRenderTargets();

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    MeshBakerStatus status = meshBaker.Update(unJitteredCamera, colorTargetMSAA.Width, colorTargetMSAA.Height,
                                              context, &sceneModels[AppSettings::CurrentScene]);
    status.SceneMinBounds = currSceneMin;
    status.SceneMaxBounds = currSceneMax;

    VoxelizeScene(status);

    RenderProbes(status);

    status.VoxelRadiance = voxelRadiance.SRView;

    if(AppSettings::VoxelVisualizerMipLevel >= int32(voxelRadiance.NumMipLevels))
        AppSettings::VoxelVisualizerMipLevel.SetValue(voxelRadiance.NumMipLevels - 1);

    if(AppSettings::ShowGroundTruth)
    {
        ID3D11RenderTargetView* rtvs[1] = { colorTargetMSAA.RTView };
        context->OMSetRenderTargets(1, rtvs, nullptr);

        SetViewport(context, colorTargetMSAA.Width, colorTargetMSAA.Height);

        spriteRenderer.Begin(context, SpriteRenderer::Point, SpriteRenderer::OpaqueBlend);
        spriteRenderer.Render(status.GroundTruth, Float4x4());
        spriteRenderer.End();

        rtvs[0] = nullptr;
        context->OMSetRenderTargets(1, rtvs, nullptr);

        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->ClearRenderTargetView(velocityTargetMSAA.RTView, clearColor);
    }
    else
    {
        status.ProbeIrradianceCubeMap = probeIrradianceCubeMap.SRView;
        status.ProbeDistanceCubeMap = probeDistanceCubeMap.SRView;
        for(uint64 i = 0; i < AppSettings::MaxBasisCount; ++i)
        {
            status.ProbeVolumeMaps[i] = probeVolumeMaps[i].SRView;
            status.ProbeDistanceVolumeMaps[i] = probeDistanceVolumeMaps[i].SRView;
        }

        RenderScene(status, colorTargetMSAA.RTView, velocityTargetMSAA.RTView, depthBuffer, camera,
                    AppSettings::ShowBakeDataVisualizer, AppSettings::ShowProbeVisualizer,
                    AppSettings::EnableAreaLight, AppSettings::VoxelVisualizerMode != VoxelVisualizerModes::None,
                    true, false);
        RenderBackgroundVelocity();
    }

    RenderAA();

    if(AppSettings::SaveEXRScreenshot)
        SaveEXRScreenshot(window.GetHwnd(), colorResolveTarget.SRView);

    {
        // Kick off post-processing
        PIXEvent ppEvent(L"Post Processing");
        postProcessor.Render(context, colorResolveTarget.SRView, depthBuffer.SRView, camera,
                             deviceManager.BackBuffer(), timer.DeltaSecondsF());
    }

    ID3D11RenderTargetView* renderTargets[1] = { deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, renderTargets, NULL);

    SetViewport(context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());

    RenderHUD(timer, status.GroundTruthProgress, status.BakeProgress, status.GroundTruthSampleCount, status.ProbeBakeProgress);

    ++frameCount;
}

void BakingLab::RenderScene(const MeshBakerStatus& status, ID3D11RenderTargetView* colorTarget, ID3D11RenderTargetView* secondRT,
                            const DepthStencilBuffer& depth, const Camera& cam, bool32 showBakeDataVisualizer, bool32 showProbeVisualizer,
                            bool32 renderAreaLight, bool32 showVoxelVisualizer, bool32 enableSkySun, bool32 probeRendering)
{
    PIXEvent event(L"Render Scene");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    ID3D11RenderTargetView* renderTargets[2] = { nullptr, nullptr };
    ID3D11DepthStencilView* dsv = depth.DSView;
    context->OMSetRenderTargets(1, renderTargets, dsv);

    SetViewport(context, depth.Width, depth.Height);

    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    meshRenderer.RenderDepth(context, cam, false, false);

    meshRenderer.ReduceDepth(context, depth, cam);

    if(AppSettings::EnableSun)
        meshRenderer.RenderSunShadowMap(context, cam, true);

    if(AppSettings::EnableAreaLight)
        meshRenderer.RenderAreaLightShadowMap(context, cam);

    renderTargets[0] = colorTarget;
    renderTargets[1] = secondRT;
    context->OMSetRenderTargets(2, renderTargets, dsv);
    SetViewport(context, depth.Width, depth.Height);

    float maxDistance = Float3::Length(FarClip);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float secondClearColor[4] = { maxDistance, maxDistance * maxDistance, 0.0f, 0.0f };
    context->ClearRenderTargetView(colorTarget, clearColor);
    context->ClearRenderTargetView(secondRT, secondClearColor);

    if(showVoxelVisualizer)
    {
       context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
    else
    {
        meshRenderer.RenderMainPass(context, cam, status, probeRendering, false);

        if(showBakeDataVisualizer)
            meshRenderer.RenderBakeDataVisualizer(context, cam, status);

        if(showProbeVisualizer)
            meshRenderer.RenderProbeVisualizer(context, cam, status);
    }

    if(renderAreaLight)
        meshRenderer.RenderAreaLight(context, cam);

    if(AppSettings::SkyMode == SkyModes::Procedural)
    {
        float sunSize = (AppSettings::EnableSun && enableSkySun) ? AppSettings::SunSize : 0.0f;
        skybox.RenderSky(context, AppSettings::SunDirection, AppSettings::GroundAlbedo,
                         AppSettings::SunLuminance(), sunSize,
                         AppSettings::Turbidity, cam.ViewMatrix(),
                         cam.ProjectionMatrix(), 1.0f);
    }
    else if(AppSettings::SkyMode == SkyModes::Simple)
    {
        float sunSize = (AppSettings::EnableSun && enableSkySun) ? AppSettings::SunSize : 0.0f;
        skybox.RenderSimpleSky(context, AppSettings::SkyColor, AppSettings::SunDirection,
                               AppSettings::SunLuminance(), sunSize,
                               cam.ViewMatrix(), cam.ProjectionMatrix(), FP16Scale);
    }
    else if(AppSettings::SkyMode >= AppSettings::CubeMapStart)
    {
        skybox.RenderEnvironmentMap(context, envMaps[AppSettings::SkyMode - AppSettings::CubeMapStart],
                                    cam.ViewMatrix(), cam.ProjectionMatrix(), 1.0f);
    }

    if(showVoxelVisualizer)
        meshRenderer.RenderVoxelVisualizer(context, cam, status);
}

void BakingLab::RenderAA()
{
    PIXEvent pixEvent(L"MSAA Resolve + Temporal AA");

    ID3D11DeviceContext* context = deviceManager.ImmediateContext();

    ID3D11RenderTargetView* rtvs[1] = { colorResolveTarget.RTView };

    context->OMSetRenderTargets(1, rtvs, nullptr);

    const uint32 SampleRadius = static_cast<uint32>((AppSettings::FilterSize / 2.0f) + 0.499f);
    ID3D11PixelShader* pixelShader = resolvePS[AppSettings::MSAAMode];
    context->PSSetShader(pixelShader, nullptr, 0);
    context->VSSetShader(resolveVS, nullptr, 0);

    resolveConstants.Data.TextureSize = Float2(float(colorTargetMSAA.Width), float(colorTargetMSAA.Height));
    resolveConstants.Data.SampleRadius = SampleRadius;
    resolveConstants.Data.EnableTemporalAA = enableTAA;
    resolveConstants.ApplyChanges(context);
    resolveConstants.SetPS(context, 0);

    ID3D11ShaderResourceView* srvs[4] = { colorTargetMSAA.SRView, velocityTargetMSAA.SRView,
                                          prevFrameTarget.SRView, postProcessor.AdaptedLuminance() };
    context->PSSetShaderResources(0, 4, srvs);

    ID3D11SamplerState* samplers[1] = { samplerStates.LinearClamp() };
    context->PSSetSamplers(0, 1, samplers);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);

    srvs[0] = srvs[1] = srvs[2] = srvs[3] = nullptr;
    context->PSSetShaderResources(0, 4, srvs);

    context->CopyResource(prevFrameTarget.Texture, colorResolveTarget.Texture);
}

void BakingLab::RenderBackgroundVelocity()
{
    PIXEvent pixEvent(L"Render Background Velocity");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    SetViewport(context, velocityTargetMSAA.Width, velocityTargetMSAA.Height);

    // Don't use camera translation for background velocity
    FirstPersonCamera tempCamera = camera;
    tempCamera.SetPosition(Float3(0.0f, 0.0f, 0.0f));

    backgroundVelocityConstants.Data.InvViewProjection = Float4x4::Transpose(Float4x4::Invert(tempCamera.ViewProjectionMatrix()));
    backgroundVelocityConstants.Data.PrevViewProjection = Float4x4::Transpose(prevViewProjection);
    backgroundVelocityConstants.Data.RTSize.x = float(velocityTargetMSAA.Width);
    backgroundVelocityConstants.Data.RTSize.y = float(velocityTargetMSAA.Height);
    backgroundVelocityConstants.Data.JitterOffset = jitterOffset;
    backgroundVelocityConstants.ApplyChanges(context);
    backgroundVelocityConstants.SetPS(context, 0);

    prevViewProjection = tempCamera.ViewProjectionMatrix();

    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthEnabled(), 0);
    context->RSSetState(rasterizerStates.NoCull());

    ID3D11RenderTargetView* rtvs[1] = { velocityTargetMSAA.RTView };
    context->OMSetRenderTargets(1, rtvs, depthBuffer.DSView);

    context->VSSetShader(backgroundVelocityVS, nullptr, 0);
    context->PSSetShader(backgroundVelocityPS, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);
}

void BakingLab::RenderHUD(const Timer& timer, float groundTruthProgress, float bakeProgress,
                         uint64 groundTruthSampleCount, float probeBakeProgress)
{
    PIXEvent event(L"HUD Pass");

    ID3D11DeviceContext* context = deviceManager.ImmediateContext();
    spriteRenderer.Begin(context, SpriteRenderer::Point);

    Float4x4 transform = Float4x4::TranslationMatrix(Float3(25.0f, 25.0f, 0.0f));
    wstring fpsText(L"FPS: ");
    fpsText += ToString(fps) + L" (" + ToString(1000.0f / fps) + L"ms)";
    spriteRenderer.RenderText(font, fpsText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    transform._42 += 25.0f;
    wstring vsyncText(L"VSYNC (V): ");
    vsyncText += deviceManager.VSYNCEnabled() ? L"Enabled" : L"Disabled";
    spriteRenderer.RenderText(font, vsyncText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    Profiler::GlobalProfiler.EndFrame(spriteRenderer, font);

    if(groundTruthProgress < 1.0f || bakeProgress < 1.0f)
    {
        std::wstring progressText;
        if(groundTruthProgress < 1.0f)
        {
            float percent = Round(groundTruthProgress * 10000.0f);
            percent /= 100.0f;
            progressText = L"Rendering ground truth (" + ToString(percent) + L"%)";
            if(groundTruthSampleCount > 0)
            {
                const uint64 currIdx = GTSampleRateBufferIdx++;
                const uint64 bufferSize = ArraySize_(GTSampleRateBuffer);
                GTSampleRateBuffer[currIdx % bufferSize] = groundTruthSampleCount / timer.DeltaMicrosecondsF();

                float samplesPerMS = 0.0f;
                for(uint64 i = 0; i < bufferSize; ++i)
                    samplesPerMS +=  GTSampleRateBuffer[i];
                samplesPerMS /= float(bufferSize);
                samplesPerMS = Round(samplesPerMS * 1000.0f);

                progressText += L" [" + ToString(samplesPerMS) + L" samp/sec]";
            }
        }
        else if(bakeProgress < 1.0f)
        {
            float percent = Round(bakeProgress * 10000.0f);
            percent /= 100.0f;
            progressText = L"Baking light maps (" + ToString(percent) + L"%)";
        }

        transform._41 = 35.0f;
        transform._42 = deviceManager.BackBufferHeight() - 60.0f;
        spriteRenderer.RenderText(font, progressText.c_str(), transform);
    }

    if(probeBakeProgress < 1.0f && AppSettings::UseProbes)
    {
        float percent = Round(probeBakeProgress * 10000.0f);
        percent /= 100.0f;
        std::wstring progressText = L"Baking probes (" + ToString(percent) + L"%)";

        transform._41 = 35.0f;
        transform._42 = deviceManager.BackBufferHeight() - 40.0f;
        spriteRenderer.RenderText(font, progressText.c_str(), transform);
    }

    if(AppSettings::EnableLuminancePicker && mouseState.IsOverWindow)
    {
        const uint8* texels = nullptr;
        uint32 pitch = 0;

        Half4 texel;

        if(AppSettings::ShowGroundTruth)
        {
            texel = meshBaker.renderBuffer[mouseState.Y * colorTargetMSAA.Width + mouseState.X];
        }
        else
        {
            context->CopyResource(readbackTexture.Texture, colorResolveTarget.Texture);
            const uint8* texels = reinterpret_cast<const uint8*>(readbackTexture.Map(context, 0, pitch));
            texel = *reinterpret_cast<const Half4*>(texels + (mouseState.Y * pitch) + mouseState.X * sizeof(Half4));
            readbackTexture.Unmap(context, 0);
        }

        Float3 color = Float4(texel.ToSIMD()).To3D();
        float illuminance = Float4(texel.ToSIMD()).w;
        illuminance *= (1.0f / FP16Scale);
        color *= (1.0f / FP16Scale);
        float luminance = ComputeLuminance(color);

        std::wstring pickerText = L"Pixel Luminance: " + ToString(luminance) + L" cd/m^2" +
                                  L"       RGB(" +
                                    ToString(color.x) + L", " +
                                    ToString(color.y) + L", " +
                                    ToString(color.z) + L")";
        transform._41 = 35.0f;
        transform._42 = deviceManager.BackBufferHeight() - 120.0f;
        spriteRenderer.RenderText(font, pickerText.c_str(), transform);

        pickerText = L"Pixel Illuminance: " + ToString(illuminance) + L" lux";
        transform._41 = 35.0f;
        transform._42 = deviceManager.BackBufferHeight() - 100.0f;
        spriteRenderer.RenderText(font, pickerText.c_str(), transform);
    }

    if(AppSettings::ShowSunIntensity)
    {
        Float3 sunIlluminance = AppSettings::SunIlluminance() / FP16Scale;
        float intensity = Max(sunIlluminance.x, Max(sunIlluminance.y, sunIlluminance.z));
        Float3 rgb = 0.0f;
        if(intensity > 0.0f)
            rgb = (sunIlluminance / intensity);

        wstring intensityText = L"Sun Intensity: " + ToString(intensity);
        intensityText += L" - R: " + ToString(rgb.x) + L" G: " + ToString(rgb.y) + L" B: " + ToString(rgb.z);
        transform._41 = 35.0f;
        transform._42 = deviceManager.BackBufferHeight() - 80.0f;
        spriteRenderer.RenderText(font, intensityText.c_str(), transform);
    }

    spriteRenderer.End();
}

static void GenerateGaussianIrradianceTable(float sharpness, const wchar* filePath)
{
    std::string output;

    const uint64 NumPoints = 50;
    for(uint64 pointIdx = 0; pointIdx < NumPoints; ++pointIdx)
    {
        float theta = Pi * pointIdx / (NumPoints - 1.0f);
        Float3 localSGDir = Float3(std::sin(-theta), 0.0f, std::cos(-theta));

        const uint64 SqrtNumSamples = 64;
        const uint64 NumSamples = SqrtNumSamples * SqrtNumSamples;
        float sum = 0.0f;
        for(uint64 sampleIdx = 0; sampleIdx < NumSamples; ++sampleIdx)
        {
            Float2 samplePoint = SampleCMJ2D(int32(sampleIdx), int32(SqrtNumSamples), int32(SqrtNumSamples), int32(pointIdx));
            Float3 sampleDir = SampleCosineHemisphere(samplePoint.x, samplePoint.y);
            sum += std::exp(sharpness * (Float3::Dot(sampleDir, localSGDir) - 1.0f));
        }

        sum *= (Pi / NumSamples);

        output += MakeAnsiString("%f,%f\n", theta, sum);
    }

    WriteStringAsFile(filePath, output);
}

static void GenerateSGInnerProductIrradianceTable(float sharpness, const wchar* filePath)
{
    std::string output;

    SG sgLight;
    sgLight.Amplitude = 1.0f;
    sgLight.Axis = Float3(0.0f, 0.0f, 1.0f);
    sgLight.Sharpness = sharpness;

    const uint64 NumPoints = 50;
    for(uint64 pointIdx = 0; pointIdx < NumPoints; ++pointIdx)
    {
        float theta = Pi * pointIdx / (NumPoints - 1.0f);
        Float3 normal = Float3(std::sin(theta), 0.0f, std::cos(theta));

        SG cosineLobe = CosineLobeSG(normal);
        float irradiance = Max(SGInnerProduct(sgLight, cosineLobe).x, 0.0f);

        output += MakeAnsiString("%f,%f\n", theta, irradiance);
    }

    WriteStringAsFile(filePath, output);
}

static void GenerateSGFittedIrradianceTable(float sharpness, const wchar* filePath)
{
    std::string output;

    SG sgLight;
    sgLight.Amplitude = 1.0f;
    sgLight.Axis = Float3(0.0f, 0.0f, 1.0f);
    sgLight.Sharpness = sharpness;

    const uint64 NumPoints = 50;
    for(uint64 pointIdx = 0; pointIdx < NumPoints; ++pointIdx)
    {
        float theta = Pi * pointIdx / (NumPoints - 1.0f);
        Float3 normal = Float3(std::sin(theta), 0.0f, std::cos(theta));

        float irradiance = SGIrradianceFitted(sgLight, normal).x;

        output += MakeAnsiString("%f,%f\n", theta, irradiance);
    }

    WriteStringAsFile(filePath, output);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // GenerateGaussianIrradianceTable(4.0f, L"SG_Irradiance_4.0.txt");
    // GenerateSGInnerProductIrradianceTable(4.0f, L"SG_InnerProduct_Irradiance_4.0.txt");
    // GenerateSGFittedIrradianceTable(4.0f, L"SG_Fitted_Irradiance_4.0.txt");

    BakingLab app;
    app.Run();
}
