#include <PCH.h>
#include <TwHelper.h>
#include "AppSettings.h"

using namespace SampleFramework11;

static const char* SunDirectionTypesLabels[2] =
{
    "Unit Vector",
    "Horizontal Coordinate System",
};

static const char* SkyModesLabels[6] =
{
    "None",
    "Procedural",
    "Simple",
    "Ennis",
    "Grace Cathedral",
    "Uffizi Cross",
};

static const char* LightUnitsLabels[4] =
{
    "Luminance",
    "Illuminance",
    "LuminousPower",
    "EV100",
};

static const char* ExposureModesLabels[4] =
{
    "Manual (Simple)",
    "Manual (SBS)",
    "Manual (SOS)",
    "Automatic",
};

static const char* FStopsLabels[23] =
{
    "f/1.8",
    "f/2.0",
    "f/2.2",
    "f/2.5",
    "f/2.8",
    "f/3.2",
    "f/3.5",
    "f/4.0",
    "f/4.5",
    "f/5.0",
    "f/5.6",
    "f/6.3",
    "f/7.1",
    "f/8.0",
    "f/9.0",
    "f/10.0",
    "f/11.0",
    "f/13.0",
    "f/14.0",
    "f/16.0",
    "f/18.0",
    "f/20.0",
    "f/22.0",
};

static const char* ISORatingsLabels[4] =
{
    "ISO100",
    "ISO200",
    "ISO400",
    "ISO800",
};

static const char* ShutterSpeedsLabels[13] =
{
    "1s",
    "1/2s",
    "1/4s",
    "1/8s",
    "1/15s",
    "1/30s",
    "1/60s",
    "1/125s",
    "1/250s",
    "1/500s",
    "1/1000s",
    "1/2000s",
    "1/4000s",
};

static const char* ToneMappingModesLabels[5] =
{
    "Film Stock",
    "Linear",
    "ACES sRGB Monitor",
    "Hejl 2015",
    "Hable (Uncharted2)",
};

static const char* MSAAModesLabels[4] =
{
    "None",
    "2x",
    "4x",
    "8x",
};

static const char* FilterTypesLabels[6] =
{
    "Box",
    "Triangle",
    "Gaussian",
    "BlackmanHarris",
    "Smoothstep",
    "BSpline",
};

static const char* JitterModesLabels[5] =
{
    "None",
    "Uniform2x",
    "Hammersley4x",
    "Hammersley8x",
    "Hammersley16x",
};

static const char* SGDiffuseModesLabels[3] =
{
    "InnerProduct",
    "Punctual",
    "Fitted (Hill 16)",
};

static const char* SampleModesLabels[5] =
{
    "Random",
    "Stratified",
    "Hammersley",
    "UniformGrid",
    "CMJ",
};

static const char* BakeModesLabels[10] =
{
    "Diffuse",
    "Half-Life 2",
    "L1 SH",
    "L2 SH",
    "L1 H-basis",
    "L2 H-basis",
    "SG5",
    "SG6",
    "SG9",
    "SG12",
};

static const char* SolveModesLabels[3] =
{
    "Ad-Hoc Projection",
    "Least Squares",
    "Non-Negative Least Squares",
};

static const char* ProbeModesLabels[4] =
{
    "CubeMap",
    "AmbientCube",
    "L1 SH",
    "L2 SH",
};

static const char* ScenesLabels[3] =
{
    "Box",
    "White Room",
    "Sponza",
};

static const char* VoxelVisualizerModesLabels[3] =
{
    "None",
    "Geometry",
    "RayMarch",
};

namespace AppSettings
{
    BoolSetting EnableSun;
    BoolSetting SunAreaLightApproximation;
    ColorSetting SunTintColor;
    FloatSetting SunIntensityScale;
    FloatSetting SunSize;
    BoolSetting NormalizeSunIntensity;
    SunDirectionTypesSetting SunDirType;
    DirectionSetting SunDirection;
    FloatSetting SunAzimuth;
    FloatSetting SunElevation;
    SkyModesSetting SkyMode;
    ColorSetting SkyColor;
    FloatSetting Turbidity;
    ColorSetting GroundAlbedo;
    BoolSetting EnableAreaLight;
    ColorSetting AreaLightColor;
    FloatSetting AreaLightIlluminance;
    FloatSetting AreaLightLuminousPower;
    FloatSetting AreaLightEV100;
    LightUnitsSetting AreaLightUnits;
    FloatSetting AreaLightIlluminanceDistance;
    FloatSetting AreaLightSize;
    FloatSetting AreaLightX;
    FloatSetting AreaLightY;
    FloatSetting AreaLightZ;
    FloatSetting AreaLightShadowBias;
    BoolSetting BakeDirectAreaLight;
    BoolSetting EnableAreaLightShadows;
    ExposureModesSetting ExposureMode;
    FloatSetting ManualExposure;
    FStopsSetting ApertureSize;
    ISORatingsSetting ISORating;
    ShutterSpeedsSetting ShutterSpeed;
    FloatSetting FilmSize;
    FloatSetting FocalLength;
    FloatSetting FocusDistance;
    IntSetting NumBlades;
    BoolSetting EnableDOF;
    FloatSetting KeyValue;
    FloatSetting AdaptationRate;
    FloatSetting ApertureFNumber;
    FloatSetting ApertureWidth;
    FloatSetting ShutterSpeedValue;
    FloatSetting ISO;
    FloatSetting BokehPolygonAmount;
    ToneMappingModesSetting ToneMappingMode;
    FloatSetting WhitePoint_Hejl;
    FloatSetting ShoulderStrength;
    FloatSetting LinearStrength;
    FloatSetting LinearAngle;
    FloatSetting ToeStrength;
    FloatSetting WhitePoint_Hable;
    MSAAModesSetting MSAAMode;
    FilterTypesSetting FilterType;
    FloatSetting FilterSize;
    FloatSetting GaussianSigma;
    BoolSetting EnableTemporalAA;
    JitterModesSetting JitterMode;
    FloatSetting JitterScale;
    SGDiffuseModesSetting SGDiffuseMode;
    BoolSetting UseASGWarp;
    IntSetting LightMapResolution;
    IntSetting NumBakeSamples;
    SampleModesSetting BakeSampleMode;
    IntSetting MaxBakePathLength;
    IntSetting BakeRussianRouletteDepth;
    FloatSetting BakeRussianRouletteProbability;
    BakeModesSetting BakeMode;
    SolveModesSetting SolveMode;
    BoolSetting UseProbes;
    ProbeModesSetting ProbeMode;
    IntSetting ProbeResX;
    IntSetting ProbeResY;
    IntSetting ProbeResZ;
    IntSetting ProbeCubemapCaptureRes;
    IntSetting ProbeIrradianceCubemapRes;
    IntSetting ProbeDistanceCubemapRes;
    FloatSetting SceneBoundsScale;
    BoolSetting WeightProbesByNormal;
    BoolSetting WeightProbesByVisibility;
    FloatSetting DistanceFilterSharpness;
    IntSetting ProbeIntegrationSamples;
    IntSetting ProbeDistanceIntegrationSamples;
    BoolSetting BakeWithVCT;
    IntSetting VoxelResX;
    IntSetting VoxelResY;
    IntSetting VoxelResZ;
    ScenesSetting CurrentScene;
    BoolSetting EnableDiffuse;
    BoolSetting EnableSpecular;
    BoolSetting EnableDirectLighting;
    BoolSetting EnableIndirectLighting;
    BoolSetting EnableIndirectDiffuse;
    BoolSetting EnableIndirectSpecular;
    BoolSetting EnableAlbedoMaps;
    BoolSetting EnableNormalMaps;
    FloatSetting NormalMapIntensity;
    FloatSetting DiffuseAlbedoScale;
    FloatSetting RoughnessScale;
    BoolSetting ShowGroundTruth;
    IntSetting NumRenderSamples;
    SampleModesSetting RenderSampleMode;
    IntSetting MaxRenderPathLength;
    IntSetting RenderRussianRouletteDepth;
    FloatSetting RenderRussianRouletteProbability;
    BoolSetting EnableRenderBounceSpecular;
    FloatSetting BloomExposure;
    FloatSetting BloomMagnitude;
    FloatSetting BloomBlurSigma;
    BoolSetting EnableLuminancePicker;
    BoolSetting ShowBakeDataVisualizer;
    BoolSetting ShowProbeVisualizer;
    BoolSetting ViewIndirectSpecular;
    VoxelVisualizerModesSetting VoxelVisualizerMode;
    Button SaveLightSettings;
    Button LoadLightSettings;
    Button SaveEXRScreenshot;
    BoolSetting ShowSunIntensity;
    BoolSetting AlwaysRegenerateProbes;
    BoolSetting AlwaysRevoxelize;
    FloatSetting SceneBoundsOffsetX;
    FloatSetting SceneBoundsOffsetY;
    FloatSetting SceneBoundsOffsetZ;

    ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device)
    {
        TwBar* tweakBar = Settings.TweakBar();

        EnableSun.Initialize(tweakBar, "EnableSun", "Sun Light", "Enable Sun", "Enables the sun light", true);
        Settings.AddSetting(&EnableSun);

        SunAreaLightApproximation.Initialize(tweakBar, "SunAreaLightApproximation", "Sun Light", "Sun Area Light Approximation", "Controls whether the sun is treated as a disc area light in the real-time shader", true);
        Settings.AddSetting(&SunAreaLightApproximation);

        SunTintColor.Initialize(tweakBar, "SunTintColor", "Sun Light", "Sun Tint Color", "The color of the sun", Float3(1.0000f, 1.0000f, 1.0000f), false, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ColorUnit::None);
        Settings.AddSetting(&SunTintColor);

        SunIntensityScale.Initialize(tweakBar, "SunIntensityScale", "Sun Light", "Sun Intensity Scale", "Scale the intensity of the sun", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunIntensityScale);

        SunSize.Initialize(tweakBar, "SunSize", "Sun Light", "Sun Size", "Angular radius of the sun in degrees", 0.2700f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunSize);

        NormalizeSunIntensity.Initialize(tweakBar, "NormalizeSunIntensity", "Sun Light", "Normalize Sun Intensity", "", false);
        Settings.AddSetting(&NormalizeSunIntensity);

        SunDirType.Initialize(tweakBar, "SunDirType", "Sun Light", "Sun Dir Type", "Input direction type for the sun", SunDirectionTypes::HorizontalCoordSystem, 2, SunDirectionTypesLabels);
        Settings.AddSetting(&SunDirType);

        SunDirection.Initialize(tweakBar, "SunDirection", "Sun Light", "Sun Direction", "Director of the sun", Float3(-0.7500f, 0.9770f, -0.4000f));
        Settings.AddSetting(&SunDirection);

        SunAzimuth.Initialize(tweakBar, "SunAzimuth", "Sun Light", "Sun Azimuth", "Angle around the horizon", 0.0000f, 0.0000f, 360.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunAzimuth);

        SunElevation.Initialize(tweakBar, "SunElevation", "Sun Light", "Sun Elevation", "Elevation of sun from ground. 0 degrees is aligned on the horizon while 90 degrees is directly overhead", 0.0000f, 0.0000f, 90.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunElevation);

        SkyMode.Initialize(tweakBar, "SkyMode", "Sky", "Sky Mode", "Controls the sky used for GI baking and background rendering", SkyModes::Procedural, 6, SkyModesLabels);
        Settings.AddSetting(&SkyMode);

        SkyColor.Initialize(tweakBar, "SkyColor", "Sky", "Sky Color", "The color of the simple sky", Float3(1400.0000f, 3500.0000f, 7000.0000f), true, 0.0000f, 1000000.0000f, 0.1000f, ColorUnit::Luminance);
        Settings.AddSetting(&SkyColor);

        Turbidity.Initialize(tweakBar, "Turbidity", "Sky", "Turbidity", "", 2.0000f, 1.0000f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&Turbidity);

        GroundAlbedo.Initialize(tweakBar, "GroundAlbedo", "Sky", "Ground Albedo", "", Float3(0.5000f, 0.5000f, 0.5000f), false, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ColorUnit::None);
        Settings.AddSetting(&GroundAlbedo);

        EnableAreaLight.Initialize(tweakBar, "EnableAreaLight", "Area Light", "Enable Area Light", "Enables the area light during baking and ground truth rendering", false);
        Settings.AddSetting(&EnableAreaLight);

        AreaLightColor.Initialize(tweakBar, "AreaLightColor", "Area Light", "Color", "The color of the area light", Float3(1000000.0000f, 1000000.0000f, 1000000.0000f), true, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.1000f, ColorUnit::Luminance);
        Settings.AddSetting(&AreaLightColor);

        AreaLightIlluminance.Initialize(tweakBar, "AreaLightIlluminance", "Area Light", "Color Intensity (lux)", "", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightIlluminance);
        AreaLightIlluminance.SetVisible(false);

        AreaLightLuminousPower.Initialize(tweakBar, "AreaLightLuminousPower", "Area Light", "Color Intensity (lm)", "", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightLuminousPower);
        AreaLightLuminousPower.SetVisible(false);

        AreaLightEV100.Initialize(tweakBar, "AreaLightEV100", "Area Light", "Color Intensity (EV100)", "", 0.0000f, -64.0000f, 64.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightEV100);
        AreaLightEV100.SetVisible(false);

        AreaLightUnits.Initialize(tweakBar, "AreaLightUnits", "Area Light", "Units", "", LightUnits::Luminance, 4, LightUnitsLabels);
        Settings.AddSetting(&AreaLightUnits);

        AreaLightIlluminanceDistance.Initialize(tweakBar, "AreaLightIlluminanceDistance", "Area Light", "Illuminance Distance", "", 10.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightIlluminanceDistance);
        AreaLightIlluminanceDistance.SetVisible(false);

        AreaLightSize.Initialize(tweakBar, "AreaLightSize", "Area Light", "Size", "The radius of the area light", 0.5000f, 0.0100f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightSize);

        AreaLightX.Initialize(tweakBar, "AreaLightX", "Area Light", "Position X", "The X coordinate of the area light", 0.0000f, -100.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightX);

        AreaLightY.Initialize(tweakBar, "AreaLightY", "Area Light", "Position Y", "The Y coordinate of the area light", 5.0000f, -100.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightY);

        AreaLightZ.Initialize(tweakBar, "AreaLightZ", "Area Light", "Position Z", "The Z coordinate of the area light", 0.0000f, -100.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightZ);

        AreaLightShadowBias.Initialize(tweakBar, "AreaLightShadowBias", "Area Light", "Shadow Bias", "", 0.0010f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AreaLightShadowBias);

        BakeDirectAreaLight.Initialize(tweakBar, "BakeDirectAreaLight", "Area Light", "Bake Direct Area Light", "Bakes the direct contribution from the area light into the light map", false);
        Settings.AddSetting(&BakeDirectAreaLight);

        EnableAreaLightShadows.Initialize(tweakBar, "EnableAreaLightShadows", "Area Light", "Enable Area Light Shadows", "", true);
        Settings.AddSetting(&EnableAreaLightShadows);

        ExposureMode.Initialize(tweakBar, "ExposureMode", "Camera Controls", "Exposure Mode", "Specifies how exposure should be controled", ExposureModes::Manual_SBS, 4, ExposureModesLabels);
        Settings.AddSetting(&ExposureMode);

        ManualExposure.Initialize(tweakBar, "ManualExposure", "Camera Controls", "Manual Exposure", "Manual exposure value when auto-exposure is disabled", -16.0000f, -32.0000f, 32.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ManualExposure);

        ApertureSize.Initialize(tweakBar, "ApertureSize", "Camera Controls", "Aperture", "", FStops::FStop16Point0, 23, FStopsLabels);
        Settings.AddSetting(&ApertureSize);

        ISORating.Initialize(tweakBar, "ISORating", "Camera Controls", "ISO Rating", "", ISORatings::ISO100, 4, ISORatingsLabels);
        Settings.AddSetting(&ISORating);

        ShutterSpeed.Initialize(tweakBar, "ShutterSpeed", "Camera Controls", "Shutter Speed", "", ShutterSpeeds::ShutterSpeed1Over125, 13, ShutterSpeedsLabels);
        Settings.AddSetting(&ShutterSpeed);

        FilmSize.Initialize(tweakBar, "FilmSize", "Camera Controls", "Film Size(mm)", "", 35.0000f, 1.0000f, 100.0000f, 0.1000f, ConversionMode::None, 0.0010f);
        Settings.AddSetting(&FilmSize);

        FocalLength.Initialize(tweakBar, "FocalLength", "Camera Controls", "Focal Length(mm)", "", 35.0000f, 1.0000f, 200.0000f, 0.1000f, ConversionMode::None, 0.0010f);
        Settings.AddSetting(&FocalLength);

        FocusDistance.Initialize(tweakBar, "FocusDistance", "Camera Controls", "Focus Distance", "", 10.0000f, 1.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&FocusDistance);

        NumBlades.Initialize(tweakBar, "NumBlades", "Camera Controls", "Num Aperture Blades", "", 5, 5, 9);
        Settings.AddSetting(&NumBlades);

        EnableDOF.Initialize(tweakBar, "EnableDOF", "Camera Controls", "Enable DOF", "", false);
        Settings.AddSetting(&EnableDOF);

        KeyValue.Initialize(tweakBar, "KeyValue", "Camera Controls", "Auto-Exposure Key Value", "Parameter that biases the result of the auto-exposure pass", 0.1150f, 0.0000f, 0.5000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&KeyValue);

        AdaptationRate.Initialize(tweakBar, "AdaptationRate", "Camera Controls", "Adaptation Rate", "Controls how quickly auto-exposure adapts to changes in scene brightness", 0.5000f, 0.0000f, 4.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AdaptationRate);

        ApertureFNumber.Initialize(tweakBar, "ApertureFNumber", "Camera Controls", "Aperture FNumber", "", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ApertureFNumber);
        ApertureFNumber.SetVisible(false);
        ApertureFNumber.SetEditable(false);

        ApertureWidth.Initialize(tweakBar, "ApertureWidth", "Camera Controls", "Aperture Width", "", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ApertureWidth);
        ApertureWidth.SetVisible(false);
        ApertureWidth.SetEditable(false);

        ShutterSpeedValue.Initialize(tweakBar, "ShutterSpeedValue", "Camera Controls", "Shutter Speed Value", "", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ShutterSpeedValue);
        ShutterSpeedValue.SetVisible(false);
        ShutterSpeedValue.SetEditable(false);

        ISO.Initialize(tweakBar, "ISO", "Camera Controls", "ISO", "", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ISO);
        ISO.SetVisible(false);
        ISO.SetEditable(false);

        BokehPolygonAmount.Initialize(tweakBar, "BokehPolygonAmount", "Camera Controls", "Bokeh Polygon Amount", "", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BokehPolygonAmount);
        BokehPolygonAmount.SetVisible(false);
        BokehPolygonAmount.SetEditable(false);

        ToneMappingMode.Initialize(tweakBar, "ToneMappingMode", "Tone Mapping", "Tone Mapping Operator", "", ToneMappingModes::ACES, 5, ToneMappingModesLabels);
        Settings.AddSetting(&ToneMappingMode);

        WhitePoint_Hejl.Initialize(tweakBar, "WhitePoint_Hejl", "Tone Mapping", "White Point (Hejl2015)", "", 1.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&WhitePoint_Hejl);

        ShoulderStrength.Initialize(tweakBar, "ShoulderStrength", "Tone Mapping", "Shoulder Strength", "", 4.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ShoulderStrength);

        LinearStrength.Initialize(tweakBar, "LinearStrength", "Tone Mapping", "Linear Strength", "", 5.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&LinearStrength);

        LinearAngle.Initialize(tweakBar, "LinearAngle", "Tone Mapping", "Linear Angle", "", 0.1200f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&LinearAngle);

        ToeStrength.Initialize(tweakBar, "ToeStrength", "Tone Mapping", "Toe Strength", "", 13.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ToeStrength);

        WhitePoint_Hable.Initialize(tweakBar, "WhitePoint_Hable", "Tone Mapping", "White Point (Hable)", "", 6.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&WhitePoint_Hable);

        MSAAMode.Initialize(tweakBar, "MSAAMode", "Anti Aliasing", "MSAAMode", "", MSAAModes::MSAA4x, 4, MSAAModesLabels);
        Settings.AddSetting(&MSAAMode);

        FilterType.Initialize(tweakBar, "FilterType", "Anti Aliasing", "Filter Type", "", FilterTypes::Smoothstep, 6, FilterTypesLabels);
        Settings.AddSetting(&FilterType);

        FilterSize.Initialize(tweakBar, "FilterSize", "Anti Aliasing", "Filter Size", "", 2.0000f, 0.0000f, 6.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&FilterSize);

        GaussianSigma.Initialize(tweakBar, "GaussianSigma", "Anti Aliasing", "Gaussian Sigma", "", 0.5000f, 0.0100f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&GaussianSigma);

        EnableTemporalAA.Initialize(tweakBar, "EnableTemporalAA", "Anti Aliasing", "Enable Temporal AA", "", true);
        Settings.AddSetting(&EnableTemporalAA);

        JitterMode.Initialize(tweakBar, "JitterMode", "Anti Aliasing", "Jitter Mode", "", JitterModes::Hammersley4x, 5, JitterModesLabels);
        Settings.AddSetting(&JitterMode);

        JitterScale.Initialize(tweakBar, "JitterScale", "Anti Aliasing", "Jitter Scale", "", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&JitterScale);

        SGDiffuseMode.Initialize(tweakBar, "SGDiffuseMode", "SG Settings", "SG Diffuse Mode", "", SGDiffuseModes::Fitted, 3, SGDiffuseModesLabels);
        Settings.AddSetting(&SGDiffuseMode);

        UseASGWarp.Initialize(tweakBar, "UseASGWarp", "SG Settings", "Use ASG Warp", "", true);
        Settings.AddSetting(&UseASGWarp);

        LightMapResolution.Initialize(tweakBar, "LightMapResolution", "Baking", "Light Map Resolution", "The texture resolution of the light map", 256, 64, 4096);
        Settings.AddSetting(&LightMapResolution);

        NumBakeSamples.Initialize(tweakBar, "NumBakeSamples", "Baking", "Sqrt Num Samples", "The square root of the number of sample rays to use for baking GI", 25, 1, 100);
        Settings.AddSetting(&NumBakeSamples);

        BakeSampleMode.Initialize(tweakBar, "BakeSampleMode", "Baking", "Sample Mode", "", SampleModes::CMJ, 5, SampleModesLabels);
        Settings.AddSetting(&BakeSampleMode);

        MaxBakePathLength.Initialize(tweakBar, "MaxBakePathLength", "Baking", "Max Bake Path Length", "Maximum path length (bounces + 2) to use for baking GI (set to -1 for infinite)", -1, -1, 2147483647);
        Settings.AddSetting(&MaxBakePathLength);

        BakeRussianRouletteDepth.Initialize(tweakBar, "BakeRussianRouletteDepth", "Baking", "Russian Roullette Depth", "Path length at which Russian roulette kicks in (-1 to disable)", 4, -1, 2147483647);
        Settings.AddSetting(&BakeRussianRouletteDepth);

        BakeRussianRouletteProbability.Initialize(tweakBar, "BakeRussianRouletteProbability", "Baking", "Russian Roullette Probability", "Maximum probability for continuing when Russian roulette is used", 0.5000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BakeRussianRouletteProbability);

        BakeMode.Initialize(tweakBar, "BakeMode", "Baking", "Bake Mode", "", BakeModes::H4, 10, BakeModesLabels);
        Settings.AddSetting(&BakeMode);

        SolveMode.Initialize(tweakBar, "SolveMode", "Baking", "Solve Mode", "", SolveModes::NNLS, 3, SolveModesLabels);
        Settings.AddSetting(&SolveMode);

        UseProbes.Initialize(tweakBar, "UseProbes", "Probes", "Use Probes", "", false);
        Settings.AddSetting(&UseProbes);

        ProbeMode.Initialize(tweakBar, "ProbeMode", "Probes", "Probe Mode", "", ProbeModes::CubeMap, 4, ProbeModesLabels);
        Settings.AddSetting(&ProbeMode);

        ProbeResX.Initialize(tweakBar, "ProbeResX", "Probes", "Probe Res X", "", 4, 1, 2048);
        Settings.AddSetting(&ProbeResX);

        ProbeResY.Initialize(tweakBar, "ProbeResY", "Probes", "Probe Res Y", "", 4, 1, 2048);
        Settings.AddSetting(&ProbeResY);

        ProbeResZ.Initialize(tweakBar, "ProbeResZ", "Probes", "Probe Res Z", "", 4, 1, 2048);
        Settings.AddSetting(&ProbeResZ);

        ProbeCubemapCaptureRes.Initialize(tweakBar, "ProbeCubemapCaptureRes", "Probes", "Probe Cubemap Capture Res", "", 256, 1, 4096);
        Settings.AddSetting(&ProbeCubemapCaptureRes);

        ProbeIrradianceCubemapRes.Initialize(tweakBar, "ProbeIrradianceCubemapRes", "Probes", "Probe Irradiance Cubemap Res", "", 16, 1, 4096);
        Settings.AddSetting(&ProbeIrradianceCubemapRes);

        ProbeDistanceCubemapRes.Initialize(tweakBar, "ProbeDistanceCubemapRes", "Probes", "Probe Distance Cubemap Res", "", 128, 1, 4096);
        Settings.AddSetting(&ProbeDistanceCubemapRes);

        SceneBoundsScale.Initialize(tweakBar, "SceneBoundsScale", "Probes", "Scene Bounds Scale", "", 1.2500f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SceneBoundsScale);

        WeightProbesByNormal.Initialize(tweakBar, "WeightProbesByNormal", "Probes", "Weight Probes By Normal", "", false);
        Settings.AddSetting(&WeightProbesByNormal);

        WeightProbesByVisibility.Initialize(tweakBar, "WeightProbesByVisibility", "Probes", "Weight Probes By Visibility", "", false);
        Settings.AddSetting(&WeightProbesByVisibility);

        DistanceFilterSharpness.Initialize(tweakBar, "DistanceFilterSharpness", "Probes", "Distance Filter Sharpness", "", 10.0000f, 1.0000f, 20.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&DistanceFilterSharpness);

        ProbeIntegrationSamples.Initialize(tweakBar, "ProbeIntegrationSamples", "Probes", "Probe Integration Samples", "", 32, 1, 64);
        Settings.AddSetting(&ProbeIntegrationSamples);

        ProbeDistanceIntegrationSamples.Initialize(tweakBar, "ProbeDistanceIntegrationSamples", "Probes", "Probe Distance Integration Samples", "", 32, 1, 64);
        Settings.AddSetting(&ProbeDistanceIntegrationSamples);

        BakeWithVCT.Initialize(tweakBar, "BakeWithVCT", "VCT", "Bake With VCT", "", false);
        Settings.AddSetting(&BakeWithVCT);

        VoxelResX.Initialize(tweakBar, "VoxelResX", "VCT", "Voxel Resolution (X)", "", 32, 1, 2048);
        Settings.AddSetting(&VoxelResX);

        VoxelResY.Initialize(tweakBar, "VoxelResY", "VCT", "Voxel Resolution (Y)", "", 32, 1, 2048);
        Settings.AddSetting(&VoxelResY);

        VoxelResZ.Initialize(tweakBar, "VoxelResZ", "VCT", "Voxel Resolution (Z)", "", 32, 1, 2048);
        Settings.AddSetting(&VoxelResZ);

        CurrentScene.Initialize(tweakBar, "CurrentScene", "Scene", "Current Scene", "", Scenes::Box, 3, ScenesLabels);
        Settings.AddSetting(&CurrentScene);

        EnableDiffuse.Initialize(tweakBar, "EnableDiffuse", "Scene", "Enable Diffuse", "Enables diffuse lighting", true);
        Settings.AddSetting(&EnableDiffuse);

        EnableSpecular.Initialize(tweakBar, "EnableSpecular", "Scene", "Enable Specular", "Enables specular lighting", true);
        Settings.AddSetting(&EnableSpecular);

        EnableDirectLighting.Initialize(tweakBar, "EnableDirectLighting", "Scene", "Enable Direct Lighting", "Enables direct lighting", true);
        Settings.AddSetting(&EnableDirectLighting);

        EnableIndirectLighting.Initialize(tweakBar, "EnableIndirectLighting", "Scene", "Enable Indirect Lighting", "Enables indirect lighting", true);
        Settings.AddSetting(&EnableIndirectLighting);

        EnableIndirectDiffuse.Initialize(tweakBar, "EnableIndirectDiffuse", "Scene", "Enable Indirect Diffuse", "Enables indirect diffuse lighting", true);
        Settings.AddSetting(&EnableIndirectDiffuse);

        EnableIndirectSpecular.Initialize(tweakBar, "EnableIndirectSpecular", "Scene", "Enable Indirect Specular", "Enables indirect specular lighting", false);
        Settings.AddSetting(&EnableIndirectSpecular);

        EnableAlbedoMaps.Initialize(tweakBar, "EnableAlbedoMaps", "Scene", "Enable Albedo Maps", "Enables albedo maps", true);
        Settings.AddSetting(&EnableAlbedoMaps);

        EnableNormalMaps.Initialize(tweakBar, "EnableNormalMaps", "Scene", "Enable Normal Maps", "Enables normal maps", true);
        Settings.AddSetting(&EnableNormalMaps);

        NormalMapIntensity.Initialize(tweakBar, "NormalMapIntensity", "Scene", "Normal Map Intensity", "Intensity of the normal map", 0.5000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&NormalMapIntensity);

        DiffuseAlbedoScale.Initialize(tweakBar, "DiffuseAlbedoScale", "Scene", "Diffuse Albedo Scale", "Global scale applied to all material diffuse albedo values", 0.5000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&DiffuseAlbedoScale);

        RoughnessScale.Initialize(tweakBar, "RoughnessScale", "Scene", "Specular Roughness Scale", "Global scale applied to all material roughness values", 2.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&RoughnessScale);

        ShowGroundTruth.Initialize(tweakBar, "ShowGroundTruth", "Ground Truth", "Show Ground Truth", "If enabled, shows a ground truth image rendered on the CPU", false);
        Settings.AddSetting(&ShowGroundTruth);

        NumRenderSamples.Initialize(tweakBar, "NumRenderSamples", "Ground Truth", "Sqrt Num Samples", "The square root of the number of per-pixel sample rays to use for ground truth rendering", 4, 1, 100);
        Settings.AddSetting(&NumRenderSamples);

        RenderSampleMode.Initialize(tweakBar, "RenderSampleMode", "Ground Truth", "Sample Mode", "", SampleModes::CMJ, 5, SampleModesLabels);
        Settings.AddSetting(&RenderSampleMode);

        MaxRenderPathLength.Initialize(tweakBar, "MaxRenderPathLength", "Ground Truth", "Max Path Length", "Maximum path length (bounces) to use for ground truth rendering (set to -1 for infinite)", -1, -1, 2147483647);
        Settings.AddSetting(&MaxRenderPathLength);

        RenderRussianRouletteDepth.Initialize(tweakBar, "RenderRussianRouletteDepth", "Ground Truth", "Russian Roullette Depth", "Path length at which Russian roulette kicks in (-1 to disable)", 4, -1, 2147483647);
        Settings.AddSetting(&RenderRussianRouletteDepth);

        RenderRussianRouletteProbability.Initialize(tweakBar, "RenderRussianRouletteProbability", "Ground Truth", "Russian Roullette Probability", "Maximum probability for continuing when Russian roulette is used", 0.5000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&RenderRussianRouletteProbability);

        EnableRenderBounceSpecular.Initialize(tweakBar, "EnableRenderBounceSpecular", "Ground Truth", "Enable Bounce Specular", "Enables specular calculations after the first hit", false);
        Settings.AddSetting(&EnableRenderBounceSpecular);

        BloomExposure.Initialize(tweakBar, "BloomExposure", "Post Processing", "Bloom Exposure Offset", "Exposure offset applied to generate the input of the bloom pass", -4.0000f, -10.0000f, 0.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomExposure);

        BloomMagnitude.Initialize(tweakBar, "BloomMagnitude", "Post Processing", "Bloom Magnitude", "Scale factor applied to the bloom results when combined with tone-mapped result", 1.0000f, 0.0000f, 2.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomMagnitude);

        BloomBlurSigma.Initialize(tweakBar, "BloomBlurSigma", "Post Processing", "Bloom Blur Sigma", "Sigma parameter of the Gaussian filter used in the bloom pass", 2.5000f, 0.5000f, 2.5000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomBlurSigma);

        EnableLuminancePicker.Initialize(tweakBar, "EnableLuminancePicker", "Debug", "Enable Luminance Picker", "", false);
        Settings.AddSetting(&EnableLuminancePicker);

        ShowBakeDataVisualizer.Initialize(tweakBar, "ShowBakeDataVisualizer", "Debug", "Show Bake Data Visualizer", "", false);
        Settings.AddSetting(&ShowBakeDataVisualizer);

        ShowProbeVisualizer.Initialize(tweakBar, "ShowProbeVisualizer", "Debug", "Show Probe Visualizer", "", false);
        Settings.AddSetting(&ShowProbeVisualizer);

        ViewIndirectSpecular.Initialize(tweakBar, "ViewIndirectSpecular", "Debug", "View Indirect Specular", "", false);
        Settings.AddSetting(&ViewIndirectSpecular);

        VoxelVisualizerMode.Initialize(tweakBar, "VoxelVisualizerMode", "Debug", "Voxel Visualizer Mode", "", VoxelVisualizerModes::None, 3, VoxelVisualizerModesLabels);
        Settings.AddSetting(&VoxelVisualizerMode);

        SaveLightSettings.Initialize(tweakBar, "SaveLightSettings", "Debug", "Save Light Settings", "Saves the lighting settings to a file");
        Settings.AddSetting(&SaveLightSettings);

        LoadLightSettings.Initialize(tweakBar, "LoadLightSettings", "Debug", "Load Light Settings", "Loads the lighting settings from a file");
        Settings.AddSetting(&LoadLightSettings);

        SaveEXRScreenshot.Initialize(tweakBar, "SaveEXRScreenshot", "Debug", "Save EXR Screenshot", "Captures the current screen image in EXR format");
        Settings.AddSetting(&SaveEXRScreenshot);

        ShowSunIntensity.Initialize(tweakBar, "ShowSunIntensity", "Debug", "Show Sun Intensity", "", false);
        Settings.AddSetting(&ShowSunIntensity);

        AlwaysRegenerateProbes.Initialize(tweakBar, "AlwaysRegenerateProbes", "Debug", "Always Regenerate Probes", "", false);
        Settings.AddSetting(&AlwaysRegenerateProbes);

        AlwaysRevoxelize.Initialize(tweakBar, "AlwaysRevoxelize", "Debug", "Always Revoxelize", "", false);
        Settings.AddSetting(&AlwaysRevoxelize);

        SceneBoundsOffsetX.Initialize(tweakBar, "SceneBoundsOffsetX", "Debug", "Scene Bounds Offset X", "", 0.0000f, -100.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SceneBoundsOffsetX);

        SceneBoundsOffsetY.Initialize(tweakBar, "SceneBoundsOffsetY", "Debug", "Scene Bounds Offset Y", "", 0.0000f, -100.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SceneBoundsOffsetY);

        SceneBoundsOffsetZ.Initialize(tweakBar, "SceneBoundsOffsetZ", "Debug", "Scene Bounds Offset Z", "", 0.0000f, -100.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SceneBoundsOffsetZ);

        TwHelper::SetOpened(tweakBar, "Sun Light", true);

        TwHelper::SetOpened(tweakBar, "Sky", true);

        TwHelper::SetOpened(tweakBar, "Area Light", false);

        TwHelper::SetOpened(tweakBar, "Camera Controls", false);

        TwHelper::SetOpened(tweakBar, "Tone Mapping", false);

        TwHelper::SetOpened(tweakBar, "Anti Aliasing", false);

        TwHelper::SetOpened(tweakBar, "SG Settings", false);

        TwHelper::SetOpened(tweakBar, "Baking", false);

        TwHelper::SetOpened(tweakBar, "Probes", false);

        TwHelper::SetOpened(tweakBar, "VCT", true);

        TwHelper::SetOpened(tweakBar, "Scene", false);

        TwHelper::SetOpened(tweakBar, "Ground Truth", false);

        TwHelper::SetOpened(tweakBar, "Post Processing", false);

        TwHelper::SetOpened(tweakBar, "Debug", true);

        CBuffer.Initialize(device);
    }

    void Update()
    {
        ApertureFNumber.SetValue(ApertureFNumber_());
        ApertureWidth.SetValue(ApertureWidth_());
        ShutterSpeedValue.SetValue(ShutterSpeedValue_());
        ISO.SetValue(ISO_());
        BokehPolygonAmount.SetValue(BokehPolygonAmount_());
    }

    void UpdateCBuffer(ID3D11DeviceContext* context)
    {
        CBuffer.Data.EnableSun = EnableSun;
        CBuffer.Data.SunAreaLightApproximation = SunAreaLightApproximation;
        CBuffer.Data.SunTintColor = SunTintColor;
        CBuffer.Data.SunIntensityScale = SunIntensityScale;
        CBuffer.Data.SunSize = SunSize;
        CBuffer.Data.SunDirType = SunDirType;
        CBuffer.Data.SunDirection = SunDirection;
        CBuffer.Data.SunAzimuth = SunAzimuth;
        CBuffer.Data.SunElevation = SunElevation;
        CBuffer.Data.EnableAreaLight = EnableAreaLight;
        CBuffer.Data.AreaLightColor = AreaLightColor;
        CBuffer.Data.AreaLightSize = AreaLightSize;
        CBuffer.Data.AreaLightX = AreaLightX;
        CBuffer.Data.AreaLightY = AreaLightY;
        CBuffer.Data.AreaLightZ = AreaLightZ;
        CBuffer.Data.AreaLightShadowBias = AreaLightShadowBias;
        CBuffer.Data.BakeDirectAreaLight = BakeDirectAreaLight;
        CBuffer.Data.EnableAreaLightShadows = EnableAreaLightShadows;
        CBuffer.Data.ExposureMode = ExposureMode;
        CBuffer.Data.ManualExposure = ManualExposure;
        CBuffer.Data.FilmSize = FilmSize;
        CBuffer.Data.FocalLength = FocalLength;
        CBuffer.Data.FocusDistance = FocusDistance;
        CBuffer.Data.NumBlades = NumBlades;
        CBuffer.Data.KeyValue = KeyValue;
        CBuffer.Data.AdaptationRate = AdaptationRate;
        CBuffer.Data.ApertureFNumber = ApertureFNumber;
        CBuffer.Data.ApertureWidth = ApertureWidth;
        CBuffer.Data.ShutterSpeedValue = ShutterSpeedValue;
        CBuffer.Data.ISO = ISO;
        CBuffer.Data.BokehPolygonAmount = BokehPolygonAmount;
        CBuffer.Data.ToneMappingMode = ToneMappingMode;
        CBuffer.Data.WhitePoint_Hejl = WhitePoint_Hejl;
        CBuffer.Data.ShoulderStrength = ShoulderStrength;
        CBuffer.Data.LinearStrength = LinearStrength;
        CBuffer.Data.LinearAngle = LinearAngle;
        CBuffer.Data.ToeStrength = ToeStrength;
        CBuffer.Data.WhitePoint_Hable = WhitePoint_Hable;
        CBuffer.Data.MSAAMode = MSAAMode;
        CBuffer.Data.FilterType = FilterType;
        CBuffer.Data.FilterSize = FilterSize;
        CBuffer.Data.GaussianSigma = GaussianSigma;
        CBuffer.Data.SGDiffuseMode = SGDiffuseMode;
        CBuffer.Data.UseASGWarp = UseASGWarp;
        CBuffer.Data.LightMapResolution = LightMapResolution;
        CBuffer.Data.BakeMode = BakeMode;
        CBuffer.Data.SolveMode = SolveMode;
        CBuffer.Data.UseProbes = UseProbes;
        CBuffer.Data.ProbeMode = ProbeMode;
        CBuffer.Data.ProbeResX = ProbeResX;
        CBuffer.Data.ProbeResY = ProbeResY;
        CBuffer.Data.ProbeResZ = ProbeResZ;
        CBuffer.Data.ProbeCubemapCaptureRes = ProbeCubemapCaptureRes;
        CBuffer.Data.ProbeIrradianceCubemapRes = ProbeIrradianceCubemapRes;
        CBuffer.Data.ProbeDistanceCubemapRes = ProbeDistanceCubemapRes;
        CBuffer.Data.WeightProbesByNormal = WeightProbesByNormal;
        CBuffer.Data.WeightProbesByVisibility = WeightProbesByVisibility;
        CBuffer.Data.DistanceFilterSharpness = DistanceFilterSharpness;
        CBuffer.Data.ProbeIntegrationSamples = ProbeIntegrationSamples;
        CBuffer.Data.ProbeDistanceIntegrationSamples = ProbeDistanceIntegrationSamples;
        CBuffer.Data.BakeWithVCT = BakeWithVCT;
        CBuffer.Data.VoxelResX = VoxelResX;
        CBuffer.Data.VoxelResY = VoxelResY;
        CBuffer.Data.VoxelResZ = VoxelResZ;
        CBuffer.Data.EnableDiffuse = EnableDiffuse;
        CBuffer.Data.EnableSpecular = EnableSpecular;
        CBuffer.Data.EnableDirectLighting = EnableDirectLighting;
        CBuffer.Data.EnableIndirectLighting = EnableIndirectLighting;
        CBuffer.Data.EnableIndirectDiffuse = EnableIndirectDiffuse;
        CBuffer.Data.EnableIndirectSpecular = EnableIndirectSpecular;
        CBuffer.Data.EnableAlbedoMaps = EnableAlbedoMaps;
        CBuffer.Data.EnableNormalMaps = EnableNormalMaps;
        CBuffer.Data.NormalMapIntensity = NormalMapIntensity;
        CBuffer.Data.DiffuseAlbedoScale = DiffuseAlbedoScale;
        CBuffer.Data.RoughnessScale = RoughnessScale;
        CBuffer.Data.BloomExposure = BloomExposure;
        CBuffer.Data.BloomMagnitude = BloomMagnitude;
        CBuffer.Data.BloomBlurSigma = BloomBlurSigma;
        CBuffer.Data.ViewIndirectSpecular = ViewIndirectSpecular;

        CBuffer.ApplyChanges(context);
        CBuffer.SetVS(context, 7);
        CBuffer.SetHS(context, 7);
        CBuffer.SetDS(context, 7);
        CBuffer.SetGS(context, 7);
        CBuffer.SetPS(context, 7);
        CBuffer.SetCS(context, 7);
    }
}

// ================================================================================================

#include <Graphics\\Spectrum.h>
#include <Graphics\\Sampling.h>
#include <SF11_Math.h>

namespace AppSettings
{
    // Returns the result of performing a irradiance/illuminance integral over the portion
    // of the hemisphere covered by a region with angular radius = theta
    static float IlluminanceIntegral(float theta)
    {
        float cosTheta = std::cos(theta);
        return Pi * (1.0f - (cosTheta * cosTheta));
    }

    Float3 SunLuminance(bool& cached)
    {
        Float3 sunDirection = AppSettings::SunDirection;
        sunDirection.y = Saturate(sunDirection.y);
        sunDirection = Float3::Normalize(sunDirection);
        const float turbidity = Clamp(AppSettings::Turbidity.Value(), 1.0f, 32.0f);
        const float intensityScale = AppSettings::SunIntensityScale;
        const Float3 tintColor = AppSettings::SunTintColor;
        const bool32 normalizeIntensity = AppSettings::NormalizeSunIntensity;
        const float sunSize = AppSettings::SunSize;

        static float turbidityCache = 2.0f;
        static Float3 sunDirectionCache = Float3(-0.579149902f, 0.754439294f, -0.308879942f);
        static Float3 luminanceCache = Float3(1.61212531e+009f, 1.36822630e+009f, 1.07235315e+009f) * FP16Scale;
        static Float3 sunTintCache = Float3(1.0f, 1.0f, 1.0f);
        static float sunIntensityCache = 1.0f;
        static bool32 normalizeCache = false;
        static float sunSizeCache = AppSettings::BaseSunSize;

        if(turbidityCache == turbidity && sunDirection == sunDirectionCache
            && intensityScale == sunIntensityCache && tintColor == sunTintCache
            && normalizeCache == normalizeIntensity && sunSize == sunSizeCache)
        {
            cached = true;
            return luminanceCache;
        }

        cached = false;

        float thetaS = std::acos(1.0f - sunDirection.y);
        float elevation = Pi_2 - thetaS;

        // Get the sun's luminance, then apply tint and scale factors
        Float3 sunLuminance;

        // For now, we'll compute an average luminance value from Hosek solar radiance model, even though
        // we could compute illuminance directly while we're sampling the disk
        SampledSpectrum groundAlbedoSpectrum = SampledSpectrum::FromRGB(GroundAlbedo);
        SampledSpectrum solarRadiance;

        const uint64 NumDiscSamples = 8;
        for(uint64 x = 0; x < NumDiscSamples; ++x)
        {
            for(uint64 y = 0; y < NumDiscSamples; ++y)
            {
                float u = (x + 0.5f) / NumDiscSamples;
                float v = (y + 0.5f) / NumDiscSamples;
                Float2 discSamplePos = SquareToConcentricDiskMapping(u, v);

                float theta = elevation + discSamplePos.y * DegToRad(AppSettings::BaseSunSize);
                float gamma = discSamplePos.x * DegToRad(AppSettings::BaseSunSize);

                for(int32 i = 0; i < NumSpectralSamples; ++i)
                {
                    ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(elevation, turbidity, groundAlbedoSpectrum[i]);
                    float wavelength = Lerp(float(SampledLambdaStart), float(SampledLambdaEnd), i / float(NumSpectralSamples));

                    solarRadiance[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));

                    arhosekskymodelstate_free(skyState);
                    skyState = nullptr;
                }

                Float3 sampleRadiance = solarRadiance.ToRGB() * FP16Scale;
                sunLuminance += sampleRadiance;
            }
        }

        // Account for luminous efficiency, coordinate system scaling, and sample averaging
        sunLuminance *= 683.0f * 100.0f * (1.0f / NumDiscSamples) * (1.0f / NumDiscSamples);

        sunLuminance = sunLuminance * tintColor;
        sunLuminance = sunLuminance * intensityScale;

        if(normalizeIntensity)
        {
            // Normalize so that the intensity stays the same even when the sun is bigger or smaller
            const float baseIntegral = IlluminanceIntegral(DegToRad(AppSettings::BaseSunSize));
            const float currIntegral = IlluminanceIntegral(DegToRad(AppSettings::SunSize));
            sunLuminance *= (baseIntegral / currIntegral);
        }

        turbidityCache = turbidity;
        sunDirectionCache = sunDirection;
        luminanceCache = sunLuminance;
        sunIntensityCache = intensityScale;
        sunTintCache = tintColor;
        normalizeCache = normalizeIntensity;
        sunSizeCache = sunSize;

        return sunLuminance;
    }

    Float3 SunLuminance()
    {
        bool cached = false;
        return SunLuminance(cached);
    }

    Float3 SunIlluminance()
    {
        Float3 sunLuminance = SunLuminance();

        // Compute partial integral over the hemisphere in order to compute illuminance
        float theta = DegToRad(AppSettings::SunSize);
        float integralFactor = IlluminanceIntegral(theta);

        return sunLuminance * integralFactor;
    }

    static void UpdateAreaLightUnits()
    {
        static LightUnits prevUnits = AreaLightUnits;
        const LightUnits currUnits = AreaLightUnits;

        const float size = AreaLightSize;
        const float distance = AreaLightIlluminanceDistance;

        // Compute the luminance from our  currently active unit
        float luminance = AreaLightColor.Intensity();
        if(prevUnits == LightUnits::Illuminance)
        {
            float illuminance = AreaLightIlluminance;
            float angularRadius = std::atan2(size, distance);
            float integralFactor = IlluminanceIntegral(angularRadius);
            luminance = illuminance / integralFactor;
        }
        else if(prevUnits == LightUnits::LuminousPower)
        {
            float luminousPower = AreaLightLuminousPower;
            luminance = luminousPower / (4 * size * size * Pi * Pi);
        }
        else if(prevUnits == LightUnits::EV100)
        {
            float ev100 = AreaLightEV100;
            luminance = std::pow(2.0f, ev100 - 3.0f);
        }

        // Set the new luminance onto the color
        AreaLightColor.SetIntensity(luminance);

        // Update the other units
        float angularRadius = std::atan2(size, distance);
        float integralFactor = IlluminanceIntegral(angularRadius);
        float illuminance = luminance * integralFactor;
        AreaLightIlluminance.SetValue(illuminance);

        float luminousPower = luminance * (4 * size * size * Pi * Pi);
        AreaLightLuminousPower.SetValue(luminousPower);

        float ev100 = std::log2(luminance) + 3.0f;
        AreaLightEV100.SetValue(ev100);

        if(prevUnits != currUnits)
        {
            AreaLightColor.SetIntensityVisible(currUnits == LightUnits::Luminance);
            AreaLightIlluminance.SetVisible(currUnits == LightUnits::Illuminance);
            AreaLightLuminousPower.SetVisible(currUnits == LightUnits::LuminousPower);
            AreaLightEV100.SetVisible(currUnits == LightUnits::EV100);
            AreaLightIlluminanceDistance.SetVisible(currUnits == LightUnits::Illuminance);

            prevUnits = currUnits;
        }
    }

    void UpdateUI()
    {
        bool enableManualExposure = ExposureMode == ExposureModes::ManualSimple;
        bool enableCameraExposure = ExposureMode == ExposureModes::Manual_SBS ||
                                    ExposureMode == ExposureModes::Manual_SOS;
        bool enableAutoExposure = ExposureMode == ExposureModes::Automatic;
        bool enableSimpleSkyParams = SkyMode == SkyModes::Simple;
        bool enableProceduralSkyParams = SkyMode == SkyModes::Procedural;
        bool useHejlTM = ToneMappingMode == ToneMappingModes::Hejl2015;
        bool useHableTM = ToneMappingMode == ToneMappingModes::Hable;
        bool useSGSettings = SGCount() > 0;

        ManualExposure.SetEditable(enableManualExposure);
        ShutterSpeed.SetEditable(enableCameraExposure);
        ISORating.SetEditable(enableCameraExposure);
        KeyValue.SetEditable(enableAutoExposure);
        AdaptationRate.SetEditable(enableAutoExposure);
        SkyColor.SetEditable(enableSimpleSkyParams);
        Turbidity.SetEditable(enableProceduralSkyParams);
        GroundAlbedo.SetEditable(enableProceduralSkyParams);

        WhitePoint_Hejl.SetVisible(useHejlTM);

        WhitePoint_Hable.SetVisible(useHableTM);
        ShoulderStrength.SetVisible(useHableTM);
        LinearStrength.SetVisible(useHableTM);
        LinearAngle.SetVisible(useHableTM);
        ToeStrength.SetVisible(useHableTM);

        bool enableSunDirection = SunDirType == SunDirectionTypes::UnitVector;
        bool enableHorizCoord = SunDirType == SunDirectionTypes::HorizontalCoordSystem;

        SunDirection.SetVisible(enableSunDirection);
        SunAzimuth.SetVisible(enableHorizCoord);
        SunElevation.SetVisible(enableHorizCoord);

        SGDiffuseMode.SetEditable(useSGSettings);
        SolveMode.SetEditable(useSGSettings);

        if(AppSettings::HasSunDirChanged())
        {
            if(SunDirType == SunDirectionTypes::UnitVector)
            {
                UpdateHorizontalCoords();
            }
            else if(SunDirType == SunDirectionTypes::HorizontalCoordSystem)
            {
                UpdateUnitVector();
            }
        }

        UpdateAreaLightUnits();
    }
}