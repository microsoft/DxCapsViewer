//-----------------------------------------------------------------------------
// Name: dxgi.cpp
//
// Desc: DirectX Capabilities Viewer for DXGI (Direct3D 10.x / 11.x / 12.0)
//
// Copyright Microsoft Corporation. All Rights Reserved.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?linkid=2136896
//-----------------------------------------------------------------------------
#include "dxview.h"

#include <D3Dcommon.h>
#include <dxgi1_5.h>
#include <d3d10_1.h>
#include <d3d11_4.h>
#include <d3d12.h>

// Define for some debug output
//#define EXTRA_DEBUG

//-----------------------------------------------------------------------------

enum FLMASK
{
    FLMASK_9_1 = 0x1,
    FLMASK_9_2 = 0x2,
    FLMASK_9_3 = 0x4,
    FLMASK_10_0 = 0x8,
    FLMASK_10_1 = 0x10,
    FLMASK_11_0 = 0x20,
    FLMASK_11_1 = 0x40,
    FLMASK_12_0 = 0x80,
    FLMASK_12_1 = 0x100,
    FLMASK_12_2 = 0x200,
};

#if !defined(NTDDI_WIN10_FE)
#define D3D_FEATURE_LEVEL_12_2 static_cast<D3D_FEATURE_LEVEL>(0xc200)
#define D3D_SHADER_MODEL_6_7 static_cast<D3D_SHADER_MODEL>(0x67)
#pragma warning(disable : 4063 4702)
#endif

//-----------------------------------------------------------------------------
namespace
{
    const char* D3D10_NOTE = "Most Direct3D 10 features are required. Tool only shows optional features.";
    const char* D3D10_NOTE1 = "Most Direct3D 10.1 features are required. Tool only shows optional features.";
    const char* D3D11_NOTE = "Most Direct3D 11 features are required. Tool only shows optional features.";
    const char* D3D11_NOTE1 = "Most Direct3D 11.1 features are required. Tool only shows optional features.";
    const char* D3D11_NOTE2 = "Most Direct3D 11.2 features are required. Tool only shows optional features.";
    const char* D3D11_NOTE3 = "Most Direct3D 11.3 features are required. Tool only shows optional features.";
    const char* D3D11_NOTE4 = "Most Direct3D 11.x features are required. Tool only shows optional features.";
    const char* _10L9_NOTE = "Most 10level9 features are required. Tool only shows optional features.";
    const char* SEE_D3D10 = "See Direct3D 10 node for device details.";
    const char* SEE_D3D10_1 = "See Direct3D 10.1 node for device details.";
    const char* SEE_D3D11 = "See Direct3D 11 node for device details.";
    const char* SEE_D3D11_1 = "See Direct3D 11.1 node for device details.";

    const char* FL_NOTE = "This feature summary is derived from hardware feature level";

    static_assert(sizeof(D3D10_NOTE) < 80, "String too long");
    static_assert(sizeof(D3D10_NOTE1) < 80, "String too long");
    static_assert(sizeof(D3D11_NOTE) < 80, "String too long");
    static_assert(sizeof(D3D11_NOTE1) < 80, "String too long");
    static_assert(sizeof(D3D11_NOTE2) < 80, "String too long");
    static_assert(sizeof(D3D11_NOTE3) < 80, "String too long");
    static_assert(sizeof(D3D11_NOTE4) < 80, "String too long");
    static_assert(sizeof(_10L9_NOTE) < 80, "String too long");
    static_assert(sizeof(SEE_D3D10) < 80, "String too long");
    static_assert(sizeof(SEE_D3D10_1) < 80, "String too long");
    static_assert(sizeof(SEE_D3D11) < 80, "String too long");
    static_assert(sizeof(SEE_D3D11_1) < 80, "String too long");

    //-----------------------------------------------------------------------------

    using LPCREATEDXGIFACTORY = HRESULT(WINAPI*)(REFIID, void**);
    using LPD3D10CREATEDEVICE = HRESULT(WINAPI*)(IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT32, ID3D10Device**);

    IDXGIFactory* g_DXGIFactory = nullptr;
    IDXGIFactory1* g_DXGIFactory1 = nullptr;
    IDXGIFactory2* g_DXGIFactory2 = nullptr;
    IDXGIFactory3* g_DXGIFactory3 = nullptr;
    IDXGIFactory4* g_DXGIFactory4 = nullptr;
    IDXGIFactory5* g_DXGIFactory5 = nullptr;
    HMODULE g_dxgi = nullptr;

    LPD3D10CREATEDEVICE g_D3D10CreateDevice = nullptr;
    HMODULE g_d3d10 = nullptr;

    PFN_D3D10_CREATE_DEVICE1 g_D3D10CreateDevice1 = nullptr;
    HMODULE g_d3d10_1 = nullptr;

    PFN_D3D11_CREATE_DEVICE g_D3D11CreateDevice = nullptr;
    HMODULE g_d3d11 = nullptr;

    PFN_D3D12_CREATE_DEVICE g_D3D12CreateDevice = nullptr;
    HMODULE g_d3d12 = nullptr;
}

extern DWORD g_dwViewState;
extern const char c_szYes[];
extern const char c_szNo[];
extern const char c_szNA[];

const char c_szOptYes[] = "Optional (Yes)";
const char c_szOptNo[] = "Optional (No)";

namespace
{
    const char* szRotation[] =
    {
        "DXGI_MODE_ROTATION_UNSPECIFIED",
        "DXGI_MODE_ROTATION_IDENTITY",
        "DXGI_MODE_ROTATION_ROTATE90",
        "DXGI_MODE_ROTATION_ROTATE180",
        "DXGI_MODE_ROTATION_ROTATE270"
    };

    // The DXGI formats that can be used as display devices
    const DXGI_FORMAT AdapterFormatArray[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, // DXGI 1.1
        DXGI_FORMAT_B8G8R8A8_UNORM,             // DXGI 1.1
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        // DXGI 1.1
    };
    const UINT NumAdapterFormats = sizeof(AdapterFormatArray) / sizeof(AdapterFormatArray[0]);

    const DXGI_FORMAT g_cfsMSAA_10level9[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM
    };

    const DXGI_FORMAT g_cfsMSAA_10[] =
    {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32_SINT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R10G10B10A2_UINT,
        DXGI_FORMAT_R11G11B10_FLOAT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_R8G8_SINT,
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_R16_UNORM,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_SINT,
        DXGI_FORMAT_A8_UNORM
    };

    const DXGI_FORMAT g_cfsMSAA_11[] =
    {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32_SINT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R10G10B10A2_UINT,
        DXGI_FORMAT_R11G11B10_FLOAT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_R8G8_SINT,
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_R16_UNORM,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_SINT,
        DXGI_FORMAT_A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8X8_UNORM,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
        DXGI_FORMAT_B5G6R5_UNORM,
        DXGI_FORMAT_B5G5R5A1_UNORM,
        DXGI_FORMAT_B4G4R4A4_UNORM,
    };

    BOOL g_sampCount10[D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT];
    BOOL g_sampCount10_1[D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT];
    BOOL g_sampCount11[D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT];
    BOOL g_sampCount11_1[D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT];

    const D3D_FEATURE_LEVEL g_featureLevels[] =
    {
          D3D_FEATURE_LEVEL_12_2,
          D3D_FEATURE_LEVEL_12_1,
          D3D_FEATURE_LEVEL_12_0,
          D3D_FEATURE_LEVEL_11_1,
          D3D_FEATURE_LEVEL_11_0,
          D3D_FEATURE_LEVEL_10_1,
          D3D_FEATURE_LEVEL_10_0,
          D3D_FEATURE_LEVEL_9_3,
          D3D_FEATURE_LEVEL_9_2,
          D3D_FEATURE_LEVEL_9_1
    };

    D3D_FEATURE_LEVEL GetD3D12FeatureLevel(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_FEATURE_LEVELS flData = {};
        flData.NumFeatureLevels = 5; // D3D12 requires FL 11.0 or greater
        flData.pFeatureLevelsRequested = g_featureLevels;

        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &flData, sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS))))
            return flData.MaxSupportedFeatureLevel;

        return static_cast<D3D_FEATURE_LEVEL>(0);
    }

    D3D_SHADER_MODEL GetD3D12ShaderModel(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModelOpt = {};
        shaderModelOpt.HighestShaderModel = D3D_SHADER_MODEL_6_7;
        HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelOpt, sizeof(shaderModelOpt));
        while (hr == E_INVALIDARG && shaderModelOpt.HighestShaderModel > D3D_SHADER_MODEL_6_0)
        {
            shaderModelOpt.HighestShaderModel = static_cast<D3D_SHADER_MODEL>(static_cast<int>(shaderModelOpt.HighestShaderModel) - 1);
            hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelOpt, sizeof(shaderModelOpt));
        }
        if (FAILED(hr))
        {
            shaderModelOpt.HighestShaderModel = D3D_SHADER_MODEL_5_1;
        }

        return shaderModelOpt.HighestShaderModel;
    }

    const char* D3D12DXRSupported(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12opts = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5))))
        {
            switch (d3d12opts.RaytracingTier)
            {
            case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: break;
            case D3D12_RAYTRACING_TIER_1_0: return "Optional (Yes - Tier 1.0)";
            case D3D12_RAYTRACING_TIER_1_1: return "Optional (Yes - Tier 1.1)";
            default: return c_szOptYes;
            }
        }

        return c_szOptNo;
    }

    const char* D3D12VRSSupported(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 d3d12opts = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6))))
        {
            switch (d3d12opts.VariableShadingRateTier)
            {
            case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: break;
            case D3D12_VARIABLE_SHADING_RATE_TIER_1: return "Optional (Yes - Tier 1)";
            case D3D12_VARIABLE_SHADING_RATE_TIER_2: return "Optional (Yes - Teir 2)";
            default: return c_szOptYes;
            }
        }

        return c_szOptNo;
    }

    bool IsD3D12MeshShaderSupported(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 d3d12opts = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7))))
        {
            return d3d12opts.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
        }

        return false;
    }

    //-----------------------------------------------------------------------------
#define ENUMNAME(a) case a: return TEXT(#a)

    const TCHAR* FormatName(DXGI_FORMAT format)
    {
        switch (format)
        {
            ENUMNAME(DXGI_FORMAT_R32G32B32A32_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R32G32B32A32_FLOAT);
            ENUMNAME(DXGI_FORMAT_R32G32B32A32_UINT);
            ENUMNAME(DXGI_FORMAT_R32G32B32A32_SINT);
            ENUMNAME(DXGI_FORMAT_R32G32B32_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R32G32B32_FLOAT);
            ENUMNAME(DXGI_FORMAT_R32G32B32_UINT);
            ENUMNAME(DXGI_FORMAT_R32G32B32_SINT);
            ENUMNAME(DXGI_FORMAT_R16G16B16A16_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R16G16B16A16_FLOAT);
            ENUMNAME(DXGI_FORMAT_R16G16B16A16_UNORM);
            ENUMNAME(DXGI_FORMAT_R16G16B16A16_UINT);
            ENUMNAME(DXGI_FORMAT_R16G16B16A16_SNORM);
            ENUMNAME(DXGI_FORMAT_R16G16B16A16_SINT);
            ENUMNAME(DXGI_FORMAT_R32G32_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R32G32_FLOAT);
            ENUMNAME(DXGI_FORMAT_R32G32_UINT);
            ENUMNAME(DXGI_FORMAT_R32G32_SINT);
            ENUMNAME(DXGI_FORMAT_R32G8X24_TYPELESS);
            ENUMNAME(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
            ENUMNAME(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
            ENUMNAME(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
            ENUMNAME(DXGI_FORMAT_R10G10B10A2_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R10G10B10A2_UNORM);
            ENUMNAME(DXGI_FORMAT_R10G10B10A2_UINT);
            ENUMNAME(DXGI_FORMAT_R11G11B10_FLOAT);
            ENUMNAME(DXGI_FORMAT_R8G8B8A8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R8G8B8A8_UNORM);
            ENUMNAME(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_R8G8B8A8_UINT);
            ENUMNAME(DXGI_FORMAT_R8G8B8A8_SNORM);
            ENUMNAME(DXGI_FORMAT_R8G8B8A8_SINT);
            ENUMNAME(DXGI_FORMAT_R16G16_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R16G16_FLOAT);
            ENUMNAME(DXGI_FORMAT_R16G16_UNORM);
            ENUMNAME(DXGI_FORMAT_R16G16_UINT);
            ENUMNAME(DXGI_FORMAT_R16G16_SNORM);
            ENUMNAME(DXGI_FORMAT_R16G16_SINT);
            ENUMNAME(DXGI_FORMAT_R32_TYPELESS);
            ENUMNAME(DXGI_FORMAT_D32_FLOAT);
            ENUMNAME(DXGI_FORMAT_R32_FLOAT);
            ENUMNAME(DXGI_FORMAT_R32_UINT);
            ENUMNAME(DXGI_FORMAT_R32_SINT);
            ENUMNAME(DXGI_FORMAT_R24G8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_D24_UNORM_S8_UINT);
            ENUMNAME(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
            ENUMNAME(DXGI_FORMAT_R8G8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R8G8_UNORM);
            ENUMNAME(DXGI_FORMAT_R8G8_UINT);
            ENUMNAME(DXGI_FORMAT_R8G8_SNORM);
            ENUMNAME(DXGI_FORMAT_R8G8_SINT);
            ENUMNAME(DXGI_FORMAT_R16_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R16_FLOAT);
            ENUMNAME(DXGI_FORMAT_D16_UNORM);
            ENUMNAME(DXGI_FORMAT_R16_UNORM);
            ENUMNAME(DXGI_FORMAT_R16_UINT);
            ENUMNAME(DXGI_FORMAT_R16_SNORM);
            ENUMNAME(DXGI_FORMAT_R16_SINT);
            ENUMNAME(DXGI_FORMAT_R8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_R8_UNORM);
            ENUMNAME(DXGI_FORMAT_R8_UINT);
            ENUMNAME(DXGI_FORMAT_R8_SNORM);
            ENUMNAME(DXGI_FORMAT_R8_SINT);
            ENUMNAME(DXGI_FORMAT_A8_UNORM);
            ENUMNAME(DXGI_FORMAT_R1_UNORM);
            ENUMNAME(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
            ENUMNAME(DXGI_FORMAT_R8G8_B8G8_UNORM);
            ENUMNAME(DXGI_FORMAT_G8R8_G8B8_UNORM);
            ENUMNAME(DXGI_FORMAT_BC1_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC1_UNORM);
            ENUMNAME(DXGI_FORMAT_BC1_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_BC2_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC2_UNORM);
            ENUMNAME(DXGI_FORMAT_BC2_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_BC3_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC3_UNORM);
            ENUMNAME(DXGI_FORMAT_BC3_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_BC4_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC4_UNORM);
            ENUMNAME(DXGI_FORMAT_BC4_SNORM);
            ENUMNAME(DXGI_FORMAT_BC5_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC5_UNORM);
            ENUMNAME(DXGI_FORMAT_BC5_SNORM);
            ENUMNAME(DXGI_FORMAT_B5G6R5_UNORM);
            ENUMNAME(DXGI_FORMAT_B5G5R5A1_UNORM);
            ENUMNAME(DXGI_FORMAT_B8G8R8A8_UNORM);
            ENUMNAME(DXGI_FORMAT_B8G8R8X8_UNORM);
            ENUMNAME(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
            ENUMNAME(DXGI_FORMAT_B8G8R8A8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_B8G8R8X8_TYPELESS);
            ENUMNAME(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_BC6H_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC6H_UF16);
            ENUMNAME(DXGI_FORMAT_BC6H_SF16);
            ENUMNAME(DXGI_FORMAT_BC7_TYPELESS);
            ENUMNAME(DXGI_FORMAT_BC7_UNORM);
            ENUMNAME(DXGI_FORMAT_BC7_UNORM_SRGB);
            ENUMNAME(DXGI_FORMAT_AYUV);
            ENUMNAME(DXGI_FORMAT_Y410);
            ENUMNAME(DXGI_FORMAT_Y416);
            ENUMNAME(DXGI_FORMAT_NV12);
            ENUMNAME(DXGI_FORMAT_P010);
            ENUMNAME(DXGI_FORMAT_P016);
            ENUMNAME(DXGI_FORMAT_420_OPAQUE);
            ENUMNAME(DXGI_FORMAT_YUY2);
            ENUMNAME(DXGI_FORMAT_Y210);
            ENUMNAME(DXGI_FORMAT_Y216);
            ENUMNAME(DXGI_FORMAT_NV11);
            ENUMNAME(DXGI_FORMAT_AI44);
            ENUMNAME(DXGI_FORMAT_IA44);
            ENUMNAME(DXGI_FORMAT_P8);
            ENUMNAME(DXGI_FORMAT_A8P8);
            ENUMNAME(DXGI_FORMAT_B4G4R4A4_UNORM);
            ENUMNAME(DXGI_FORMAT_P208);
            ENUMNAME(DXGI_FORMAT_V208);
            ENUMNAME(DXGI_FORMAT_V408);

        default:
            return TEXT("DXGI_FORMAT_UNKNOWN");
        }
    }

    const TCHAR* FLName(D3D10_FEATURE_LEVEL1 lvl)
    {
        switch (lvl)
        {
            ENUMNAME(D3D10_FEATURE_LEVEL_10_0);
            ENUMNAME(D3D10_FEATURE_LEVEL_10_1);
            ENUMNAME(D3D10_FEATURE_LEVEL_9_1);
            ENUMNAME(D3D10_FEATURE_LEVEL_9_2);
            ENUMNAME(D3D10_FEATURE_LEVEL_9_3);

        default:
            return TEXT("D3D10_FEATURE_LEVEL_UNKNOWN");
        }
    }

    const TCHAR* FLName(D3D_FEATURE_LEVEL lvl)
    {
        switch (lvl)
        {
            ENUMNAME(D3D_FEATURE_LEVEL_9_1);
            ENUMNAME(D3D_FEATURE_LEVEL_9_2);
            ENUMNAME(D3D_FEATURE_LEVEL_9_3);
            ENUMNAME(D3D_FEATURE_LEVEL_10_0);
            ENUMNAME(D3D_FEATURE_LEVEL_10_1);
            ENUMNAME(D3D_FEATURE_LEVEL_11_0);
            ENUMNAME(D3D_FEATURE_LEVEL_11_1);
            ENUMNAME(D3D_FEATURE_LEVEL_12_0);
            ENUMNAME(D3D_FEATURE_LEVEL_12_1);
            return TEXT("D3D_FEATURE_LEVEL_12_2");

        default:
            return TEXT("D3D_FEATURE_LEVEL_UNKNOWN");
        }
    }

    //-----------------------------------------------------------------------------
    UINT RefreshRate(const DXGI_RATIONAL& Rate)
    {
        if (Rate.Numerator == 0)
            return 0;
        else if (Rate.Denominator == 0)
            return 0;
        else if (Rate.Denominator == 1)
            return Rate.Numerator;
        else
            return (UINT)(float(Rate.Numerator) / float(Rate.Denominator));
    }

    //-----------------------------------------------------------------------------
    HRESULT DXGIAdapterInfo(LPARAM /*lParam1*/, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pAdapter = reinterpret_cast<IDXGIAdapter*>(lParam2);
        if (!pAdapter)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 50);
        }

        DXGI_ADAPTER_DESC desc;
        HRESULT hr = pAdapter->GetDesc(&desc);
        if (FAILED(hr))
            return hr;

        DWORD dvm = (DWORD)(desc.DedicatedVideoMemory / (1024 * 1024));
        DWORD dsm = (DWORD)(desc.DedicatedSystemMemory / (1024 * 1024));
        DWORD ssm = (DWORD)(desc.SharedSystemMemory / (1024 * 1024));

        char szDesc[128];

        wcstombs_s(nullptr, szDesc, desc.Description, 128);

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, "Description");
            LVAddText(g_hwndLV, 1, "%s", szDesc);

            LVAddText(g_hwndLV, 0, "VendorId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.VendorId);

            LVAddText(g_hwndLV, 0, "DeviceId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.DeviceId);

            LVAddText(g_hwndLV, 0, "SubSysId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.SubSysId);

            LVAddText(g_hwndLV, 0, "Revision");
            LVAddText(g_hwndLV, 1, "%d", desc.Revision);

            LVAddText(g_hwndLV, 0, "DedicatedVideoMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", dvm);

            LVAddText(g_hwndLV, 0, "DedicatedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", dsm);

            LVAddText(g_hwndLV, 0, "SharedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", ssm);
        }
        else
        {
            PrintStringValueLine("Description", szDesc, pPrintInfo);
            PrintHexValueLine("VendorId", desc.VendorId, pPrintInfo);
            PrintHexValueLine("DeviceId", desc.DeviceId, pPrintInfo);
            PrintHexValueLine("SubSysId", desc.SubSysId, pPrintInfo);
            PrintValueLine("Revision", desc.Revision, pPrintInfo);
            PrintValueLine("DedicatedVideoMemory (MB)", dvm, pPrintInfo);
            PrintValueLine("DedicatedSystemMemory (MB)", dsm, pPrintInfo);
            PrintValueLine("SharedSystemMemory (MB)", ssm, pPrintInfo);
        }

        return S_OK;
    }

    HRESULT DXGIAdapterInfo1(LPARAM /*lParam1*/, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pAdapter = reinterpret_cast<IDXGIAdapter1*>(lParam2);
        if (!pAdapter)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 50);
        }

        DXGI_ADAPTER_DESC1 desc;
        HRESULT hr = pAdapter->GetDesc1(&desc);
        if (FAILED(hr))
            return hr;

        auto dvm = static_cast<DWORD>(desc.DedicatedVideoMemory / (1024 * 1024));
        auto dsm = static_cast<DWORD>(desc.DedicatedSystemMemory / (1024 * 1024));
        auto ssm = static_cast<DWORD>(desc.SharedSystemMemory / (1024 * 1024));

        char szDesc[128];

        wcstombs_s(nullptr, szDesc, desc.Description, 128);

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, "Description");
            LVAddText(g_hwndLV, 1, "%s", szDesc);

            LVAddText(g_hwndLV, 0, "VendorId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.VendorId);

            LVAddText(g_hwndLV, 0, "DeviceId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.DeviceId);

            LVAddText(g_hwndLV, 0, "SubSysId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.SubSysId);

            LVAddText(g_hwndLV, 0, "Revision");
            LVAddText(g_hwndLV, 1, "%d", desc.Revision);

            LVAddText(g_hwndLV, 0, "DedicatedVideoMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", dvm);

            LVAddText(g_hwndLV, 0, "DedicatedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", dsm);

            LVAddText(g_hwndLV, 0, "SharedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", ssm);

            LVAddText(g_hwndLV, 0, "Remote");
            LVAddText(g_hwndLV, 1, (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo);
        }
        else
        {
            PrintStringValueLine("Description", szDesc, pPrintInfo);
            PrintHexValueLine("VendorId", desc.VendorId, pPrintInfo);
            PrintHexValueLine("DeviceId", desc.DeviceId, pPrintInfo);
            PrintHexValueLine("SubSysId", desc.SubSysId, pPrintInfo);
            PrintValueLine("Revision", desc.Revision, pPrintInfo);
            PrintValueLine("DedicatedVideoMemory (MB)", dvm, pPrintInfo);
            PrintValueLine("DedicatedSystemMemory (MB)", dsm, pPrintInfo);
            PrintValueLine("SharedSystemMemory (MB)", ssm, pPrintInfo);
            PrintStringValueLine("Remote", (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo, pPrintInfo);
        }

        return S_OK;
    }

    HRESULT DXGIAdapterInfo2(LPARAM /*lParam1*/, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pAdapter = reinterpret_cast<IDXGIAdapter2*>(lParam2);
        if (!pAdapter)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 50);
        }

        DXGI_ADAPTER_DESC2 desc;
        HRESULT hr = pAdapter->GetDesc2(&desc);
        if (FAILED(hr))
            return hr;

        auto dvm = static_cast<DWORD>(desc.DedicatedVideoMemory / (1024 * 1024));
        auto dsm = static_cast<DWORD>(desc.DedicatedSystemMemory / (1024 * 1024));
        auto ssm = static_cast<DWORD>(desc.SharedSystemMemory / (1024 * 1024));

        char szDesc[128];

        wcstombs_s(nullptr, szDesc, desc.Description, 128);

        const char* gpg = nullptr;
        const char* cpg = nullptr;

        switch (desc.GraphicsPreemptionGranularity)
        {
        case DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY:    gpg = "DMA Buffer"; break;
        case DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY:     gpg = "Primitive"; break;
        case DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY:      gpg = "Triangle"; break;
        case DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY:         gpg = "Pixel"; break;
        case DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY:   gpg = "Instruction"; break;
        default:                                              gpg = "Unknown"; break;
        }

        switch (desc.ComputePreemptionGranularity)
        {
        case DXGI_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY:    cpg = "DMA Buffer"; break;
        case DXGI_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY:      cpg = "Dispatch"; break;
        case DXGI_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY:  cpg = "Thread Group"; break;
        case DXGI_COMPUTE_PREEMPTION_THREAD_BOUNDARY:        cpg = "Thread"; break;
        case DXGI_COMPUTE_PREEMPTION_INSTRUCTION_BOUNDARY:   cpg = "Instruction"; break;
        default:                                             cpg = "Unknown"; break;
        }

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, "Description");
            LVAddText(g_hwndLV, 1, "%s", szDesc);

            LVAddText(g_hwndLV, 0, "VendorId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.VendorId);

            LVAddText(g_hwndLV, 0, "DeviceId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.DeviceId);

            LVAddText(g_hwndLV, 0, "SubSysId");
            LVAddText(g_hwndLV, 1, "0x%08x", desc.SubSysId);

            LVAddText(g_hwndLV, 0, "Revision");
            LVAddText(g_hwndLV, 1, "%d", desc.Revision);

            LVAddText(g_hwndLV, 0, "DedicatedVideoMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", dvm);

            LVAddText(g_hwndLV, 0, "DedicatedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", dsm);

            LVAddText(g_hwndLV, 0, "SharedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, "%d", ssm);

            LVAddText(g_hwndLV, 0, "Remote");
            LVAddText(g_hwndLV, 1, (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo);

            LVAddText(g_hwndLV, 0, "Graphics Preemption Granularity");
            LVAddText(g_hwndLV, 1, gpg);

            LVAddText(g_hwndLV, 0, "Compute Preemption Granularity");
            LVAddText(g_hwndLV, 1, cpg);
        }
        else
        {
            PrintStringValueLine("Description", szDesc, pPrintInfo);
            PrintHexValueLine("VendorId", desc.VendorId, pPrintInfo);
            PrintHexValueLine("DeviceId", desc.DeviceId, pPrintInfo);
            PrintHexValueLine("SubSysId", desc.SubSysId, pPrintInfo);
            PrintValueLine("Revision", desc.Revision, pPrintInfo);
            PrintValueLine("DedicatedVideoMemory (MB)", dvm, pPrintInfo);
            PrintValueLine("DedicatedSystemMemory (MB)", dsm, pPrintInfo);
            PrintValueLine("SharedSystemMemory (MB)", ssm, pPrintInfo);
            PrintStringValueLine("Remote", (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo, pPrintInfo);
            PrintStringValueLine("Graphics Preemption Granularity", gpg, pPrintInfo);
            PrintStringValueLine("Compute Preemption Granularity", cpg, pPrintInfo);
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    HRESULT DXGIOutputInfo(LPARAM /*lParam1*/, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pOutput = reinterpret_cast<IDXGIOutput*>(lParam2);
        if (!pOutput)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 30);
        }

        DXGI_OUTPUT_DESC desc;
        HRESULT hr = pOutput->GetDesc(&desc);
        if (FAILED(hr))
            return hr;

        char szDevName[32];

        wcstombs_s(nullptr, szDevName, desc.DeviceName, 32);

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, "DeviceName");
            LVAddText(g_hwndLV, 1, "%s", szDevName);

            LVAddText(g_hwndLV, 0, "AttachedToDesktop");
            LVAddText(g_hwndLV, 1, desc.AttachedToDesktop ? c_szYes : c_szNo);

            LVAddText(g_hwndLV, 0, "Rotation");
            LVAddText(g_hwndLV, 1, szRotation[desc.Rotation]);
        }
        else
        {
            PrintStringValueLine("DeviceName", szDevName, pPrintInfo);
            PrintStringValueLine("AttachedToDesktop", desc.AttachedToDesktop ? c_szYes : c_szNo, pPrintInfo);
            PrintStringValueLine("Rotation", szRotation[desc.Rotation], pPrintInfo);
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    HRESULT DXGIOutputModes(LPARAM /*lParam1*/, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pOutput = reinterpret_cast<IDXGIOutput*>(lParam2);
        if (!pOutput)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Resolution", 10);
            LVAddColumn(g_hwndLV, 1, "Pixel Format", 30);
            LVAddColumn(g_hwndLV, 2, "Refresh Rate", 10);
        }

        for (UINT iFormat = 0; iFormat < NumAdapterFormats; ++iFormat)
        {
            DXGI_FORMAT fmt = AdapterFormatArray[iFormat];

            if (!g_DXGIFactory1)
            {
                switch (fmt)
                {
                case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
                case DXGI_FORMAT_B8G8R8A8_UNORM:
                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                    continue;
                }
            }

            UINT num = 0;
            const DWORD flags = 0;
            HRESULT hr = pOutput->GetDisplayModeList(fmt, flags, &num, 0);

            if (SUCCEEDED(hr) && num > 0)
            {
                auto pDescs = new (std::nothrow) DXGI_MODE_DESC[num];
                if (!pDescs)
                   return E_OUTOFMEMORY;

                hr = pOutput->GetDisplayModeList(fmt, flags, &num, pDescs);

                if (SUCCEEDED(hr))
                {
                    for (UINT iMode = 0; iMode < num; ++iMode)
                    {
                        const DXGI_MODE_DESC* pDesc = &pDescs[iMode];
                        if (!pPrintInfo)
                        {
                            LVAddText(g_hwndLV, 0, "%d x %d", pDesc->Width, pDesc->Height);
                            LVAddText(g_hwndLV, 1, FormatName(pDesc->Format));
                            LVAddText(g_hwndLV, 2, "%d", RefreshRate(pDesc->RefreshRate));
                        }
                        else
                        {
                            char  szBuff[80];
                            DWORD cchLen;

                            // Calculate Name and Value column x offsets
                            int x1 = (pPrintInfo->dwCurrIndent * DEF_TAB_SIZE * pPrintInfo->dwCharWidth);
                            int x2 = x1 + (30 * pPrintInfo->dwCharWidth);
                            int x3 = x2 + (20 * pPrintInfo->dwCharWidth);
                            int yLine = (pPrintInfo->dwCurrLine * pPrintInfo->dwLineHeight);

                            sprintf_s(szBuff, sizeof(szBuff), "%u x %u", pDesc->Width, pDesc->Height);
                            cchLen = static_cast<DWORD>(_tcslen(szBuff));
                            if (FAILED(PrintLine(x1, yLine, szBuff, cchLen, pPrintInfo)))
                                return E_FAIL;

                            strcpy_s(szBuff, sizeof(szBuff), FormatName(pDesc->Format));
                            cchLen = static_cast<DWORD>(_tcslen(szBuff));
                            if (FAILED(PrintLine(x2, yLine, szBuff, cchLen, pPrintInfo)))
                                return E_FAIL;

                            sprintf_s(szBuff, sizeof(szBuff), "%u", RefreshRate(pDesc->RefreshRate));
                            cchLen = static_cast<DWORD>(_tcslen(szBuff));
                            if (FAILED(PrintLine(x3, yLine, szBuff, cchLen, pPrintInfo)))
                                return E_FAIL;

                            // Advance to next line on page
                            if (FAILED(PrintNextLine(pPrintInfo)))
                                return E_FAIL;
                        }
                    }
                }

                delete[] pDescs;
            }
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
#define LVYESNO(a,b) \
        if ( g_dwViewState == IDM_VIEWALL || (b) ) \
        { \
            LVAddText( g_hwndLV, 0, a ); \
            LVAddText( g_hwndLV, 1, (b) ? c_szYes : c_szNo ); \
        }

#define PRINTYESNO(a,b) \
        PrintStringValueLine( a, (b) ? c_szYes : c_szNo, pPrintInfo );

#define LVLINE(a,b) \
        if ( g_dwViewState == IDM_VIEWALL || (b != c_szNA && b != c_szNo) ) \
        { \
            LVAddText( g_hwndLV, 0, a ); \
            LVAddText( g_hwndLV, 1, b ); \
        }

#define PRINTLINE(a,b) \
        if ( g_dwViewState == IDM_VIEWALL || (b != c_szNA && b != c_szNo) ) \
        { \
            PrintStringValueLine( a, b, pPrintInfo ); \
        }

#define XTOSTRING(a) #a TOSTRING(a)
#define TOSTRING(a) #a

#define XTOSTRING2(a) #a TOSTRING2(a)
#define TOSTRING2(a) "( " #a " )"

    //-----------------------------------------------------------------------------
    HRESULT DXGIFeatures(LPARAM /*lParam1*/, LPARAM /*lParam2*/, PRINTCBINFO* pPrintInfo)
    {
        if (g_DXGIFactory5)
        {
            if (!pPrintInfo)
            {
                LVAddColumn(g_hwndLV, 0, "Name", 30);
                LVAddColumn(g_hwndLV, 1, "Value", 60);
            }

            BOOL allowTearing = FALSE;
            HRESULT hr = g_DXGIFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL));
            if (FAILED(hr))
                allowTearing = FALSE;

            if (!pPrintInfo)
            {
                LVYESNO("Allow tearing", allowTearing);
            }
            else
            {
                PRINTYESNO("Allow tearing", allowTearing);
            }
        }

        return S_OK;
    }

    bool IsMSAAPowerOf2(UINT value)
    {
        // only supports values 0..32
        if (value <= 1)
            return true;

        if (value & 0x1)
            return false;

        for (UINT mask = 0x2; mask < 0x100; mask <<= 1)
        {
            if ((value & ~mask) == 0)
                return true;
        }

        return false;
    }


    //-----------------------------------------------------------------------------
    void CheckExtendedFormats(ID3D10Device* pDevice, BOOL& ext, BOOL& x2)
    {
        ext = x2 = FALSE;

        // DXGI 1.1 required for extended formats
        if (!g_DXGIFactory1)
            return;

        UINT fmtSupport = 0;
        HRESULT hr = pDevice->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, &fmtSupport);
        if (FAILED(hr))
            fmtSupport = 0;

        if (fmtSupport & D3D10_FORMAT_SUPPORT_RENDER_TARGET)
            ext = TRUE;

        hr = pDevice->CheckFormatSupport(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, &fmtSupport);
        if (FAILED(hr))
            fmtSupport = 0;

        if (fmtSupport & D3D10_FORMAT_SUPPORT_DISPLAY)
            x2 = TRUE;
    }

    void CheckExtendedFormats(ID3D11Device* pDevice, BOOL& ext, BOOL& x2, BOOL& bpp565)
    {
        ext = x2 = bpp565 = FALSE;

        UINT fmtSupport = 0;
        HRESULT hr = pDevice->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, &fmtSupport);
        if (FAILED(hr))
            fmtSupport = 0;

        if (fmtSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET)
            ext = TRUE;

        fmtSupport = 0;
        hr = pDevice->CheckFormatSupport(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, &fmtSupport);
        if (FAILED(hr))
            fmtSupport = 0;

        if (fmtSupport & D3D11_FORMAT_SUPPORT_DISPLAY)
            x2 = TRUE;

        // DXGI 1.2 is required for 16bpp support
        if (g_DXGIFactory2)
        {
            fmtSupport = 0;
            hr = pDevice->CheckFormatSupport(DXGI_FORMAT_B5G6R5_UNORM, &fmtSupport);
            if (FAILED(hr))
                fmtSupport = 0;

            if (fmtSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D)
                bpp565 = TRUE;
        }
    }


    //-----------------------------------------------------------------------------
    void CheckD3D11Ops(ID3D11Device* pDevice, bool& logicOps, bool& cbPartial, bool& cbOffsetting)
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS d3d11opts = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &d3d11opts, sizeof(d3d11opts));
        if (FAILED(hr))
            memset(&d3d11opts, 0, sizeof(d3d11opts));

        logicOps = (d3d11opts.OutputMergerLogicOp) ? true : false;
        cbPartial = (d3d11opts.ConstantBufferPartialUpdate) ? true : false;
        cbOffsetting = (d3d11opts.ConstantBufferOffsetting) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D11Ops1(ID3D11Device* pDevice, D3D11_TILED_RESOURCES_TIER& tiled, bool& minmaxfilter, bool& mapdefaultbuff)
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS1 d3d11opts1 = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &d3d11opts1, sizeof(d3d11opts1));
        if (FAILED(hr))
            memset(&d3d11opts1, 0, sizeof(d3d11opts1));

        tiled = d3d11opts1.TiledResourcesTier;
        minmaxfilter = (d3d11opts1.MinMaxFiltering) ? true : false;
        mapdefaultbuff = (d3d11opts1.MapOnDefaultBuffers) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D11Ops2(ID3D11Device* pDevice, D3D11_TILED_RESOURCES_TIER& tiled, D3D11_CONSERVATIVE_RASTERIZATION_TIER& crast,
        bool& rovs, bool& pssref)
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS2 d3d11opts2 = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &d3d11opts2, sizeof(d3d11opts2));
        if (FAILED(hr))
            memset(&d3d11opts2, 0, sizeof(d3d11opts2));

        crast = d3d11opts2.ConservativeRasterizationTier;
        tiled = d3d11opts2.TiledResourcesTier;
        rovs = (d3d11opts2.ROVsSupported) ? true : false;
        pssref = (d3d11opts2.PSSpecifiedStencilRefSupported) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D9Ops(ID3D11Device* pDevice, bool& nonpow2, bool& shadows)
    {
        D3D11_FEATURE_DATA_D3D9_OPTIONS d3d9opts = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D9_OPTIONS, &d3d9opts, sizeof(d3d9opts));
        if (FAILED(hr))
            memset(&d3d9opts, 0, sizeof(d3d9opts));

        nonpow2 = (d3d9opts.FullNonPow2TextureSupport) ? true : false;

        D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT d3d9shadows = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D9_SHADOW_SUPPORT, &d3d9shadows, sizeof(d3d9shadows));
        if (FAILED(hr))
            memset(&d3d9shadows, 0, sizeof(d3d9shadows));

        shadows = (d3d9shadows.SupportsDepthAsTextureWithLessEqualComparisonFilter) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D9Ops1(ID3D11Device* pDevice, bool& nonpow2, bool& shadows, bool& instancing, bool& cubemapRT)
    {
        D3D11_FEATURE_DATA_D3D9_OPTIONS1 d3d9opts = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D9_OPTIONS1, &d3d9opts, sizeof(d3d9opts));
        if (FAILED(hr))
        {
            memset(&d3d9opts, 0, sizeof(d3d9opts));

            // Handle Windows 8.1 Preview by falling back to D3D9_OPTIONS, D3D9_SHADOW_SUPPORT, and D3D9_SIMPLE_INSTANCING_SUPPORT
            CheckD3D9Ops(pDevice, nonpow2, shadows);

            D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT d3d9si = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT, &d3d9si, sizeof(d3d9si));
            if (FAILED(hr))
                memset(&d3d9si, 0, sizeof(d3d9si));

            instancing = (d3d9si.SimpleInstancingSupported) ? true : false;

            cubemapRT = false;

            return;
        }

        nonpow2 = (d3d9opts.FullNonPow2TextureSupported) ? true : false;

        shadows = (d3d9opts.DepthAsTextureWithLessEqualComparisonFilterSupported) ? true : false;

        instancing = (d3d9opts.SimpleInstancingSupported) ? true : false;

        cubemapRT = (d3d9opts.TextureCubeFaceRenderTargetWithNonCubeDepthStencilSupported) ? true : false;
    }

//-----------------------------------------------------------------------------
#define D3D_FL_LPARAM3_D3D10( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 0 )
#define D3D_FL_LPARAM3_D3D10_1( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 1 )
#define D3D_FL_LPARAM3_D3D11( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 2 )
#define D3D_FL_LPARAM3_D3D11_1( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 3 )
#define D3D_FL_LPARAM3_D3D11_2( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 4 )
#define D3D_FL_LPARAM3_D3D11_3( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 5 )
#define D3D_FL_LPARAM3_D3D12( d3dType ) ( ( (d3dType & 0xff) << 8 ) | 10 )

    HRESULT D3D_FeatureLevel(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto fl = static_cast<D3D_FEATURE_LEVEL>(lParam1);
        if (lParam2 == 0)
            return S_OK;

        ID3D10Device* pD3D10 = nullptr;
        ID3D10Device1* pD3D10_1 = nullptr;
        ID3D11Device* pD3D11 = nullptr;
        ID3D11Device1* pD3D11_1 = nullptr;
        ID3D11Device2* pD3D11_2 = nullptr;
        ID3D11Device3* pD3D11_3 = nullptr;
        ID3D12Device* pD3D12 = nullptr;

        auto d3dVer = static_cast<unsigned int>(lParam3 & 0xff);
        auto d3dType = static_cast<D3D_DRIVER_TYPE>((lParam3 & 0xff00) >> 8);

        if (d3dVer == 10)
        {
            pD3D12 = reinterpret_cast<ID3D12Device*>(lParam2);
        }
        else if (d3dVer == 5)
        {
            pD3D11_3 = reinterpret_cast<ID3D11Device3*>(lParam2);
        }
        else
        {
            if (fl > D3D_FEATURE_LEVEL_11_1)
                fl = D3D_FEATURE_LEVEL_11_1;

            if (d3dVer == 4)
            {
                pD3D11_2 = reinterpret_cast<ID3D11Device2*>(lParam2);
            }
            else if (d3dVer == 3)
            {
                pD3D11_1 = reinterpret_cast<ID3D11Device1*>(lParam2);
            }
            else
            {
                if (fl > D3D_FEATURE_LEVEL_11_0)
                    fl = D3D_FEATURE_LEVEL_11_0;

                if (d3dVer == 2)
                {
                    pD3D11 = reinterpret_cast<ID3D11Device*>(lParam2);
                }
                else
                {
                    if (fl > D3D_FEATURE_LEVEL_10_1)
                        fl = D3D_FEATURE_LEVEL_10_1;

                    if (d3dVer == 1)
                    {
                        pD3D10_1 = reinterpret_cast<ID3D10Device1*>(lParam2);
                    }
                    else
                    {
                        if (fl > D3D_FEATURE_LEVEL_10_0)
                            fl = D3D_FEATURE_LEVEL_10_0;

                        pD3D10 = reinterpret_cast<ID3D10Device*>(lParam2);
                    }
                }
            }
        }

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        const char* shaderModel = nullptr;
        const char* computeShader = c_szNo;
        const char* maxTexDim = nullptr;
        const char* maxCubeDim = nullptr;
        const char* maxVolDim = nullptr;
        const char* maxTexRepeat = nullptr;
        const char* maxAnisotropy = nullptr;
        const char* maxPrimCount = "4294967296";
        const char* maxInputSlots = nullptr;
        const char* mrt = nullptr;
        const char* extFormats = nullptr;
        const char* x2_10BitFormat = nullptr;
        const char* logic_ops = c_szNo;
        const char* cb_partial = c_szNA;
        const char* cb_offsetting = c_szNA;
        const char* uavSlots = nullptr;
        const char* uavEveryStage = nullptr;
        const char* uavOnlyRender = nullptr;
        const char* nonpow2 = nullptr;
        const char* bpp16 = c_szNo;
        const char* shadows = nullptr;
        const char* cubeRT = nullptr;
        const char* tiled_rsc = nullptr;
        const char* binding_rsc = nullptr;
        const char* minmaxfilter = nullptr;
        const char* mapdefaultbuff = nullptr;
        const char* consrv_rast = nullptr;
        const char* rast_ordered_views = nullptr;
        const char* ps_stencil_ref = nullptr;
        const char* instancing = nullptr;
        const char* vrs = nullptr;
        const char* meshShaders = nullptr;
        const char* dxr = nullptr;

        BOOL _10level9 = FALSE;

        switch (fl)
        {
        case D3D_FEATURE_LEVEL_12_2:
            if (pD3D12)
            {
                switch (GetD3D12ShaderModel(pD3D12))
                {
                case D3D_SHADER_MODEL_6_7:
                    shaderModel = "6.7 (Optional)";
                    computeShader = "Yes (CS 6.7)";
                    break;
                case D3D_SHADER_MODEL_6_6:
                    shaderModel = "6.6 (Optional)";
                    computeShader = "Yes (CS 6.6)";
                    break;
                default:
                    shaderModel = "6.5";
                    computeShader = "Yes (CS 6.5)";
                    break;
                }
                vrs = "Yes - Tier 2";
                meshShaders = c_szYes;
                dxr = "Yes - Tier 1.1";
            }
            // Fall-through

        case D3D_FEATURE_LEVEL_12_1:
        case D3D_FEATURE_LEVEL_12_0:
            if (!shaderModel)
            {
                switch (GetD3D12ShaderModel(pD3D12))
                {
                case D3D_SHADER_MODEL_6_7:
                    shaderModel = "6.7 (Optional)";
                    computeShader = "Yes (CS 6.7)";
                    break;
                case D3D_SHADER_MODEL_6_6:
                    shaderModel = "6.6 (Optional)";
                    computeShader = "Yes (CS 6.6)";
                    break;
                case D3D_SHADER_MODEL_6_5:
                    shaderModel = "6.5 (Optional)";
                    computeShader = "Yes (CS 6.5)";
                    break;
                case D3D_SHADER_MODEL_6_4:
                    shaderModel = "6.4 (Optional)";
                    computeShader = "Yes (CS 6.4)";
                    break;
                case D3D_SHADER_MODEL_6_3:
                    shaderModel = "6.3 (Optional)";
                    computeShader = "Yes (CS 6.3)";
                    break;
                case D3D_SHADER_MODEL_6_2:
                    shaderModel = "6.2 (Optional)";
                    computeShader = "Yes (CS 6.2)";
                    break;
                case D3D_SHADER_MODEL_6_1:
                    shaderModel = "6.1 (Optional)";
                    computeShader = "Yes (CS 6.1)";
                    break;
                case D3D_SHADER_MODEL_6_0:
                    shaderModel = "6.0 (Optional)";
                    computeShader = "Yes (CS 6.0)";
                    break;
                default:
                    shaderModel = "5.1";
                    computeShader = "Yes (CS 5.1)";
                    break;
                }
            }
            extFormats = c_szYes;
            x2_10BitFormat = c_szYes;
            logic_ops = c_szYes;
            cb_partial = c_szYes;
            cb_offsetting = c_szYes;
            uavEveryStage = c_szYes;
            uavOnlyRender = "16";
            nonpow2 = "Full";
            bpp16 = c_szYes;
            instancing = c_szYes;

            if (pD3D12)
            {
                maxTexDim = XTOSTRING(D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
                maxCubeDim = XTOSTRING(D3D12_REQ_TEXTURECUBE_DIMENSION);
                maxVolDim = XTOSTRING(D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
                maxTexRepeat = XTOSTRING(D3D12_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
                maxAnisotropy = XTOSTRING(D3D12_REQ_MAXANISOTROPY);
                maxInputSlots = XTOSTRING(D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
                mrt = XTOSTRING(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
                uavSlots = XTOSTRING(D3D12_UAV_SLOT_COUNT);

                D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
                HRESULT hr = pD3D12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
                if (FAILED(hr))
                    memset(&d3d12opts, 0, sizeof(d3d12opts));

                switch (d3d12opts.TiledResourcesTier)
                {
                    // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
                case D3D12_TILED_RESOURCES_TIER_2:  tiled_rsc = "Yes - Tier 2"; break;
                case D3D12_TILED_RESOURCES_TIER_3:  tiled_rsc = "Yes - Tier 3"; break;
                case D3D12_TILED_RESOURCES_TIER_4:  tiled_rsc = "Yes - Tier 4"; break;
                default:                            tiled_rsc = c_szYes;        break;
                }

                switch (d3d12opts.ResourceBindingTier)
                {
                    // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
                case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = "Yes - Tier 2"; break;
                case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = "Yes - Tier 3"; break;
                default:                            binding_rsc = c_szYes;        break;
                }

                if (fl >= D3D_FEATURE_LEVEL_12_1)
                {
                    switch (d3d12opts.ConservativeRasterizationTier)
                    {
                        // 12.1 requires T1, 12.2 requires T3
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:   consrv_rast = "Yes - Tier 1";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:   consrv_rast = "Yes - Tier 2";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:   consrv_rast = "Yes - Tier 3";  break;
                    default:                                        consrv_rast = c_szYes;         break;
                    }

                    rast_ordered_views = c_szYes;
                }
                else
                {
                    switch (d3d12opts.ConservativeRasterizationTier)
                    {
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:   consrv_rast = c_szOptNo; break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:               consrv_rast = "Optional (Yes - Tier 1)";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:               consrv_rast = "Optional (Yes - Tier 2)";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:               consrv_rast = "Optional (Yes - Tier 3)";  break;
                    default:                                                    consrv_rast = c_szOptYes; break;
                    }

                    rast_ordered_views = (d3d12opts.ROVsSupported) ? c_szOptYes : c_szOptNo;
                }

                ps_stencil_ref = (d3d12opts.PSSpecifiedStencilRefSupported) ? c_szOptYes : c_szOptNo;
                minmaxfilter = c_szYes;
                mapdefaultbuff = c_szYes;
            }
            else if (pD3D11_3)
            {
                maxTexDim = XTOSTRING(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
                maxCubeDim = XTOSTRING(D3D11_REQ_TEXTURECUBE_DIMENSION);
                maxVolDim = XTOSTRING(D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
                maxTexRepeat = XTOSTRING(D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
                maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
                maxInputSlots = XTOSTRING(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
                mrt = XTOSTRING(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
                uavSlots = XTOSTRING(D3D11_1_UAV_SLOT_COUNT);

                D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
                bool bMinMaxFilter, bMapDefaultBuff;
                CheckD3D11Ops1(pD3D11_3, tiled, bMinMaxFilter, bMapDefaultBuff);

                D3D11_CONSERVATIVE_RASTERIZATION_TIER crast = D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED;
                bool rovs, pssref;
                CheckD3D11Ops2(pD3D11_3, tiled, crast, rovs, pssref);

                switch (tiled)
                {
                    // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Yes - Tier 2";  break;
                case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = "Yes - Tier 3";  break;
                default:                                    tiled_rsc = c_szYes;         break;
                }

                if (fl >= D3D_FEATURE_LEVEL_12_1)
                {
                    switch (crast)
                    {
                        // 12.1 requires T1
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = "Yes - Tier 1";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = "Yes - Tier 2";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = "Yes - Tier 3";  break;
                    default:                                                consrv_rast = c_szYes;         break;
                    }

                    rast_ordered_views = c_szYes;
                }
                else
                {
                    switch (crast)
                    {
                    case D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED:    consrv_rast = c_szOptNo; break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = "Optional (Yes - Tier 1)";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = "Optional (Yes - Tier 2)";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = "Optional (Yes - Tier 3)";  break;
                    default:                                                consrv_rast = c_szOptYes; break;
                    }

                    rast_ordered_views = (rovs) ? c_szOptYes : c_szOptNo;
                }

                ps_stencil_ref = (pssref) ? c_szOptYes : c_szOptNo;
                minmaxfilter = c_szYes;
                mapdefaultbuff = c_szYes;
            }
            break;

        case D3D_FEATURE_LEVEL_11_1:
            shaderModel = "5.0";
            computeShader = "Yes (CS 5.0)";
            maxTexDim = XTOSTRING(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING(D3D11_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING(D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING(D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
            maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
            maxInputSlots = XTOSTRING(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
            extFormats = c_szYes;
            x2_10BitFormat = c_szYes;
            logic_ops = c_szYes;
            cb_partial = c_szYes;
            cb_offsetting = c_szYes;
            uavSlots = XTOSTRING(D3D11_1_UAV_SLOT_COUNT);
            uavEveryStage = c_szYes;
            uavOnlyRender = "16";
            nonpow2 = "Full";
            bpp16 = c_szYes;
            instancing = c_szYes;

            if (pD3D12)
            {
                shaderModel = "5.1";
                computeShader = "Yes (CS 5.1)";

                D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
                HRESULT hr = pD3D12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
                if (FAILED(hr))
                    memset(&d3d12opts, 0, sizeof(d3d12opts));

                switch (d3d12opts.TiledResourcesTier)
                {
                case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:  tiled_rsc = c_szOptNo; break;
                case D3D12_TILED_RESOURCES_TIER_1:              tiled_rsc = "Optional (Yes - Tier 1)";  break;
                case D3D12_TILED_RESOURCES_TIER_2:              tiled_rsc = "Optional (Yes - Tier 2)";  break;
                case D3D12_TILED_RESOURCES_TIER_3:              tiled_rsc = "Optional (Yes - Tier 3)";  break;
                case D3D12_TILED_RESOURCES_TIER_4:              tiled_rsc = "Optional (Yes - Tier 4)";  break;
                default:                                        tiled_rsc = c_szOptYes; break;
                }

                switch (d3d12opts.ResourceBindingTier)
                {
                case D3D12_RESOURCE_BINDING_TIER_1: binding_rsc = "Yes - Tier 1"; break;
                case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = "Yes - Tier 2"; break;
                case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = "Yes - Tier 3"; break;
                default:                            binding_rsc = c_szYes;        break;
                }

                switch (d3d12opts.ConservativeRasterizationTier)
                {
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:   consrv_rast = c_szOptNo; break;
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:               consrv_rast = "Optional (Yes - Tier 1)";  break;
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:               consrv_rast = "Optional (Yes - Tier 2)";  break;
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:               consrv_rast = "Optional (Yes - Tier 3)";  break;
                default:                                                    consrv_rast = c_szOptYes; break;
                }

                rast_ordered_views = (d3d12opts.ROVsSupported) ? c_szOptYes : c_szOptNo;
                ps_stencil_ref = (d3d12opts.PSSpecifiedStencilRefSupported) ? c_szOptYes : c_szOptNo;
                minmaxfilter = c_szYes;
                mapdefaultbuff = c_szYes;
            }
            else if (pD3D11_3)
            {
                D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
                bool bMinMaxFilter, bMapDefaultBuff;
                CheckD3D11Ops1(pD3D11_3, tiled, bMinMaxFilter, bMapDefaultBuff);

                D3D11_CONSERVATIVE_RASTERIZATION_TIER crast = D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED;
                bool rovs, pssref;
                CheckD3D11Ops2(pD3D11_3, tiled, crast, rovs, pssref);

                switch (tiled)
                {
                case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
                case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = "Optional (Yes - Tier 3)";  break;
                default:                                    tiled_rsc = c_szOptYes; break;
                }

                switch (crast)
                {
                case D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED:    consrv_rast = c_szOptNo; break;
                case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = "Optional (Yes - Tier 1)";  break;
                case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = "Optional (Yes - Tier 2)";  break;
                case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = "Optional (Yes - Tier 3)";  break;
                default:                                                consrv_rast = c_szOptYes; break;
                }

                rast_ordered_views = (rovs) ? c_szOptYes : c_szOptNo;
                ps_stencil_ref = (pssref) ? c_szOptYes : c_szOptNo;
                minmaxfilter = bMinMaxFilter ? c_szOptYes : c_szOptNo;
                mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11_2)
            {
                D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
                bool bMinMaxFilter, bMapDefaultBuff;
                CheckD3D11Ops1(pD3D11_2, tiled, bMinMaxFilter, bMapDefaultBuff);

                switch (tiled)
                {
                case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
                default:                                    tiled_rsc = c_szOptYes; break;
                }

                minmaxfilter = bMinMaxFilter ? c_szOptYes : c_szOptNo;
                mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
            }
            break;

        case D3D_FEATURE_LEVEL_11_0:
            shaderModel = "5.0";
            computeShader = "Yes (CS 5.0)";
            maxTexDim = XTOSTRING(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING(D3D11_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING(D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING(D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
            maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
            maxInputSlots = XTOSTRING(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
            extFormats = c_szYes;
            x2_10BitFormat = c_szYes;
            instancing = c_szYes;

            if (pD3D12)
            {
                shaderModel = "5.1";
                computeShader = "Yes (CS 5.1)";

                D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
                HRESULT hr = pD3D12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
                if (FAILED(hr))
                    memset(&d3d12opts, 0, sizeof(d3d12opts));

                switch (d3d12opts.TiledResourcesTier)
                {
                case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:  tiled_rsc = c_szOptNo; break;
                case D3D12_TILED_RESOURCES_TIER_1:              tiled_rsc = "Optional (Yes - Tier 1)";  break;
                case D3D12_TILED_RESOURCES_TIER_2:              tiled_rsc = "Optional (Yes - Tier 2)";  break;
                case D3D12_TILED_RESOURCES_TIER_3:              tiled_rsc = "Optional (Yes - Tier 3)";  break;
                case D3D12_TILED_RESOURCES_TIER_4:              tiled_rsc = "Optional (Yes - Tier 4)";  break;
                default:                                        tiled_rsc = c_szOptYes; break;
                }

                switch (d3d12opts.ResourceBindingTier)
                {
                case D3D12_RESOURCE_BINDING_TIER_1: binding_rsc = "Yes - Tier 1"; break;
                case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = "Yes - Tier 2"; break;
                case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = "Yes - Tier 3"; break;
                default:                            binding_rsc = c_szYes;        break;
                }

                logic_ops = d3d12opts.OutputMergerLogicOp ? c_szOptYes : c_szOptNo;
                consrv_rast = rast_ordered_views = ps_stencil_ref = minmaxfilter = c_szNo;
                mapdefaultbuff = c_szYes;

                uavSlots = "8";
                uavEveryStage = c_szNo;
                uavOnlyRender = "8";
                nonpow2 = "Full";
            }
            else if (pD3D11_3)
            {
                D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
                bool bMinMaxFilter, bMapDefaultBuff;
                CheckD3D11Ops1(pD3D11_3, tiled, bMinMaxFilter, bMapDefaultBuff);

                D3D11_FEATURE_DATA_D3D11_OPTIONS2 d3d11opts2 = {};
                HRESULT hr = pD3D11_3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &d3d11opts2, sizeof(d3d11opts2));
                if (SUCCEEDED(hr))
                {
                    // D3D11_FEATURE_DATA_D3D11_OPTIONS1 caps this at Tier 2
                    tiled = d3d11opts2.TiledResourcesTier;
                }

                switch (tiled)
                {
                case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
                case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = "Optional (Yes - Tier 3)";  break;
                default:                                    tiled_rsc = c_szOptYes; break;
                }

                consrv_rast = rast_ordered_views = ps_stencil_ref = minmaxfilter = c_szNo;
                mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11_2)
            {
                D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
                bool bMinMaxFilter, bMapDefaultBuff;
                CheckD3D11Ops1(pD3D11_2, tiled, bMinMaxFilter, bMapDefaultBuff);

                switch (tiled)
                {
                case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
                default:                                    tiled_rsc = c_szOptYes; break;
                }

                minmaxfilter = c_szNo;
                mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : ((pD3D11_2) ? pD3D11_2 : pD3D11_1);

                bool bLogicOps, bCBpartial, bCBoffsetting;
                CheckD3D11Ops(pD3D, bLogicOps, bCBpartial, bCBoffsetting);
                logic_ops = bLogicOps ? c_szOptYes : c_szOptNo;
                cb_partial = bCBpartial ? c_szOptYes : c_szOptNo;
                cb_offsetting = bCBoffsetting ? c_szOptYes : c_szOptNo;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D, ext, x2, bpp565);
                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                uavSlots = "8";
                uavEveryStage = c_szNo;
                uavOnlyRender = "8";
                nonpow2 = "Full";
            }
            break;

        case D3D_FEATURE_LEVEL_10_1:
            shaderModel = "4.x";
            instancing = c_szYes;

            if (pD3D11_3)
            {
                consrv_rast = rast_ordered_views = ps_stencil_ref = c_szNo;
            }

            if (pD3D11_2 || pD3D11_3)
            {
                tiled_rsc = minmaxfilter = mapdefaultbuff = c_szNo;
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : ((pD3D11_2) ? pD3D11_2 : pD3D11_1);

                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
                HRESULT hr = pD3D->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
                if (FAILED(hr))
                    memset(&d3d10xhw, 0, sizeof(d3d10xhw));

                if (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
                {
                    computeShader = "Optional (Yes - CS 4.x)";
                    uavSlots = "1";
                    uavEveryStage = c_szNo;
                    uavOnlyRender = c_szNo;
                }
                else
                {
                    computeShader = c_szOptNo;
                }

                bool bLogicOps, bCBpartial, bCBoffsetting;
                CheckD3D11Ops(pD3D, bLogicOps, bCBpartial, bCBoffsetting);
                logic_ops = bLogicOps ? c_szOptYes : c_szOptNo;
                cb_partial = bCBpartial ? c_szOptYes : c_szOptNo;
                cb_offsetting = bCBoffsetting ? c_szOptYes : c_szOptNo;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D, ext, x2, bpp565);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
                nonpow2 = "Full";
                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11)
            {
                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
                HRESULT hr = pD3D11->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
                if (FAILED(hr))
                    memset(&d3d10xhw, 0, sizeof(d3d10xhw));

                computeShader = (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) ? "Optional (Yes - CS 4.x)" : c_szOptNo;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D11, ext, x2, bpp565);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D10_1)
            {
                BOOL ext, x2;
                CheckExtendedFormats(pD3D10_1, ext, x2);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D10)
            {
                BOOL ext, x2;
                CheckExtendedFormats(pD3D10, ext, x2);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
            }

            maxTexDim = XTOSTRING(D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING(D3D10_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING(D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING(D3D10_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
            maxAnisotropy = XTOSTRING(D3D10_REQ_MAXANISOTROPY);
            maxInputSlots = XTOSTRING(D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT);
            break;

        case D3D_FEATURE_LEVEL_10_0:
            shaderModel = "4.0";
            instancing = c_szYes;

            if (pD3D11_3)
            {
                consrv_rast = rast_ordered_views = ps_stencil_ref = c_szNo;
            }

            if (pD3D11_2 || pD3D11_3)
            {
                tiled_rsc = minmaxfilter = mapdefaultbuff = c_szNo;
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : ((pD3D11_2) ? pD3D11_2 : pD3D11_1);

                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
                HRESULT hr = pD3D->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
                if (FAILED(hr))
                    memset(&d3d10xhw, 0, sizeof(d3d10xhw));

                if (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
                {
                    computeShader = "Optional (Yes - CS 4.x)";
                    uavSlots = "1";
                    uavEveryStage = c_szNo;
                    uavOnlyRender = c_szNo;
                }
                else
                {
                    computeShader = c_szOptNo;
                }

                bool bLogicOps, bCBpartial, bCBoffsetting;
                CheckD3D11Ops(pD3D, bLogicOps, bCBpartial, bCBoffsetting);
                logic_ops = bLogicOps ? c_szOptYes : c_szOptNo;
                cb_partial = bCBpartial ? c_szOptYes : c_szOptNo;
                cb_offsetting = bCBoffsetting ? c_szOptYes : c_szOptNo;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D, ext, x2, bpp565);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
                nonpow2 = "Full";
                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11)
            {
                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
                HRESULT hr = pD3D11->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
                if (FAILED(hr))
                    memset(&d3d10xhw, 0, sizeof(d3d10xhw));

                computeShader = (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) ? "Optional (Yes - CS 4.0)" : c_szOptNo;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D11, ext, x2, bpp565);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D10_1)
            {
                BOOL ext, x2;
                CheckExtendedFormats(pD3D10_1, ext, x2);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D10)
            {
                BOOL ext, x2;
                CheckExtendedFormats(pD3D10, ext, x2);

                extFormats = (ext) ? c_szOptYes : c_szOptNo;
                x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
            }

            maxTexDim = XTOSTRING(D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING(D3D10_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING(D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING(D3D10_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
            maxAnisotropy = XTOSTRING(D3D10_REQ_MAXANISOTROPY);
            maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT);
            break;

        case D3D_FEATURE_LEVEL_9_3:
            _10level9 = TRUE;
            shaderModel = "2.0 (4_0_level_9_3) [vs_2_a/ps_2_b]";
            computeShader = c_szNA;
            maxTexDim = XTOSTRING2(D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING2(D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING2(D3D_FL9_3_MAX_TEXTURE_REPEAT);
            maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
            maxPrimCount = XTOSTRING2(D3D_FL9_2_IA_PRIMITIVE_MAX_COUNT);
            maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING2(D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT);
            extFormats = c_szYes;
            instancing = c_szYes;

            if (pD3D11_2 || pD3D11_3)
            {
                ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : pD3D11_2;

                cb_partial = c_szYes;
                cb_offsetting = c_szYes;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D, ext, x2, bpp565);

                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                bool bNonPow2, bShadows, bInstancing, bCubeRT;
                CheckD3D9Ops1(pD3D, bNonPow2, bShadows, bInstancing, bCubeRT);
                nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
                cubeRT = bCubeRT ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11_1)
            {
                cb_partial = c_szYes;
                cb_offsetting = c_szYes;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D11_1, ext, x2, bpp565);

                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                bool bNonPow2, bShadows;
                CheckD3D9Ops(pD3D11_1, bNonPow2, bShadows);
                nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
            }
            break;

        case D3D_FEATURE_LEVEL_9_2:
            _10level9 = TRUE;
            shaderModel = "2.0 (4_0_level_9_1)";
            computeShader = c_szNA;
            maxTexDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING2(D3D_FL9_2_MAX_TEXTURE_REPEAT);
            maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
            maxPrimCount = XTOSTRING2(D3D_FL9_2_IA_PRIMITIVE_MAX_COUNT);
            maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING2(D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT);
            extFormats = c_szYes;
            instancing = c_szNo;

            if (pD3D11_2 || pD3D11_3)
            {
                ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : pD3D11_2;

                cb_partial = c_szYes;
                cb_offsetting = c_szYes;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D, ext, x2, bpp565);

                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                bool bNonPow2, bShadows, bInstancing, bCubeRT;
                CheckD3D9Ops1(pD3D, bNonPow2, bShadows, bInstancing, bCubeRT);
                nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
                instancing = bInstancing ? "Optional (Simple)" : c_szOptNo;
                cubeRT = bCubeRT ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11_1)
            {
                cb_partial = c_szYes;
                cb_offsetting = c_szYes;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D11_1, ext, x2, bpp565);

                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                bool bNonPow2, bShadows;
                CheckD3D9Ops(pD3D11_1, bNonPow2, bShadows);
                nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
            }
            break;

        case D3D_FEATURE_LEVEL_9_1:
            _10level9 = TRUE;
            shaderModel = "2.0 (4_0_level_9_1)";
            computeShader = c_szNA;
            maxTexDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            maxCubeDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION);
            maxVolDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            maxTexRepeat = XTOSTRING2(D3D_FL9_1_MAX_TEXTURE_REPEAT);
            maxAnisotropy = XTOSTRING2(D3D_FL9_1_DEFAULT_MAX_ANISOTROPY);
            maxPrimCount = XTOSTRING2(D3D_FL9_1_IA_PRIMITIVE_MAX_COUNT);
            maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
            mrt = XTOSTRING2(D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT);
            extFormats = c_szYes;
            instancing = c_szNo;

            if (pD3D11_2 || pD3D11_3)
            {
                ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : pD3D11_2;

                cb_partial = c_szYes;
                cb_offsetting = c_szYes;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D, ext, x2, bpp565);

                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                bool bNonPow2, bShadows, bInstancing, bCubeRT;
                CheckD3D9Ops1(pD3D, bNonPow2, bShadows, bInstancing, bCubeRT);
                nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
                instancing = bInstancing ? "Optional (Simple)" : c_szOptNo;
                cubeRT = bCubeRT ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11_1)
            {
                cb_partial = c_szYes;
                cb_offsetting = c_szYes;

                BOOL ext, x2, bpp565;
                CheckExtendedFormats(pD3D11_1, ext, x2, bpp565);

                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

                bool bNonPow2, bShadows;
                CheckD3D9Ops(pD3D11_1, bNonPow2, bShadows);
                nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
            }
            break;

        default:
            return E_FAIL;
        }

        if (d3dType == D3D_DRIVER_TYPE_WARP)
        {
            maxTexDim = (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1) ? "16777216" : "65536";
        }

        if (pD3D12)
        {
            if (!vrs)
            {
                vrs = D3D12VRSSupported(pD3D12);
            }

            if (!meshShaders)
            {
                meshShaders = IsD3D12MeshShaderSupported(pD3D12) ? c_szOptYes : c_szOptNo;
            }

            if (!dxr)
            {
                dxr = D3D12DXRSupported(pD3D12);
            }
        }

        if (!pPrintInfo)
        {
            LVLINE("Shader Model", shaderModel);
            LVYESNO("Geometry Shader", (fl >= D3D_FEATURE_LEVEL_10_0));
            LVYESNO("Stream Out", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                if (g_dwViewState == IDM_VIEWALL || computeShader != c_szNo)
                {
                    LVLINE("DirectCompute", computeShader);
                }

                LVYESNO("Hull & Domain Shaders", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            if (pD3D12)
            {
                LVLINE("Variable Rate Shading (VRS)", vrs);
                LVLINE("Mesh & Amplification Shaders", meshShaders);
                LVLINE("DirectX Raytracing", dxr);
            }

            LVYESNO("Texture Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (d3dVer != 0)
            {
                LVYESNO("Cubemap Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_1));
            }

            LVYESNO("BC4/BC5 Compression", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                LVYESNO("BC6H/BC7 Compression", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            LVYESNO("Alpha-to-coverage", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D11_1 || pD3D11_2 || pD3D11_3 || pD3D12)
            {
                LVLINE("Logic Ops (Output Merger)", logic_ops);
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                LVLINE("Constant Buffer Partial Updates", cb_partial);
                LVLINE("Constant Buffer Offsetting", cb_offsetting);

                if (uavEveryStage)
                {
                    LVLINE("UAVs at Every Stage", uavEveryStage);
                }

                if (uavOnlyRender)
                {
                    LVLINE("UAV-only rendering", uavOnlyRender);
                }
            }

            if (pD3D11_2 || pD3D11_3 || pD3D12)
            {
                if (tiled_rsc)
                {
                    LVLINE("Tiled Resources", tiled_rsc);
                }
            }

            if (pD3D11_2 || pD3D11_3)
            {
                if (minmaxfilter)
                {
                    LVLINE("Min/Max Filtering", minmaxfilter);
                }

                if (mapdefaultbuff)
                {
                    LVLINE("Map DEFAULT Buffers", mapdefaultbuff);
                }
            }

            if (pD3D11_3 || pD3D12)
            {
                if (consrv_rast)
                {
                    LVLINE("Conservative Rasterization", consrv_rast);
                }

                if (ps_stencil_ref)
                {
                    LVLINE("PS-Specified Stencil Ref", ps_stencil_ref);
                }

                if (rast_ordered_views)
                {
                    LVLINE("Rasterizer Ordered Views", rast_ordered_views);
                }
            }

            if (pD3D12 && binding_rsc)
            {
                LVLINE("Resource Binding", binding_rsc);
            }

            if (g_DXGIFactory1 && !pD3D12)
            {
                LVLINE("Extended Formats (BGRA, etc.)", extFormats);

                if (x2_10BitFormat && (g_dwViewState == IDM_VIEWALL || x2_10BitFormat != c_szNo))
                {
                    LVLINE("10-bit XR High Color Format", x2_10BitFormat);
                }
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                LVLINE("16-bit Formats (565/5551/4444)", bpp16);
            }

            if (nonpow2 && !pD3D12)
            {
                LVLINE("Non-Power-of-2 Textures", nonpow2);
            }

            LVLINE("Max Texture Dimension", maxTexDim);
            LVLINE("Max Cubemap Dimension", maxCubeDim);

            if (maxVolDim)
            {
                LVLINE("Max Volume Extent", maxVolDim);
            }

            if (maxTexRepeat)
            {
                LVLINE("Max Texture Repeat", maxTexRepeat);
            }

            LVLINE("Max Input Slots", maxInputSlots);

            if (uavSlots)
            {
                LVLINE("UAV Slots", uavSlots);
            }

            if (maxAnisotropy)
            {
                LVLINE("Max Anisotropy", maxAnisotropy);
            }

            LVLINE("Max Primitive Count", maxPrimCount);

            if (mrt)
            {
                LVLINE("Simultaneous Render Targets", mrt);
            }

            if (_10level9)
            {
                LVYESNO("Occlusion Queries", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO("Separate Alpha Blend", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO("Mirror Once", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO("Overlapping Vertex Elements", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO("Independant Write Masks", (fl >= D3D_FEATURE_LEVEL_9_3));

                LVLINE("Instancing", instancing);

                if (pD3D11_1 || pD3D11_2 || pD3D11_3)
                {
                    LVLINE("Shadow Support", shadows);
                }

                if (pD3D11_2 || pD3D11_3)
                {
                    LVLINE("Cubemap Render w/ non-Cube Depth", cubeRT);
                }
            }

            LVLINE("Note", FL_NOTE);
        }
        else
        {
            PRINTLINE("Shader Model", shaderModel);
            PRINTYESNO("Geometry Shader", (fl >= D3D_FEATURE_LEVEL_10_0));
            PRINTYESNO("Stream Out", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                PRINTLINE("DirectCompute", computeShader);
                PRINTYESNO("Hull & Domain Shaders", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            if (pD3D12)
            {
                PRINTLINE("Variable Rate Shading (VRS)", vrs);
                PRINTLINE("Mesh & Amplification Shaders", meshShaders);
                PRINTLINE("DirectX Raytracing", dxr);
            }

            PRINTYESNO("Texture Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (d3dVer != 0)
            {
                PRINTYESNO("Cubemap Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_1));
            }

            PRINTYESNO("BC4/BC5 Compression", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                PRINTYESNO("BC6H/BC7 Compression", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            PRINTYESNO("Alpha-to-coverage", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D11_1 || pD3D11_2 || pD3D11_3 || pD3D12)
            {
                PRINTLINE("Logic Ops (Output Merger)", logic_ops);
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                PRINTLINE("Constant Buffer Partial Updates", cb_partial);
                PRINTLINE("Constant Buffer Offsetting", cb_offsetting);

                if (uavEveryStage)
                {
                    PRINTLINE("UAVs at Every Stage", uavEveryStage);
                }

                if (uavOnlyRender)
                {
                    PRINTLINE("UAV-only rendering", uavOnlyRender);
                }
            }

            if (pD3D11_2 || pD3D11_3 || pD3D12)
            {
                if (tiled_rsc)
                {
                    PRINTLINE("Tiled Resources", tiled_rsc);
                }
            }

            if (pD3D11_2 || pD3D11_3)
            {
                if (minmaxfilter)
                {
                    PRINTLINE("Min/Max Filtering", minmaxfilter);
                }

                if (mapdefaultbuff)
                {
                    PRINTLINE("Map DEFAULT Buffers", mapdefaultbuff);
                }
            }

            if (pD3D11_3 || pD3D12)
            {
                if (consrv_rast)
                {
                    PRINTLINE("Conservative Rasterization", consrv_rast);
                }

                if (ps_stencil_ref)
                {
                    PRINTLINE("PS-Specified Stencil Ref ", ps_stencil_ref);
                }

                if (rast_ordered_views)
                {
                    PRINTLINE("Rasterizer Ordered Views", rast_ordered_views);
                }
            }

            if (pD3D12 && binding_rsc)
            {
                PRINTLINE("Resource Binding", binding_rsc);
            }

            if (g_DXGIFactory1 && !pD3D12)
            {
                PRINTLINE("Extended Formats (BGRA, etc.)", extFormats);

                if (x2_10BitFormat)
                {
                    PRINTLINE("10-bit XR High Color Format", x2_10BitFormat);
                }
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                PRINTLINE("16-bit Formats (565/5551/4444)", bpp16);
            }

            if (nonpow2)
            {
                PRINTLINE("Non-Power-of-2 Textures", nonpow2);
            }

            PRINTLINE("Max Texture Dimension", maxTexDim);
            PRINTLINE("Max Cubemap Dimension", maxCubeDim);

            if (maxVolDim)
            {
                PRINTLINE("Max Volume Extent", maxVolDim);
            }

            if (maxTexRepeat)
            {
                PRINTLINE("Max Texture Repeat", maxTexRepeat);
            }

            PRINTLINE("Max Input Slots", maxInputSlots);

            if (uavSlots)
            {
                PRINTLINE("UAV Slots", uavSlots);
            }

            if (maxAnisotropy)
            {
                PRINTLINE("Max Anisotropy", maxAnisotropy);
            }

            PRINTLINE("Max Primitive Count", maxPrimCount);

            if (mrt)
            {
                PRINTLINE("Simultaneous Render Targets", mrt);
            }

            if (_10level9)
            {
                PRINTYESNO("Occlusion Queries", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO("Separate Alpha Blend", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO("Mirror Once", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO("Overlapping Vertex Elements", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO("Independant Write Masks", (fl >= D3D_FEATURE_LEVEL_9_3));

                PRINTLINE("Instancing", instancing);

                if (pD3D11_1 || pD3D11_2 || pD3D11_3)
                {
                    PRINTLINE("Shadow Support", shadows);
                }

                if (pD3D11_2 || pD3D11_3)
                {
                    PRINTLINE("Cubemap Render w/ non-Cube Depth", cubeRT);
                }
            }

            PRINTLINE("Note", FL_NOTE);
        }

        return S_OK;
    }
  

    //-----------------------------------------------------------------------------
    HRESULT D3D10Info(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D10Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);

            if (lParam2 == D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, "Value", 25);
                LVAddColumn(g_hwndLV, 2, "Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, "Value", 60);
            }
        }

        if (!lParam2)
        {
            // General Direct3D 10.0 device information

            BOOL ext, x2;
            CheckExtendedFormats(pDevice, ext, x2);

            if (!pPrintInfo)
            {
                if (g_DXGIFactory1)
                {
                    LVYESNO("Extended Formats (BGRA, etc.)", ext);
                    LVYESNO("10-bit XR High Color Format", x2);
                }

                LVLINE("Note", D3D10_NOTE);
            }
            else
            {
                if (g_DXGIFactory1)
                {
                    PRINTYESNO("Extended Formats (BGRA, etc.)", ext);
                    PRINTYESNO("10-bit XR High Color Format", x2);
                }

                PRINTLINE("Note", D3D10_NOTE);
            }

            return S_OK;
        }

        // Use CheckFormatSupport for format usage defined as optional for Direct3D 10 devices
        static const DXGI_FORMAT cfsShaderSample[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R24_UNORM_X8_TYPELESS
        };

        static const DXGI_FORMAT cfsMipAutoGen[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsRenderTarget[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT
        };

        static const DXGI_FORMAT cfsBlendableRT[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R16_UNORM
        };

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;
        BOOL skips = FALSE;

        switch (lParam2)
        {
        case D3D10_FORMAT_SUPPORT_SHADER_SAMPLE:
            count = sizeof(cfsShaderSample) / sizeof(DXGI_FORMAT);
            array = cfsShaderSample;
            break;

        case D3D10_FORMAT_SUPPORT_MIP_AUTOGEN:
            count = sizeof(cfsMipAutoGen) / sizeof(DXGI_FORMAT);
            array = cfsMipAutoGen;
            break;

        case D3D10_FORMAT_SUPPORT_RENDER_TARGET:
            count = sizeof(cfsRenderTarget) / sizeof(DXGI_FORMAT);
            array = cfsRenderTarget;
            break;

        case D3D10_FORMAT_SUPPORT_BLENDABLE:
            count = sizeof(cfsBlendableRT) / sizeof(DXGI_FORMAT);
            array = cfsBlendableRT;
            break;

        case D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET:
            count = sizeof(g_cfsMSAA_10) / sizeof(DXGI_FORMAT);
            array = g_cfsMSAA_10;
            break;

        case D3D10_FORMAT_SUPPORT_MULTISAMPLE_LOAD:
            count = sizeof(g_cfsMSAA_10) / sizeof(DXGI_FORMAT);
            array = g_cfsMSAA_10;
            skips = TRUE;
            break;

        default:
            return E_FAIL;
        }

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = array[i];

            if (skips)
            {
                // Special logic to let us reuse the MSAA array twice with a few special cases that are skipped
                switch (fmt)
                {
                case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                case DXGI_FORMAT_D32_FLOAT:
                case DXGI_FORMAT_D24_UNORM_S8_UINT:
                case DXGI_FORMAT_D16_UNORM:
                    continue;
                }
            }

            if (lParam2 == D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(fmt, (UINT)lParam3, &quality);

                BOOL msaa = (SUCCEEDED(hr) && quality > 0) != 0;

                if (!pPrintInfo)
                {
                    if (g_dwViewState == IDM_VIEWALL || msaa)
                    {
                        LVAddText(g_hwndLV, 0, FormatName(fmt));
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo); \

                            TCHAR strBuffer[16];
                        sprintf_s(strBuffer, 16, "%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    TCHAR strBuffer[32];
                    if (msaa)
                        sprintf_s(strBuffer, 32, "Yes (%u)", quality);
                    else
                        strcpy_s(strBuffer, 32, c_szNo);

                    PrintStringValueLine(FormatName(fmt), strBuffer, pPrintInfo);
                }
            }
            else
            {
                UINT fmtSupport = 0;
                HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
                if (FAILED(hr))
                    fmtSupport = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
            }
        }

        return S_OK;
    }

    HRESULT D3D10Info1(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D10Device1*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);

            if (lParam2 == D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, "Value", 25);
                LVAddColumn(g_hwndLV, 2, "Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, "Value", 60);
            }
        }

        if (!lParam2)
        {
            // General Direct3D 10.1 device information
            D3D10_FEATURE_LEVEL1 fl = pDevice->GetFeatureLevel();

            const char* szNote = nullptr;

            switch (fl)
            {
            case D3D10_FEATURE_LEVEL_10_0:
                szNote = SEE_D3D10;
                break;

            case D3D10_FEATURE_LEVEL_9_3:
            case D3D10_FEATURE_LEVEL_9_2:
            case D3D10_FEATURE_LEVEL_9_1:
                szNote = _10L9_NOTE;
                break;

            default:
                szNote = D3D10_NOTE1;
                break;
            }

            BOOL ext, x2;
            CheckExtendedFormats(pDevice, ext, x2);

            if (!pPrintInfo)
            {
                LVLINE("Feature Level", FLName(fl));

                    if (g_DXGIFactory1 != nullptr && (fl >= D3D10_FEATURE_LEVEL_10_0))
                    {
                        LVYESNO("Extended Formats (BGRA, etc.)", ext);
                        LVYESNO("10-bit XR High Color Format", x2);
                    }

                    LVLINE("Note", szNote);
            }
            else
            {
                PRINTLINE("Feature Level", FLName(fl));

                    if (g_DXGIFactory1 != nullptr && (fl >= D3D10_FEATURE_LEVEL_10_0))
                    {
                        PRINTYESNO("Extended Formats (BGRA, etc.)", ext);
                        PRINTYESNO("10-bit XR High Color Format", x2);
                    }

                    PRINTLINE("Note", szNote);
            }

            return S_OK;
        }

        // Use CheckFormatSupport for format usage defined as optional for Direct3D 10.1 devices
        static const DXGI_FORMAT cfsShaderSample[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsMipAutoGen[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsRenderTarget[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT
        };

        // Most RT formats are required to support 4x MSAA for 10.1 devices
        static const DXGI_FORMAT cfsMSAA4x[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT,
        };

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;

        switch (lParam2)
        {
        case D3D10_FORMAT_SUPPORT_SHADER_SAMPLE:
            count = sizeof(cfsShaderSample) / sizeof(DXGI_FORMAT);
            array = cfsShaderSample;
            break;

        case D3D10_FORMAT_SUPPORT_MIP_AUTOGEN:
            count = sizeof(cfsMipAutoGen) / sizeof(DXGI_FORMAT);
            array = cfsMipAutoGen;
            break;

        case D3D10_FORMAT_SUPPORT_RENDER_TARGET:
            count = sizeof(cfsRenderTarget) / sizeof(DXGI_FORMAT);
            array = cfsRenderTarget;
            break;

        case D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET:
            if (pDevice->GetFeatureLevel() <= D3D10_FEATURE_LEVEL_9_3)
            {
                count = sizeof(g_cfsMSAA_10level9) / sizeof(DXGI_FORMAT);
                array = g_cfsMSAA_10level9;
            }
            else if (g_dwViewState != IDM_VIEWALL && lParam3 == 4)
            {
                count = sizeof(cfsMSAA4x) / sizeof(DXGI_FORMAT);
                array = cfsMSAA4x;
            }
            else
            {
                count = sizeof(g_cfsMSAA_10) / sizeof(DXGI_FORMAT);
                array = g_cfsMSAA_10;
            }
            break;

        default:
            return E_FAIL;
        }

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = array[i];

            if (lParam2 == D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(fmt, (UINT)lParam3, &quality);

                BOOL msaa = (SUCCEEDED(hr) && quality > 0) != 0;

                if (!pPrintInfo)
                {
                    if (g_dwViewState == IDM_VIEWALL || msaa)
                    {
                        LVAddText(g_hwndLV, 0, FormatName(fmt));
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo); \

                            TCHAR strBuffer[16];
                        sprintf_s(strBuffer, 16, "%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    TCHAR strBuffer[32];
                    if (msaa)
                        sprintf_s(strBuffer, 32, "Yes (%u)", quality);
                    else
                        strcpy_s(strBuffer, 32, c_szNo);

                    PrintStringValueLine(FormatName(fmt), strBuffer, pPrintInfo);
                }
            }
            else
            {
                UINT fmtSupport = 0;
                HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
                if (FAILED(hr))
                    fmtSupport = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), fmtSupport& (UINT)lParam2);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
            }
        }

        return S_OK;
    }


    HRESULT D3D10InfoMSAA(LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D10Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        auto sampCount = reinterpret_cast<BOOL*>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);

            UINT column = 1;
            for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
            {
                if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                    continue;

                TCHAR strBuffer[8];
                sprintf_s(strBuffer, 8, "%ux", samples);
                LVAddColumn(g_hwndLV, column, strBuffer, 8);
                ++column;
            }
        }

        const UINT count = sizeof(g_cfsMSAA_10) / sizeof(DXGI_FORMAT);

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = g_cfsMSAA_10[i];

            UINT sampQ[D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT];
            memset(sampQ, 0, sizeof(sampQ));

            BOOL any = FALSE;
            for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
            {
                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(fmt, samples, &quality);

                if (SUCCEEDED(hr) && quality > 0)
                {
                    sampQ[samples - 1] = quality;
                    any = TRUE;
                }
            }

            if (!pPrintInfo)
            {
                if (g_dwViewState != IDM_VIEWALL && !any)
                    continue;

                LVAddText(g_hwndLV, 0, FormatName(fmt));

                UINT column = 1;
                for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
                {
                    if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                        continue;

                    if (sampQ[samples - 1] > 0)
                    {
                        TCHAR strBuffer[32];
                        sprintf_s(strBuffer, 32, "Yes (%u)", sampQ[samples - 1]);
                        LVAddText(g_hwndLV, column, strBuffer);
                    }
                    else
                        LVAddText(g_hwndLV, column, c_szNo);

                    ++column;
                }
            }
            else
            {
                TCHAR buff[1024];
                *buff = 0;

                for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
                {
                    if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                        continue;

                    TCHAR strBuffer[32];
                    if (sampQ[samples - 1] > 0)
                        sprintf_s(strBuffer, 32, "%ux: Yes (%u)   ", samples, sampQ[samples - 1]);
                    else
                        sprintf_s(strBuffer, 32, "%ux: No   ", samples);

                    strcat_s(buff, 1024, strBuffer);
                }

                PrintStringValueLine(FormatName(fmt), buff, pPrintInfo);
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    HRESULT D3D11Info(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D11Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);

            if (lParam2 == D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, "Value", 25);
                LVAddColumn(g_hwndLV, 2, "Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, "Value", 60);
            }
        }

        if (!lParam2)
        {
            // General Direct3D 11 device information
            D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
            if (fl > D3D_FEATURE_LEVEL_11_0)
                fl = D3D_FEATURE_LEVEL_11_0;

            // CheckFeatureSupport
            D3D11_FEATURE_DATA_THREADING threading = {};
            HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threading, sizeof(threading));
            if (FAILED(hr))
                memset(&threading, 0, sizeof(threading));

            D3D11_FEATURE_DATA_DOUBLES doubles = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &doubles, sizeof(doubles));
            if (FAILED(hr))
                memset(&doubles, 0, sizeof(doubles));

            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
            if (FAILED(hr))
                memset(&d3d10xhw, 0, sizeof(d3d10xhw));

            // Setup note
            const char* szNote = nullptr;

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_10_0:
                szNote = SEE_D3D10;
                break;

            case D3D_FEATURE_LEVEL_10_1:
            case D3D_FEATURE_LEVEL_9_3:
            case D3D_FEATURE_LEVEL_9_2:
            case D3D_FEATURE_LEVEL_9_1:
                szNote = SEE_D3D10_1;
                break;

            default:
                szNote = D3D11_NOTE;
                break;
            }

            if (!pPrintInfo)
            {
                LVLINE("Feature Level", FLName(fl));

                LVYESNO("Driver Concurrent Creates", threading.DriverConcurrentCreates);
                LVYESNO("Driver Command Lists", threading.DriverCommandLists);
                LVYESNO("Double-precision Shaders", doubles.DoublePrecisionFloatShaderOps);
                LVYESNO("DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

                    LVLINE("Note", szNote);
            }
            else
            {
                PRINTLINE("Feature Level", FLName(fl));

                PRINTYESNO("Driver Concurrent Creates", threading.DriverConcurrentCreates);
                PRINTYESNO("Driver Command Lists", threading.DriverCommandLists);
                PRINTYESNO("Double-precision Shaders", doubles.DoublePrecisionFloatShaderOps);
                PRINTYESNO("DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

                    PRINTLINE("Note", szNote);
            }

            return S_OK;
        }

        // Use CheckFormatSupport for format usage defined as optional for Direct3D 11 devices
        static const DXGI_FORMAT cfsShaderSample[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsShaderGather[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsMipAutoGen[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsRenderTarget[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT
        };

        // Most RT formats are required to support 8x MSAA for 11 devices
        static const DXGI_FORMAT cfsMSAA8x[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32A32_SINT
        };

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;

        switch (lParam2)
        {
        case D3D11_FORMAT_SUPPORT_SHADER_SAMPLE:
            count = sizeof(cfsShaderSample) / sizeof(DXGI_FORMAT);
            array = cfsShaderSample;
            break;

        case D3D11_FORMAT_SUPPORT_SHADER_GATHER:
            count = sizeof(cfsShaderGather) / sizeof(DXGI_FORMAT);
            array = cfsShaderGather;
            break;

        case D3D11_FORMAT_SUPPORT_MIP_AUTOGEN:
            count = sizeof(cfsMipAutoGen) / sizeof(DXGI_FORMAT);
            array = cfsMipAutoGen;
            break;

        case D3D11_FORMAT_SUPPORT_RENDER_TARGET:
            count = sizeof(cfsRenderTarget) / sizeof(DXGI_FORMAT);
            array = cfsRenderTarget;
            break;

        case D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET:
            if (g_dwViewState != IDM_VIEWALL && lParam3 == 8)
            {
                count = sizeof(cfsMSAA8x) / sizeof(DXGI_FORMAT);
                array = cfsMSAA8x;
            }
            else if (g_dwViewState != IDM_VIEWALL && lParam3 == 4)
            {
                count = 0;
            }
            else
            {
                count = static_cast<UINT>(std::size(g_cfsMSAA_11));
                array = g_cfsMSAA_11;
            }
            break;

        default:
            return E_FAIL;
        }

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = array[i];

            if (lParam2 == D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                // Skip 16 bits-per-pixel formats for DX 11.0
                if (fmt == DXGI_FORMAT_B5G6R5_UNORM || fmt == DXGI_FORMAT_B5G5R5A1_UNORM || fmt == DXGI_FORMAT_B4G4R4A4_UNORM)
                    continue;

                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(fmt, (UINT)lParam3, &quality);

                BOOL msaa = (SUCCEEDED(hr) && quality > 0) != 0;

                if (!pPrintInfo)
                {
                    if (g_dwViewState == IDM_VIEWALL || msaa)
                    {
                        LVAddText(g_hwndLV, 0, FormatName(fmt));
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo); \

                            TCHAR strBuffer[16];
                        sprintf_s(strBuffer, 16, "%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    TCHAR strBuffer[32];
                    if (msaa)
                        sprintf_s(strBuffer, 32, "Yes (%u)", quality);
                    else
                        strcpy_s(strBuffer, 32, c_szNo);

                    PrintStringValueLine(FormatName(fmt), strBuffer, pPrintInfo);
                }
            }
            else
            {
                UINT fmtSupport = 0;
                HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
                if (FAILED(hr))
                    fmtSupport = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
            }
        }

        return S_OK;
    }

    void D3D11FeatureSupportInfo1(ID3D11Device1* pDevice, bool bDev2, PRINTCBINFO* pPrintInfo)
    {
        if (!pDevice)
            return;

        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        // CheckFeatureSupport
        D3D11_FEATURE_DATA_THREADING threading = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threading, sizeof(threading));
        if (FAILED(hr))
            memset(&threading, 0, sizeof(threading));

        D3D11_FEATURE_DATA_DOUBLES doubles = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &doubles, sizeof(doubles));
        if (FAILED(hr))
            memset(&doubles, 0, sizeof(doubles));

        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
        if (FAILED(hr))
            memset(&d3d10xhw, 0, sizeof(d3d10xhw));

        D3D11_FEATURE_DATA_D3D11_OPTIONS d3d11opts = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &d3d11opts, sizeof(d3d11opts));
        if (FAILED(hr))
            memset(&d3d11opts, 0, sizeof(d3d11opts));

        D3D11_FEATURE_DATA_D3D9_OPTIONS d3d9opts = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D9_OPTIONS, &d3d9opts, sizeof(d3d9opts));
        if (FAILED(hr))
            memset(&d3d9opts, 0, sizeof(d3d9opts));

        D3D11_FEATURE_DATA_ARCHITECTURE_INFO d3d11arch = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_ARCHITECTURE_INFO, &d3d11arch, sizeof(d3d11arch));
        if (FAILED(hr))
            memset(&d3d11arch, 0, sizeof(d3d11arch));

        D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT minprecis = {};
        hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &minprecis, sizeof(minprecis));
        if (FAILED(hr))
            memset(&minprecis, 0, sizeof(minprecis));

        D3D11_FEATURE_DATA_D3D11_OPTIONS1 d3d11opts1 = {};
        memset(&d3d11opts1, 0, sizeof(d3d11opts1));
        if (bDev2)
        {
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &d3d11opts1, sizeof(d3d11opts1));
            if (FAILED(hr))
                memset(&d3d11opts1, 0, sizeof(d3d11opts1));
        }

        const char* clearview = nullptr;
        if (d3d11opts.ClearView)
        {
            clearview = d3d11opts1.ClearViewAlsoSupportsDepthOnlyFormats ? "RTV, UAV, and Depth (Driver sees)" : "RTV and UAV (Driver sees)";
        }
        else
        {
            clearview = d3d11opts1.ClearViewAlsoSupportsDepthOnlyFormats ? "RTV, UAV, and Depth (Driver doesn't see)" : "RTV and UAV (Driver doesn't see)";
        }

        const char* double_shaders = nullptr;
        if (d3d11opts.ExtendedDoublesShaderInstructions)
        {
            double_shaders = "Extended";
        }
        else if (doubles.DoublePrecisionFloatShaderOps)
        {
            double_shaders = c_szYes;
        }
        else
        {
            double_shaders = c_szNo;
        }

        const char* ps_precis = nullptr;
        switch (minprecis.PixelShaderMinPrecision & (D3D11_SHADER_MIN_PRECISION_16_BIT | D3D11_SHADER_MIN_PRECISION_10_BIT))
        {
        case 0:                                 ps_precis = "Full";         break;
        case D3D11_SHADER_MIN_PRECISION_16_BIT: ps_precis = "16/32-bit";    break;
        case D3D11_SHADER_MIN_PRECISION_10_BIT: ps_precis = "10/32-bit";    break;
        default:                                ps_precis = "10/16/32-bit"; break;
        }

        const char* other_precis = nullptr;
        switch (minprecis.AllOtherShaderStagesMinPrecision & (D3D11_SHADER_MIN_PRECISION_16_BIT | D3D11_SHADER_MIN_PRECISION_10_BIT))
        {
        case 0:                                 other_precis = "Full";      break;
        case D3D11_SHADER_MIN_PRECISION_16_BIT: other_precis = "16/32-bit"; break;
        case D3D11_SHADER_MIN_PRECISION_10_BIT: other_precis = "10/32-bit"; break;
        default:                                other_precis = "10/16/32-bit"; break;
        }

        const char* nonpow2 = (d3d9opts.FullNonPow2TextureSupport) ? "Full" : "Conditional";

        if (!pPrintInfo)
        {
            LVLINE("Feature Level", FLName(fl));

            LVYESNO("Driver Concurrent Creates", threading.DriverConcurrentCreates);
            LVYESNO("Driver Command Lists", threading.DriverCommandLists);
            LVLINE("Double-precision Shaders", double_shaders);
            LVYESNO("DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

            LVYESNO("Driver sees DiscardResource/View", d3d11opts.DiscardAPIsSeenByDriver);
            LVYESNO("Driver sees COPY_FLAGS", d3d11opts.FlagsForUpdateAndCopySeenByDriver);
            LVLINE("ClearView", clearview);
            LVYESNO("Copy w/ Overlapping Rect", d3d11opts.CopyWithOverlap);
            LVYESNO("CB Partial Update", d3d11opts.ConstantBufferPartialUpdate);
            LVYESNO("CB Offsetting", d3d11opts.ConstantBufferOffsetting);
            LVYESNO("Map NO_OVERWRITE on Dynamic CB", d3d11opts.MapNoOverwriteOnDynamicConstantBuffer);

            if (fl >= D3D_FEATURE_LEVEL_10_0)
            {
                LVYESNO("Map NO_OVERWRITE on Dynamic SRV", d3d11opts.MapNoOverwriteOnDynamicBufferSRV);
                LVYESNO("MSAA with ForcedSampleCount=1", d3d11opts.MultisampleRTVWithForcedSampleCountOne);
                    LVYESNO("Extended resource sharing", d3d11opts.ExtendedResourceSharing);
            }

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                LVYESNO("Saturating Add Instruction", d3d11opts.SAD4ShaderInstructions);
            }

            LVYESNO("Tile-based Deferred Renderer", d3d11arch.TileBasedDeferredRenderer);

                LVLINE("Non-Power-of-2 Textures", nonpow2);

            LVLINE("Pixel Shader Precision", ps_precis);
            LVLINE("Other Stage Precision", other_precis);
        }
        else
        {
            PRINTLINE("Feature Level", FLName(fl));

            PRINTYESNO("Driver Concurrent Creates", threading.DriverConcurrentCreates);
            PRINTYESNO("Driver Command Lists", threading.DriverCommandLists);
            PRINTLINE("Double-precision Shaders", double_shaders);
            PRINTYESNO("DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

            PRINTYESNO("Driver sees DiscardResource/View", d3d11opts.DiscardAPIsSeenByDriver);
            PRINTYESNO("Driver sees COPY_FLAGS", d3d11opts.FlagsForUpdateAndCopySeenByDriver);
            PRINTLINE("ClearView", clearview);
            PRINTYESNO("Copy w/ Overlapping Rect", d3d11opts.CopyWithOverlap);
            PRINTYESNO("CB Partial Update", d3d11opts.ConstantBufferPartialUpdate);
            PRINTYESNO("CB Offsetting", d3d11opts.ConstantBufferOffsetting);
            PRINTYESNO("Map NO_OVERWRITE on Dynamic CB", d3d11opts.MapNoOverwriteOnDynamicConstantBuffer);

            if (fl >= D3D_FEATURE_LEVEL_10_0)
            {
                PRINTYESNO("Map NO_OVERWRITE on Dynamic SRV", d3d11opts.MapNoOverwriteOnDynamicBufferSRV);
                PRINTYESNO("MSAA with ForcedSampleCount=1", d3d11opts.MultisampleRTVWithForcedSampleCountOne);
                PRINTYESNO("Extended resource sharing", d3d11opts.ExtendedResourceSharing);
            }

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                PRINTYESNO("Saturating Add Instruction", d3d11opts.SAD4ShaderInstructions);
            }

            PRINTYESNO("Tile-based Deferred Renderer", d3d11arch.TileBasedDeferredRenderer);

            PRINTLINE("Non-Power-of-2 Textures", nonpow2);

            PRINTLINE("Pixel Shader Precision", ps_precis);
            PRINTLINE("Other Stage Precision", other_precis);
        }
    }

    HRESULT D3D11Info1(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D11Device1*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);

            if (lParam2 == D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, "Value", 25);
                LVAddColumn(g_hwndLV, 2, "Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, "Value", 60);
            }
        }

        // General Direct3D 11.1 device information
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        if (!lParam2)
        {
            D3D11FeatureSupportInfo1(pDevice, false, pPrintInfo);

            // Setup note
            const char* szNote = nullptr;

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_10_0:
                szNote = SEE_D3D10;
                break;

            case D3D_FEATURE_LEVEL_10_1:
            case D3D_FEATURE_LEVEL_9_3:
            case D3D_FEATURE_LEVEL_9_2:
            case D3D_FEATURE_LEVEL_9_1:
                szNote = SEE_D3D10_1;
                break;

            case D3D_FEATURE_LEVEL_11_0:
                szNote = SEE_D3D11;
                break;

            default:
                szNote = D3D11_NOTE1;
                break;
            }

            if (!pPrintInfo)
            {
                LVLINE("Note", szNote);
            }
            else
            {
                PRINTLINE("Note", szNote);
            }

            return S_OK;
        }

        // Use CheckFormatSupport for format usage defined as optional for Direct3D 11.1 devices

        static const DXGI_FORMAT cfs16bpp[] =
        {
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfs16bpp2[] =
        {
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfsShaderSample[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R24_UNORM_X8_TYPELESS
        };

        static const DXGI_FORMAT cfsShaderSample10_1[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsShaderGather[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT
        };

        static const DXGI_FORMAT cfsMipAutoGen[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfsMipAutoGen11_1[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfsRenderTarget[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfsRenderTarget11_1[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfsBlendableRT[] =
        {
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        static const DXGI_FORMAT cfsLogicOps[] =
        {
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R8_UINT,
        };

        // Most RT formats are required to support 8x MSAA for 11.x devices
        static const DXGI_FORMAT cfsMSAA8x[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM,
        };

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;
        BOOL skips = FALSE;

        switch (lParam2)
        {
        case D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER:
            count = sizeof(cfs16bpp) / sizeof(DXGI_FORMAT);
            array = cfs16bpp;
            break;

        case D3D11_FORMAT_SUPPORT_SHADER_SAMPLE:
            if (fl >= D3D_FEATURE_LEVEL_10_1)
            {
                count = sizeof(cfsShaderSample10_1) / sizeof(DXGI_FORMAT);
                array = cfsShaderSample10_1;
            }
            else
            {
                count = sizeof(cfsShaderSample) / sizeof(DXGI_FORMAT);
                array = cfsShaderSample;
            }
            break;

        case D3D11_FORMAT_SUPPORT_SHADER_GATHER:
            count = sizeof(cfsShaderGather) / sizeof(DXGI_FORMAT);
            array = cfsShaderGather;
            break;

        case D3D11_FORMAT_SUPPORT_MIP_AUTOGEN:
            if (fl >= D3D_FEATURE_LEVEL_11_1)
            {
                count = sizeof(cfsMipAutoGen11_1) / sizeof(DXGI_FORMAT);
                array = cfsMipAutoGen11_1;
            }
            else
            {
                count = sizeof(cfsMipAutoGen) / sizeof(DXGI_FORMAT);
                array = cfsMipAutoGen;
            }
            break;

        case D3D11_FORMAT_SUPPORT_RENDER_TARGET:
            if (fl >= D3D_FEATURE_LEVEL_11_1)
            {
                count = sizeof(cfsRenderTarget11_1) / sizeof(DXGI_FORMAT);
                array = cfsRenderTarget11_1;
            }
            else
            {
                count = sizeof(cfsRenderTarget) / sizeof(DXGI_FORMAT);
                array = cfsRenderTarget;
            }
            break;

        case D3D11_FORMAT_SUPPORT_BLENDABLE:
            if (fl >= D3D_FEATURE_LEVEL_10_0)
            {
                count = sizeof(cfs16bpp2) / sizeof(DXGI_FORMAT);
                array = cfs16bpp2;
            }
            else
            {
                count = sizeof(cfsBlendableRT) / sizeof(DXGI_FORMAT);
                array = cfsBlendableRT;
            }
            break;

        case D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET:
            if (g_dwViewState != IDM_VIEWALL && lParam3 == 8)
            {
                count = sizeof(cfsMSAA8x) / sizeof(DXGI_FORMAT);
                array = cfsMSAA8x;
            }
            else if (g_dwViewState != IDM_VIEWALL && (lParam3 == 2 || lParam3 == 4))
            {
                count = sizeof(cfs16bpp) / sizeof(DXGI_FORMAT);
                array = cfs16bpp;
            }
            else
            {
                count = sizeof(g_cfsMSAA_11) / sizeof(DXGI_FORMAT);
                array = g_cfsMSAA_11;
            }
            break;

        case D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD:
            if (fl >= D3D_FEATURE_LEVEL_10_1)
            {
                count = sizeof(cfs16bpp2) / sizeof(DXGI_FORMAT);
                array = cfs16bpp2;
            }
            else if (fl >= D3D_FEATURE_LEVEL_10_0)
            {
                count = sizeof(cfs16bpp) / sizeof(DXGI_FORMAT);
                array = cfs16bpp;
            }
            else
            {
                count = sizeof(g_cfsMSAA_11) / sizeof(DXGI_FORMAT);
                array = g_cfsMSAA_11;
            }
            skips = TRUE;
            break;

        case D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW:
            count = sizeof(cfs16bpp) / sizeof(DXGI_FORMAT);
            array = cfs16bpp;
            break;

        case LPARAM(-1): // D3D11_FORMAT_SUPPORT2
            switch (lParam3)
            {
            case D3D11_FORMAT_SUPPORT2_OUTPUT_MERGER_LOGIC_OP:
                count = sizeof(cfsLogicOps) / sizeof(DXGI_FORMAT);
                array = cfsLogicOps;
                break;

            case D3D11_FORMAT_SUPPORT2_UAV_TYPED_STORE:
                count = sizeof(cfs16bpp) / sizeof(DXGI_FORMAT);
                array = cfs16bpp;
                break;

            default:
                return E_FAIL;
            }
            break;

        default:
            return E_FAIL;
        }

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = array[i];

            if (skips)
            {
                // Special logic to let us reuse the MSAA array twice with a few special cases that are skipped
                switch (fmt)
                {
                case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                case DXGI_FORMAT_D32_FLOAT:
                case DXGI_FORMAT_D24_UNORM_S8_UINT:
                case DXGI_FORMAT_D16_UNORM:
                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                    continue;
                }
            }

            if (lParam2 == D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                if (g_dwViewState != IDM_VIEWALL && fmt == DXGI_FORMAT_B5G6R5_UNORM)
                    continue;

                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(fmt, (UINT)lParam3, &quality);

                BOOL msaa = (SUCCEEDED(hr) && quality > 0) != 0;

                if (!pPrintInfo)
                {
                    if (g_dwViewState == IDM_VIEWALL || msaa)
                    {
                        LVAddText(g_hwndLV, 0, FormatName(fmt));
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo); \

                            TCHAR strBuffer[16];
                        sprintf_s(strBuffer, 16, "%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    TCHAR strBuffer[32];
                    if (msaa)
                        sprintf_s(strBuffer, 32, "Yes (%u)", quality);
                    else
                        strcpy_s(strBuffer, 32, c_szNo);

                    PrintStringValueLine(FormatName(fmt), strBuffer, pPrintInfo);
                }
            }
            else if (lParam2 != LPARAM(-1))
            {
                UINT fmtSupport = 0;
                HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
                if (FAILED(hr))
                    fmtSupport = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
            }
            else
            {
                D3D11_FEATURE_DATA_FORMAT_SUPPORT2 cfs2;
                cfs2.InFormat = fmt;
                cfs2.OutFormatSupport2 = 0;
                HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &cfs2, sizeof(cfs2));
                if (FAILED(hr))
                    cfs2.OutFormatSupport2 = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), cfs2.OutFormatSupport2 & (UINT)lParam3);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), cfs2.OutFormatSupport2 & (UINT)lParam3);
                }
            }
        }

        return S_OK;
    }

    HRESULT D3D11Info2(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D11Device2*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        // General Direct3D 11.2 device information
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        if (!lParam2)
        {
            D3D11FeatureSupportInfo1(pDevice, true, pPrintInfo);

            D3D11_FEATURE_DATA_MARKER_SUPPORT marker = {};
            HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_MARKER_SUPPORT, &marker, sizeof(marker));
            if (FAILED(hr))
                memset(&marker, 0, sizeof(marker));

            // Setup note
            const char* szNote = nullptr;

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_10_0:
                szNote = SEE_D3D10;
                break;

            case D3D_FEATURE_LEVEL_10_1:
            case D3D_FEATURE_LEVEL_9_3:
            case D3D_FEATURE_LEVEL_9_2:
            case D3D_FEATURE_LEVEL_9_1:
                szNote = SEE_D3D10_1;
                break;

            case D3D_FEATURE_LEVEL_11_0:
                szNote = SEE_D3D11;
                break;

            case D3D_FEATURE_LEVEL_11_1:
                szNote = SEE_D3D11_1;
                break;

            default:
                szNote = D3D11_NOTE2;
                break;
            }

            if (!pPrintInfo)
            {
                LVYESNO("Profile Marker Support", marker.Profile);

                LVLINE("Note", szNote);
            }
            else
            {
                PRINTYESNO("Profile Marker Support", marker.Profile);

                PRINTLINE("Note", szNote);
            }

            return S_OK;
        }

        // Use CheckFormatSupport for format usage defined as optional for Direct3D 11.2 devices

        static const DXGI_FORMAT cfsShareable[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R16G16B16A16_SINT,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R8G8B8A8_SINT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R32_SINT,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R16_SINT,
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8_UINT,
            DXGI_FORMAT_R8_SNORM,
            DXGI_FORMAT_R8_SINT,
            DXGI_FORMAT_A8_UNORM,
            DXGI_FORMAT_BC1_UNORM,
            DXGI_FORMAT_BC1_UNORM_SRGB,
            DXGI_FORMAT_BC2_UNORM,
            DXGI_FORMAT_BC2_UNORM_SRGB,
            DXGI_FORMAT_BC3_UNORM,
            DXGI_FORMAT_BC3_UNORM_SRGB,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_B8G8R8X8_UNORM,
            DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
        };

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;

        switch (lParam2)
        {
        case LPARAM(-1): // D3D11_FORMAT_SUPPORT2
            switch (lParam3)
            {
            case D3D11_FORMAT_SUPPORT2_SHAREABLE:
                count = static_cast<UINT>(std::size(cfsShareable));
                array = cfsShareable;
                break;

            default:
                return E_FAIL;
            }
            break;

        default:
            return E_FAIL;
        }

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = array[i];

            if (lParam2 != LPARAM(-1))
            {
                UINT fmtSupport = 0;
                HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
                if (FAILED(hr))
                    fmtSupport = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
            }
            else
            {
                D3D11_FEATURE_DATA_FORMAT_SUPPORT2 cfs2;
                cfs2.InFormat = fmt;
                cfs2.OutFormatSupport2 = 0;
                HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &cfs2, sizeof(cfs2));
                if (FAILED(hr))
                    cfs2.OutFormatSupport2 = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), cfs2.OutFormatSupport2 & (UINT)lParam3);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), cfs2.OutFormatSupport2 & (UINT)lParam3);
                }
            }
        }

        return S_OK;
    }

    HRESULT D3D11Info3(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        ID3D11Device3* pDevice = reinterpret_cast<ID3D11Device3*>(lParam1);

        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        // General Direct3D 11.3/11.4 device information
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        if (!lParam2)
        {
            D3D11FeatureSupportInfo1(pDevice, true, pPrintInfo);

            D3D11_FEATURE_DATA_MARKER_SUPPORT marker = {};
            HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_MARKER_SUPPORT, &marker, sizeof(marker));
            if (FAILED(hr))
                memset(&marker, 0, sizeof(marker));

            D3D11_FEATURE_DATA_D3D11_OPTIONS2 d3d11opts2 = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &d3d11opts2, sizeof(d3d11opts2));
            if (FAILED(hr))
                memset(&d3d11opts2, 0, sizeof(d3d11opts2));

            D3D11_FEATURE_DATA_D3D11_OPTIONS3 d3d11opts3 = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &d3d11opts3, sizeof(d3d11opts3));
            if (FAILED(hr))
                memset(&d3d11opts3, 0, sizeof(d3d11opts3));

            D3D11_FEATURE_DATA_D3D11_OPTIONS4 d3d11opts4 = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &d3d11opts4, sizeof(d3d11opts4));
            if (FAILED(hr))
                memset(&d3d11opts4, 0, sizeof(d3d11opts4));

            D3D11_FEATURE_DATA_D3D11_OPTIONS5 d3d11opts5 = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &d3d11opts5, sizeof(d3d11opts5));
            if (FAILED(hr))
                memset(&d3d11opts5, 0, sizeof(d3d11opts5));

            D3D11_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT d3d11vm = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &d3d11vm, sizeof(d3d11vm));
            if (FAILED(hr))
                memset(&d3d11vm, 0, sizeof(d3d11vm));

            D3D11_FEATURE_DATA_SHADER_CACHE d3d11sc = {};
            hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_SHADER_CACHE, &d3d11sc, sizeof(d3d11sc));
            if (FAILED(hr))
                memset(&d3d11sc, 0, sizeof(d3d11sc));

            // Setup note
            const char* szNote = nullptr;

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_10_0:
                szNote = SEE_D3D10;
                break;

            case D3D_FEATURE_LEVEL_10_1:
            case D3D_FEATURE_LEVEL_9_3:
            case D3D_FEATURE_LEVEL_9_2:
            case D3D_FEATURE_LEVEL_9_1:
                szNote = SEE_D3D10_1;
                break;

            case D3D_FEATURE_LEVEL_11_0:
                szNote = SEE_D3D11;
                break;

            case D3D_FEATURE_LEVEL_11_1:
                szNote = SEE_D3D11_1;
                break;

            default:
                szNote = (lParam3 != 0) ? D3D11_NOTE4 : D3D11_NOTE3;
                break;
            }

            char tiled_rsc[16];
            if (d3d11opts2.TiledResourcesTier == D3D11_TILED_RESOURCES_NOT_SUPPORTED)
                strcpy_s(tiled_rsc, "Not supported");
            else
                sprintf_s(tiled_rsc, "Tier %d", d3d11opts2.TiledResourcesTier);

            char consrv_rast[16];
            if (d3d11opts2.ConservativeRasterizationTier == D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED)
                strcpy_s(consrv_rast, "Not supported");
            else
                sprintf_s(consrv_rast, "Tier %d", d3d11opts2.ConservativeRasterizationTier);

            const char* sharedResTier = nullptr;
            switch (d3d11opts5.SharedResourceTier)
            {
            case D3D11_SHARED_RESOURCE_TIER_0: sharedResTier = "Yes - Tier 0"; break;
            case D3D11_SHARED_RESOURCE_TIER_1: sharedResTier = "Yes - Tier 1"; break;
            case D3D11_SHARED_RESOURCE_TIER_2: sharedResTier = "Yes - Tier 2"; break;
            case D3D11_SHARED_RESOURCE_TIER_3: sharedResTier = "Yes - Tier 3"; break;
            default: sharedResTier = c_szYes; break;
            }

            char vmRes[16];
            sprintf_s(vmRes, 16, "%u", d3d11vm.MaxGPUVirtualAddressBitsPerResource);

            char vmProcess[16];
            sprintf_s(vmProcess, 16, "%u", d3d11vm.MaxGPUVirtualAddressBitsPerProcess);

            char shaderCache[32] = {};
            if (d3d11sc.SupportFlags)
            {
                if (d3d11sc.SupportFlags & D3D11_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE)
                {
                    strcat_s(shaderCache, "In-Proc ");
                }
                if (d3d11sc.SupportFlags & D3D11_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE)
                {
                    strcat_s(shaderCache, "Disk ");
                }
            }
            else
            {
                strcpy_s(shaderCache, "None");
            }

            if (!pPrintInfo)
            {
                LVYESNO("Profile Marker Support", marker.Profile);

                LVYESNO("Map DEFAULT Textures", d3d11opts2.MapOnDefaultTextures);
                LVYESNO("Standard Swizzle", d3d11opts2.StandardSwizzle);
                LVYESNO("UMA", d3d11opts2.UnifiedMemoryArchitecture);
                LVYESNO("Extended formats TypedUAVLoad", d3d11opts2.TypedUAVLoadAdditionalFormats);

                LVLINE("Conservative Rasterization", consrv_rast);
                LVLINE("Tiled Resources", tiled_rsc);
                LVYESNO("PS-Specified Stencil Ref", d3d11opts2.PSSpecifiedStencilRefSupported);
                LVYESNO("Rasterizer Ordered Views", d3d11opts2.ROVsSupported);

                LVYESNO("VP/RT f/ Rast-feeding Shader", d3d11opts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer);

                if (lParam3 != 0)
                {
                    LVLINE("Max GPU VM bits per resource", vmRes);
                    LVLINE("Max GPU VM bits per process", vmProcess);

                    LVYESNO("Extended Shared NV12", d3d11opts4.ExtendedNV12SharedTextureSupported);
                    LVLINE("Shader Cache", shaderCache);

                    LVLINE("Shared Resource Tier", sharedResTier);
                }

                LVLINE("Note", szNote);
            }
            else
            {
                PRINTYESNO("Profile Marker Support", marker.Profile);

                PRINTYESNO("Map DEFAULT Textures", d3d11opts2.MapOnDefaultTextures);
                PRINTYESNO("Standard Swizzle", d3d11opts2.StandardSwizzle);
                PRINTYESNO("UMA", d3d11opts2.UnifiedMemoryArchitecture);
                PRINTYESNO("Extended formats TypedUAVLoad", d3d11opts2.TypedUAVLoadAdditionalFormats);

                PRINTLINE("Conservative Rasterization", consrv_rast);
                PRINTLINE("Tiled Resources", tiled_rsc);
                PRINTYESNO("PS-Specified Stencil Ref", d3d11opts2.PSSpecifiedStencilRefSupported);
                PRINTYESNO("Rasterizer Ordered Views", d3d11opts2.ROVsSupported);

                PRINTYESNO("VP/RT f/ Rast-feeding Shader", d3d11opts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer);

                if (lParam3 != 0)
                {
                    PRINTLINE("Max GPU VM bits per resource", vmRes);
                    PRINTLINE("Max GPU VM bits per process", vmProcess);

                    PRINTYESNO("Extended Shared NV12", d3d11opts4.ExtendedNV12SharedTextureSupported);
                    PRINTLINE("Shader Cache", shaderCache);

                    PRINTLINE("Shared Resource Tier", sharedResTier);
                }

                PRINTLINE("Note", szNote);
            }

            return S_OK;
        }

        // Use CheckFormatSupport for format usage defined as optional for Direct3D 11.3/11.4 devices

        static const DXGI_FORMAT cfsUAVTypedLoad[] =
        {
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R16G16B16A16_SINT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R8G8B8A8_SINT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R16G16_SNORM,
            DXGI_FORMAT_R16G16_SINT,
            DXGI_FORMAT_R32_FLOAT, // req
            DXGI_FORMAT_R32_UINT, // req
            DXGI_FORMAT_R32_SINT, // req
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R8G8_SNORM,
            DXGI_FORMAT_R8G8_SINT,
            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R16_SINT,
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8_UINT,
            DXGI_FORMAT_R8_SNORM,
            DXGI_FORMAT_R8_SINT,
            DXGI_FORMAT_A8_UNORM,
        };

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;

        switch (lParam2)
        {
        case LPARAM(-1): // D3D11_FORMAT_SUPPORT2
            switch (lParam3)
            {
            case D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD:
                count = static_cast<UINT>(std::size(cfsUAVTypedLoad));
                array = cfsUAVTypedLoad;
                break;

            default:
                return E_FAIL;
            }
            break;

        default:
            return E_FAIL;
        }

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = array[i];

            if (lParam2 != LPARAM(-1))
            {
                UINT fmtSupport = 0;
                HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
                if (FAILED(hr))
                    fmtSupport = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), fmtSupport & (UINT)lParam2);
                }
            }
            else
            {
                D3D11_FEATURE_DATA_FORMAT_SUPPORT2 cfs2;
                cfs2.InFormat = fmt;
                cfs2.OutFormatSupport2 = 0;
                HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &cfs2, sizeof(cfs2));
                if (FAILED(hr))
                    cfs2.OutFormatSupport2 = 0;

                if (!pPrintInfo)
                {
                    LVYESNO(FormatName(fmt), cfs2.OutFormatSupport2 & (UINT)lParam3);
                }
                else
                {
                    PRINTYESNO(FormatName(fmt), cfs2.OutFormatSupport2 & (UINT)lParam3);
                }
            }
        }

        return S_OK;
    }

    HRESULT D3D11InfoMSAA(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D11Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        auto sampCount = reinterpret_cast<BOOL*>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);

            UINT column = 1;
            for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
            {
                if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                    continue;

                TCHAR strBuffer[8];
                sprintf_s(strBuffer, 8, "%ux", samples);
                LVAddColumn(g_hwndLV, column, strBuffer, 8);
                ++column;
            }
        }

        const UINT count = sizeof(g_cfsMSAA_11) / sizeof(DXGI_FORMAT);

        for (UINT i = 0; i < count; ++i)
        {
            DXGI_FORMAT fmt = g_cfsMSAA_11[i];

            // Skip 16 bits-per-pixel formats for DX 11.0
            if (lParam3 == 0
                && (fmt == DXGI_FORMAT_B5G6R5_UNORM || fmt == DXGI_FORMAT_B5G5R5A1_UNORM || fmt == DXGI_FORMAT_B4G4R4A4_UNORM))
                continue;

            UINT sampQ[D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT];
            memset(sampQ, 0, sizeof(sampQ));

            BOOL any = FALSE;
            for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
            {
                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(fmt, samples, &quality);

                if (SUCCEEDED(hr) && quality > 0)
                {
                    sampQ[samples - 1] = quality;
                    any = TRUE;
                }
            }

            if (!pPrintInfo)
            {
                if (g_dwViewState != IDM_VIEWALL && !any)
                    continue;

                LVAddText(g_hwndLV, 0, FormatName(fmt));

                UINT column = 1;
                for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
                {
                    if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                        continue;

                    if (sampQ[samples - 1] > 0)
                    {
                        TCHAR strBuffer[32];
                        sprintf_s(strBuffer, 32, "Yes (%u)", sampQ[samples - 1]);
                        LVAddText(g_hwndLV, column, strBuffer);
                    }
                    else
                        LVAddText(g_hwndLV, column, c_szNo);

                    ++column;
                }
            }
            else
            {
                TCHAR buff[1024];
                *buff = 0;

                for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
                {
                    if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                        continue;

                    TCHAR strBuffer[32];
                    if (sampQ[samples - 1] > 0)
                        sprintf_s(strBuffer, 32, "%ux: Yes (%u)   ", samples, sampQ[samples - 1]);
                    else
                        sprintf_s(strBuffer, 32, "%ux: No   ", samples);

                    strcat_s(buff, 1024, strBuffer);
                }

                PrintStringValueLine(FormatName(fmt), buff, pPrintInfo);
            }
        }

        return S_OK;
    }

    void FillMSAASampleTable(ID3D10Device* pDevice, BOOL sampCount[], BOOL _10level9)
    {
        memset(sampCount, 0, sizeof(BOOL) * D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT);
        sampCount[0] = TRUE;    // sample count of 1 is always required

        UINT count = 0;
        const DXGI_FORMAT* array = nullptr;

        if (_10level9)
        {
            count = sizeof(g_cfsMSAA_10level9) / sizeof(DXGI_FORMAT);
            array = g_cfsMSAA_10level9;
        }
        else
        {
            count = sizeof(g_cfsMSAA_10) / sizeof(DXGI_FORMAT);
            array = g_cfsMSAA_10;
        }

        for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
        {
            for (UINT i = 0; i < count; ++i)
            {
                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(array[i], samples, &quality);

                if (SUCCEEDED(hr) && quality > 0)
                {
                    sampCount[samples - 1] = TRUE;
                    break;
                }
            }
        }
    }

    void FillMSAASampleTable(ID3D11Device* pDevice, BOOL sampCount[])
    {
        memset(sampCount, 0, sizeof(BOOL) * D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT);
        sampCount[0] = TRUE;    // sample count of 1 is always required

        for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
        {
            const UINT count = sizeof(g_cfsMSAA_11) / sizeof(DXGI_FORMAT);

            for (UINT i = 0; i < count; ++i)
            {
                UINT quality;
                HRESULT hr = pDevice->CheckMultisampleQualityLevels(g_cfsMSAA_11[i], samples, &quality);

                if (SUCCEEDED(hr) && quality > 0)
                {
                    sampCount[samples - 1] = TRUE;
                    break;
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    HRESULT D3D11InfoVideo(LPARAM lParam1, LPARAM /*lParam2*/, LPARAM /*lParam3*/, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D11Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Texture2D", 15);
            LVAddColumn(g_hwndLV, 2, "Input", 15);
            LVAddColumn(g_hwndLV, 3, "Output", 15);
            LVAddColumn(g_hwndLV, 4, "Encoder", 15);
        }

        static const DXGI_FORMAT cfsVideo[] =
        {
            DXGI_FORMAT_NV12,
            DXGI_FORMAT_420_OPAQUE,
            DXGI_FORMAT_YUY2,
            DXGI_FORMAT_AYUV,
            DXGI_FORMAT_Y410,
            DXGI_FORMAT_Y416,
            DXGI_FORMAT_P010,
            DXGI_FORMAT_P016,
            DXGI_FORMAT_Y210,
            DXGI_FORMAT_Y216,
            DXGI_FORMAT_NV11,
            DXGI_FORMAT_AI44,
            DXGI_FORMAT_IA44,
            DXGI_FORMAT_P8,
            DXGI_FORMAT_A8P8,
        };

        for (UINT i = 0; i < std::size(cfsVideo); ++i)
        {
            DXGI_FORMAT fmt = cfsVideo[i];

            UINT fmtSupport = 0;
            HRESULT hr = pDevice->CheckFormatSupport(fmt, &fmtSupport);
            if (FAILED(hr))
                fmtSupport = 0;

            bool any = (fmtSupport & (D3D11_FORMAT_SUPPORT_TEXTURE2D
                | D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_INPUT
                | D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_OUTPUT
                | D3D11_FORMAT_SUPPORT_VIDEO_ENCODER)) ? true : false;

            if (!pPrintInfo)
            {
                if (g_dwViewState != IDM_VIEWALL && !any)
                    continue;

                LVAddText(g_hwndLV, 0, FormatName(fmt));

                LVAddText(g_hwndLV, 1, (fmtSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) ? c_szYes : c_szNo);
                LVAddText(g_hwndLV, 2, (fmtSupport & D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_INPUT) ? c_szYes : c_szNo);
                LVAddText(g_hwndLV, 3, (fmtSupport & D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_OUTPUT) ? c_szYes : c_szNo);
                LVAddText(g_hwndLV, 4, (fmtSupport & D3D11_FORMAT_SUPPORT_VIDEO_ENCODER) ? c_szYes : c_szNo);
            }
            else
            {
                TCHAR buff[1024];

                sprintf_s(buff, "Texture2D: %-3s   Input: %-3s   Output: %-3s   Encoder: %-3s",
                    (fmtSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) ? c_szYes : c_szNo,
                    (fmtSupport & D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_INPUT) ? c_szYes : c_szNo,
                    (fmtSupport & D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_OUTPUT) ? c_szYes : c_szNo,
                    (fmtSupport & D3D11_FORMAT_SUPPORT_VIDEO_ENCODER) ? c_szYes : c_szNo);

                PrintStringValueLine(FormatName(fmt), buff, pPrintInfo);
            }
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    HRESULT D3D12Info(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D12Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        auto fl = static_cast<D3D_FEATURE_LEVEL>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSigOpt = {};
        rootSigOpt.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSigOpt, sizeof(rootSigOpt));
        if (FAILED(hr))
        {
            rootSigOpt.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        if (FAILED(hr))
            memset(&d3d12opts, 0, sizeof(d3d12opts));

        D3D12_FEATURE_DATA_D3D12_OPTIONS2 d3d12opts2 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &d3d12opts2, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS2));
        if (FAILED(hr))
            memset(&d3d12opts2, 0, sizeof(d3d12opts2));

        D3D12_FEATURE_DATA_D3D12_OPTIONS3 d3d12opts3 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &d3d12opts3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
        if (FAILED(hr))
            memset(&d3d12opts3, 0, sizeof(d3d12opts3));

        D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12opts4 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12opts4, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4));
        if (FAILED(hr))
            memset(&d3d12opts4, 0, sizeof(d3d12opts4));

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12opts5 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12opts5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (FAILED(hr))
            memset(&d3d12opts5, 0, sizeof(d3d12opts5));

        D3D12_FEATURE_DATA_D3D12_OPTIONS6 d3d12opts6 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &d3d12opts6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (FAILED(hr))
            memset(&d3d12opts6, 0, sizeof(d3d12opts6));

        D3D12_FEATURE_DATA_SERIALIZATION d3d12serial = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_SERIALIZATION, &d3d12serial, sizeof(D3D12_FEATURE_DATA_SERIALIZATION));
        if (FAILED(hr))
            memset(&d3d12serial, 0, sizeof(d3d12serial));

        const char* shaderModel = "Unknown";
        switch (GetD3D12ShaderModel(pDevice))
        {
        case D3D_SHADER_MODEL_6_7: shaderModel = "6.7"; break;
        case D3D_SHADER_MODEL_6_6: shaderModel = "6.6"; break;
        case D3D_SHADER_MODEL_6_5: shaderModel = "6.5"; break;
        case D3D_SHADER_MODEL_6_4: shaderModel = "6.4"; break;
        case D3D_SHADER_MODEL_6_3: shaderModel = "6.3"; break;
        case D3D_SHADER_MODEL_6_2: shaderModel = "6.2"; break;
        case D3D_SHADER_MODEL_6_1: shaderModel = "6.1"; break;
        case D3D_SHADER_MODEL_6_0: shaderModel = "6.0"; break;
        case D3D_SHADER_MODEL_5_1: shaderModel = "5.1"; break;
        }

        const char* rootSig = "Unknown";
        switch (rootSigOpt.HighestVersion)
        {
        case D3D_ROOT_SIGNATURE_VERSION_1_0: rootSig = "1.0"; break;
        case D3D_ROOT_SIGNATURE_VERSION_1_1: rootSig = "1.1"; break;
        }

        char heap[16];
        sprintf_s(heap, "Tier %d", d3d12opts.ResourceHeapTier);

        char tiled_rsc[64] = {};
        if (d3d12opts.TiledResourcesTier == D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED)
            strcpy_s(tiled_rsc, "Not supported");
        else
        {
            sprintf_s(tiled_rsc, "Tier %d", d3d12opts.TiledResourcesTier);

            if ((d3d12opts.TiledResourcesTier < D3D12_TILED_RESOURCES_TIER_3)
                && d3d12opts5.SRVOnlyTiledResourceTier3)
            {
                strcat_s(tiled_rsc, " (plus SRV only Volume textures)");
            }
        }

        char consrv_rast[16];
        if (d3d12opts.ConservativeRasterizationTier == D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED)
            strcpy_s(consrv_rast, "Not supported");
        else
            sprintf_s(consrv_rast, "Tier %d", d3d12opts.ConservativeRasterizationTier);

        char binding_rsc[16];
        sprintf_s(binding_rsc, "Tier %d", d3d12opts.ResourceBindingTier);

        const char* prgSamplePos = nullptr;
        switch (d3d12opts2.ProgrammableSamplePositionsTier)
        {
        case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED: prgSamplePos = c_szNo; break;
        case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_1: prgSamplePos = "Yes - Tier 1"; break;
        case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_2: prgSamplePos = "Yes - Tier 2"; break;
        default: prgSamplePos = c_szYes; break;
        }

        const char* viewInstTier = nullptr;
        switch (d3d12opts3.ViewInstancingTier)
        {
        case D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED: viewInstTier = c_szNo; break;
        case D3D12_VIEW_INSTANCING_TIER_1: viewInstTier = "Yes - Tier 1"; break;
        case D3D12_VIEW_INSTANCING_TIER_2: viewInstTier = "Yes - Tier 2"; break;
        case D3D12_VIEW_INSTANCING_TIER_3: viewInstTier = "Yes - Tier 3"; break;
        default: viewInstTier = c_szYes; break;
        }

        const char* renderPasses = nullptr;
        switch (d3d12opts5.RenderPassesTier)
        {
        case D3D12_RENDER_PASS_TIER_0: renderPasses = c_szNo; break;
        case D3D12_RENDER_PASS_TIER_1: renderPasses = "Yes - Tier 1"; break;
        case D3D12_RENDER_PASS_TIER_2: renderPasses = "Yes - Tier 2"; break;
        default: renderPasses = c_szYes; break;
        }

        const char* dxr = nullptr;
        switch (d3d12opts5.RaytracingTier)
        {
        case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: dxr = c_szNo; break;
        case D3D12_RAYTRACING_TIER_1_0: dxr = "Yes - Tier 1.0"; break;
        case D3D12_RAYTRACING_TIER_1_1: dxr = "Yes - Tier 1.1"; break;
        default: dxr = c_szYes; break;
        }

        const char* heapSerial = nullptr;
        switch (d3d12serial.HeapSerializationTier)
        {
        case D3D12_HEAP_SERIALIZATION_TIER_0: heapSerial = c_szNo; break;
        case D3D12_HEAP_SERIALIZATION_TIER_10: heapSerial = "Yes - Tier 10"; break;
        default: heapSerial = c_szYes; break;
        }

        if (!pPrintInfo)
        {
            LVLINE("Feature Level", FLName(fl));
            LVLINE("Shader Model", shaderModel);
            LVLINE("Root Signature", rootSig);

            LVYESNO("Standard Swizzle 64KB", d3d12opts.StandardSwizzle64KBSupported);
            LVYESNO("Extended formats TypedUAVLoad", d3d12opts.TypedUAVLoadAdditionalFormats);

            LVLINE("Conservative Rasterization", consrv_rast);
            LVLINE("Resource Binding", binding_rsc);
            LVLINE("Tiled Resources", tiled_rsc);
            LVLINE("Resource Heap", heap);
            LVYESNO("Rasterizer Ordered Views", d3d12opts.ROVsSupported);

            LVYESNO("VP/RT without GS Emulation", d3d12opts.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation);

            LVYESNO("Depth bound test supported", d3d12opts2.DepthBoundsTestSupported);
            LVLINE("Programmable Sample Positions", prgSamplePos);

            LVLINE("View Instancing", viewInstTier);

            LVYESNO("Casting fully typed formats", d3d12opts3.CastingFullyTypedFormatSupported);
            LVYESNO("Copy queue timestamp queries", d3d12opts3.CopyQueueTimestampQueriesSupported);

            LVYESNO("64KB aligned MSAA textures", d3d12opts4.MSAA64KBAlignedTextureSupported);

            LVLINE("Heap serialization", heapSerial);

            LVLINE("Render Passes", renderPasses);

            LVLINE("DirectX Raytracing", dxr);

            LVYESNO("Background processing supported", d3d12opts6.BackgroundProcessingSupported);
        }
        else
        {
            PRINTLINE("Feature Level", FLName(fl));
            PRINTLINE("Shader Model", shaderModel);
            PRINTLINE("Root Signature", rootSig);

            PRINTYESNO("Standard Swizzle 64KB", d3d12opts.StandardSwizzle64KBSupported);
            PRINTYESNO("Extended formats TypedUAVLoad", d3d12opts.TypedUAVLoadAdditionalFormats);

            PRINTLINE("Conservative Rasterization", consrv_rast);
            PRINTLINE("Resource Binding", binding_rsc);
            PRINTLINE("Tiled Resources", tiled_rsc);
            PRINTLINE("Resource Heap", heap);
            PRINTYESNO("Rasterizer Ordered Views", d3d12opts.ROVsSupported);

            PRINTYESNO("VP/RT without GS Emulation", d3d12opts.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation);

            PRINTYESNO("Depth bound test supported", d3d12opts2.DepthBoundsTestSupported);
            PRINTLINE("Programmable Sample Positions", prgSamplePos);

            PRINTLINE("View Instancing", viewInstTier);

            PRINTYESNO("Casting fully typed formats", d3d12opts3.CastingFullyTypedFormatSupported);
            PRINTYESNO("Copy queue timestamp queries", d3d12opts3.CopyQueueTimestampQueriesSupported);

            PRINTYESNO("64KB aligned MSAA textures", d3d12opts4.MSAA64KBAlignedTextureSupported);

            PRINTLINE("Heap serialization", heapSerial);

            PRINTLINE("Render Passes", renderPasses);

            PRINTLINE("DirectX Raytracing", dxr);

            PRINTYESNO("Background processing supported", d3d12opts6.BackgroundProcessingSupported);
        }

        return S_OK;
    }

    HRESULT D3D12Architecture(LPARAM lParam1, LPARAM /*lParam2*/, LPARAM /*lParam3*/, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D12Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        D3D12_FEATURE_DATA_ARCHITECTURE d3d12arch = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &d3d12arch, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE));
        if (FAILED(hr))
            memset(&d3d12arch, 0, sizeof(d3d12arch));

        bool usearch1 = false;
        D3D12_FEATURE_DATA_ARCHITECTURE1 d3d12arch1 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &d3d12arch1, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));
        if (SUCCEEDED(hr))
            usearch1 = true;
        else
            memset(&d3d12arch1, 0, sizeof(d3d12arch1));

        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT d3d12vm = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &d3d12vm, sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT));
        if (FAILED(hr))
            memset(&d3d12vm, 0, sizeof(d3d12vm));

        char vmRes[16];
        sprintf_s(vmRes, 16, "%u", d3d12vm.MaxGPUVirtualAddressBitsPerResource);

        char vmProcess[16];
        sprintf_s(vmProcess, 16, "%u", d3d12vm.MaxGPUVirtualAddressBitsPerProcess);

        if (!pPrintInfo)
        {
            LVYESNO("Tile-based Renderer", d3d12arch.TileBasedRenderer);
            LVYESNO("UMA", d3d12arch.UMA);
            LVYESNO("Cache Coherent UMA", d3d12arch.CacheCoherentUMA);
            if (usearch1)
            {
                LVYESNO("Isolated MMU", d3d12arch1.IsolatedMMU);
            }

            LVLINE("Max GPU VM bits per resource", vmRes);
            LVLINE("Max GPU VM bits per process", vmProcess);
        }
        else
        {
            PRINTYESNO("Tile-based Renderer", d3d12arch.TileBasedRenderer);
            PRINTYESNO("UMA", d3d12arch.UMA);
            PRINTYESNO("Cache Coherent UMA", d3d12arch.CacheCoherentUMA);
            if (usearch1)
            {
                PRINTYESNO("Isolated MMU", d3d12arch1.IsolatedMMU);
            }

            PRINTLINE("Max GPU VM bits per resource", vmRes);
            PRINTLINE("Max GPU VM bits per process", vmProcess);
        }

        return S_OK;
    }

    HRESULT D3D12ExShaderInfo(LPARAM lParam1, LPARAM /*lParam2*/, LPARAM /*lParam3*/, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D12Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        if (FAILED(hr))
            memset(&d3d12opts, 0, sizeof(d3d12opts));

        D3D12_FEATURE_DATA_D3D12_OPTIONS1 d3d12opts1 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &d3d12opts1, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1));
        if (FAILED(hr))
            memset(&d3d12opts1, 0, sizeof(d3d12opts1));

        D3D12_FEATURE_DATA_D3D12_OPTIONS3 d3d12opts3 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &d3d12opts3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
        if (FAILED(hr))
            memset(&d3d12opts3, 0, sizeof(d3d12opts3));

        D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12opts4 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12opts4, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4));
        if (FAILED(hr))
            memset(&d3d12opts4, 0, sizeof(d3d12opts4));

        D3D12_FEATURE_DATA_D3D12_OPTIONS6 d3d12opts6 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &d3d12opts6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (FAILED(hr))
            memset(&d3d12opts6, 0, sizeof(d3d12opts6));

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 d3d12opts7 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &d3d12opts7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
        if (FAILED(hr))
            memset(&d3d12opts7, 0, sizeof(d3d12opts7));

        D3D12_FEATURE_DATA_SHADER_CACHE d3d12sc = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &d3d12sc, sizeof(D3D12_FEATURE_DATA_SHADER_CACHE));
        if (FAILED(hr))
            memset(&d3d12sc, 0, sizeof(d3d12sc));

        const char* precis = nullptr;
        switch (d3d12opts.MinPrecisionSupport & (D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT | D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT))
        {
        case 0:                                         precis = "Full";         break;
        case D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT: precis = "16/32-bit";    break;
        case D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT: precis = "10/32-bit";    break;
        default:                                        precis = "10/16/32-bit"; break;
        }

        char lane_count[16];
        sprintf_s(lane_count, "%u", d3d12opts1.TotalLaneCount);

        char wave_lane_count[16];
        sprintf_s(wave_lane_count, "%u/%u", d3d12opts1.WaveLaneCountMin, d3d12opts1.WaveLaneCountMax);

        const char* vrs = nullptr;
        switch (d3d12opts6.VariableShadingRateTier)
        {
        case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: vrs = c_szNo; break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_1: vrs = "Yes - 1"; break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_2: vrs = "Yes - 2"; break;
        default: vrs = c_szYes; break;
        }

        char vrs_tile_size[16];
        sprintf_s(vrs_tile_size, "%u", d3d12opts6.ShadingRateImageTileSize);

        const char* meshShaders = nullptr;
        switch (d3d12opts7.MeshShaderTier)
        {
        case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED:  meshShaders = c_szNo; break;
        case D3D12_MESH_SHADER_TIER_1:              meshShaders = "Yes - Tier 1"; break;
        default:                                    meshShaders = c_szYes; break;
        }

        const char* feedbackTier = nullptr;
        switch (d3d12opts7.SamplerFeedbackTier)
        {
        case D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED:  feedbackTier = c_szNo; break;
        case D3D12_SAMPLER_FEEDBACK_TIER_0_9:            feedbackTier = "Yes - Tier 0.9"; break;
        case D3D12_SAMPLER_FEEDBACK_TIER_1_0:            feedbackTier = "Yes - Tier 1"; break;
        default:                                         feedbackTier = c_szYes; break;
        }

        char shaderCache[64] = {};
        if (d3d12sc.SupportFlags)
        {
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO)
            {
                strcat_s(shaderCache, "SinglePSO ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_LIBRARY)
            {
                strcat_s(shaderCache, "Library ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE)
            {
                strcat_s(shaderCache, "In-Proc ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE)
            {
                strcat_s(shaderCache, "Disk ");
            }
        }
        else
        {
            strcpy_s(shaderCache, "None");
        }

        if (!pPrintInfo)
        {
            LVYESNO("Double-precision Shaders", d3d12opts.DoublePrecisionFloatShaderOps);

            LVLINE("Minimum Precision", precis);
            LVYESNO("Native 16-bit Shader Ops", d3d12opts4.Native16BitShaderOpsSupported);

            LVYESNO("Wave operations", d3d12opts1.WaveOps);
            if (d3d12opts1.WaveOps)
            {
                LVLINE("Wave lane count", wave_lane_count);
                LVLINE("Total lane count", lane_count);
                LVYESNO("Expanded compute resource states", d3d12opts1.ExpandedComputeResourceStates);
            }

            LVYESNO("PS-Specified Stencil Ref", d3d12opts.PSSpecifiedStencilRefSupported);

            LVYESNO("Barycentrics", d3d12opts3.BarycentricsSupported);

            LVLINE("Variable Rate Shading (VRS)", vrs);
            if (d3d12opts6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
            {
                LVYESNO("VRS: Additional shading rates", d3d12opts6.AdditionalShadingRatesSupported);
                LVYESNO("VRS: Per-primitive w/ SV_ViewportIndex", d3d12opts6.PerPrimitiveShadingRateSupportedWithViewportIndexing);
                LVLINE("VRS: Screen-space tile size", vrs_tile_size);
            }

            LVLINE("Mesh & Amplification Shaders", meshShaders);
            LVLINE("Sampler Feedback", feedbackTier);

            LVLINE("Shader Cache", shaderCache);
        }
        else
        {
            PRINTYESNO("Double-precision Shaders", d3d12opts.DoublePrecisionFloatShaderOps);

            PRINTLINE("Minimum Precision", precis);
            PRINTYESNO("Native 16-bit Shader Ops", d3d12opts4.Native16BitShaderOpsSupported);

            PRINTYESNO("Wave operations", d3d12opts1.WaveOps);
            if (d3d12opts1.WaveOps)
            {
                PRINTLINE("Wave lane count", wave_lane_count);
                PRINTLINE("Total lane count", lane_count);
                PRINTYESNO("Expanded compute resource states", d3d12opts1.ExpandedComputeResourceStates);
            }

            PRINTYESNO("PS-Specified Stencil Ref", d3d12opts.PSSpecifiedStencilRefSupported);

            PRINTYESNO("Barycentrics", d3d12opts3.BarycentricsSupported);

            PRINTLINE("Variable Rate Shading (VRS)", vrs);
            if (d3d12opts6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
            {
                PRINTYESNO("VRS: Additional shading rates", d3d12opts6.AdditionalShadingRatesSupported);
                PRINTYESNO("VRS: Per-primitive w/ SV_ViewportIndex", d3d12opts6.PerPrimitiveShadingRateSupportedWithViewportIndexing);
                PRINTLINE("VRS: Screen-space tile size", vrs_tile_size);
            }

            PRINTLINE("Mesh & Amplification Shaders", meshShaders);
            PRINTLINE("Sampler Feedback", feedbackTier);

            PRINTLINE("Shader Cache", shaderCache);
        }

        return S_OK;
    }

    HRESULT D3D12MultiGPU(LPARAM lParam1, LPARAM /*lParam2*/, LPARAM /*lParam3*/, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D12Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Value", 60);
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        if (FAILED(hr))
            memset(&d3d12opts, 0, sizeof(d3d12opts));

        D3D12_FEATURE_DATA_D3D12_OPTIONS4 d3d12opts4 = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &d3d12opts4, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4));
        if (FAILED(hr))
            memset(&d3d12opts4, 0, sizeof(d3d12opts4));

        D3D12_FEATURE_DATA_CROSS_NODE d3d12xnode = {};
        hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_CROSS_NODE, &d3d12xnode, sizeof(D3D12_FEATURE_DATA_CROSS_NODE));
        if (FAILED(hr))
            memset(&d3d12xnode, 0, sizeof(d3d12xnode));

        char sharing[16];
        switch (d3d12opts.CrossNodeSharingTier)
        {
        case D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED:   strcpy_s(sharing, c_szNo); break;
        case D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED:      strcpy_s(sharing, "Yes - Tier 1 (Emulated)"); break;
        case D3D12_CROSS_NODE_SHARING_TIER_1:               strcpy_s(sharing, "Yes - Tier 1"); break;
        case D3D12_CROSS_NODE_SHARING_TIER_2:               strcpy_s(sharing, "Yes - Tier 2"); break;
        case D3D12_CROSS_NODE_SHARING_TIER_3:               strcpy_s(sharing, "Yes - Tier 3"); break;
        default:                                            strcpy_s(sharing, c_szYes); break;
        }

        const char* sharedResTier = nullptr;
        switch (d3d12opts4.SharedResourceCompatibilityTier)
        {
        case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_0: sharedResTier = "Yes - Tier 0"; break;
        case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_1: sharedResTier = "Yes - Tier 1"; break;
        case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_2: sharedResTier = "Yes - Tier 2"; break;
        default: sharedResTier = c_szYes; break;
        }

        if (!pPrintInfo)
        {
            LVLINE("Cross-node Sharing", sharing);
            LVYESNO("Cross-adapter RM Texture", d3d12opts.CrossAdapterRowMajorTextureSupported);

            LVLINE("Shared Resource Tier", sharedResTier);
            LVYESNO("Atomic Shader Instructions", d3d12xnode.AtomicShaderInstructions);
        }
        else
        {
            PRINTLINE("Cross-node Sharing", sharing);
            PRINTYESNO("Cross-adapter RM Texture", d3d12opts.CrossAdapterRowMajorTextureSupported);

            PRINTLINE("Shared Resource Tier", sharedResTier);
            PRINTYESNO("Atomic Shader Instructions", d3d12xnode.AtomicShaderInstructions);
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    void D3D10_FillTree(HTREEITEM hTree, ID3D10Device* pDevice, D3D_DRIVER_TYPE devType)
    {
        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, "Direct3D 10.0", TRUE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, "Features", FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10(devType));

        TVAddNodeEx(hTreeD3D, "Shader sample (any filter)", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

        TVAddNodeEx(hTreeD3D, "Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

        TVAddNodeEx(hTreeD3D, "Render Target", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_RENDER_TARGET, 0);

        TVAddNodeEx(hTreeD3D, "Blendable Render Target", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_BLENDABLE, 0);

        // MSAA
        FillMSAASampleTable(pDevice, g_sampCount10, FALSE);

        TVAddNodeEx(hTreeD3D, "2x MSAA", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

        TVAddNodeEx(hTreeD3D, "4x MSAA", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

        TVAddNodeEx(hTreeD3D, "8x MSAA", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

        TVAddNode(hTreeD3D, "Other MSAA", FALSE, IDI_CAPS, D3D10InfoMSAA, (LPARAM)pDevice, (LPARAM)g_sampCount10);

        TVAddNodeEx(hTreeD3D, "MSAA Load", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_LOAD, 0);
    }

    void D3D10_FillTree1(HTREEITEM hTree, ID3D10Device1* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D10_FEATURE_LEVEL1 fl = pDevice->GetFeatureLevel();

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, "Direct3D 10.1", TRUE,
            IDI_CAPS, D3D10Info1, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel, (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));

        if ((g_DXGIFactory1 != nullptr && fl != D3D10_FEATURE_LEVEL_9_1)
            || (g_DXGIFactory1 == nullptr && fl != D3D10_FEATURE_LEVEL_10_0))
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, "Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D10_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, "D3D10_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                // Fall thru

            case D3D10_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, "D3D10_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                // Fall thru

            case D3D10_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, "D3D10_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                // Fall thru

            case D3D10_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, "D3D10_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                break;
            }
        }

        // Only display for 10.1 devices. 10 and 11 devices are handled in their "native" node
        // Nothing optional about 10level9 feature levels
        if (fl == D3D10_FEATURE_LEVEL_10_1)
        {
            TVAddNodeEx(hTreeD3D, "Shader sample (any filter)", FALSE, IDI_CAPS, D3D10Info1,
                (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

            TVAddNodeEx(hTreeD3D, "Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D10Info1,
                (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

            TVAddNodeEx(hTreeD3D, "Render Target", FALSE, IDI_CAPS, D3D10Info1,
                (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_RENDER_TARGET, 0);
        }

        // MSAA (for all but 10 devices, which are handled in their "native" node)
        if (fl != D3D10_FEATURE_LEVEL_10_0)
        {
            FillMSAASampleTable(pDevice, g_sampCount10_1, (fl != D3D10_FEATURE_LEVEL_10_1));

            if (fl == D3D10_FEATURE_LEVEL_10_1)
            {
                TVAddNodeEx(hTreeD3D, "2x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

                TVAddNodeEx(hTreeD3D, "4x MSAA (most required)", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

                TVAddNodeEx(hTreeD3D, "8x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

                TVAddNode(hTreeD3D, "Other MSAA", FALSE, IDI_CAPS, D3D10InfoMSAA, (LPARAM)pDevice, (LPARAM)g_sampCount10_1);
            }
            else // 10level9
            {
                TVAddNodeEx(hTreeD3D, "2x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

                if (fl >= D3D10_FEATURE_LEVEL_9_3)
                {
                    TVAddNodeEx(hTreeD3D, "4x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                        (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    HRESULT D3D12InfoVideo(LPARAM lParam1, LPARAM /*lParam2*/, LPARAM /*lParam3*/, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D12Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, "Name", 30);
            LVAddColumn(g_hwndLV, 1, "Texture2D", 15);
            LVAddColumn(g_hwndLV, 2, "Input", 15);
            LVAddColumn(g_hwndLV, 3, "Output", 15);
            LVAddColumn(g_hwndLV, 4, "Encoder", 15);
        }

        static const DXGI_FORMAT cfsVideo[] =
        {
            DXGI_FORMAT_NV12,
            DXGI_FORMAT_420_OPAQUE,
            DXGI_FORMAT_YUY2,
            DXGI_FORMAT_AYUV,
            DXGI_FORMAT_Y410,
            DXGI_FORMAT_Y416,
            DXGI_FORMAT_P010,
            DXGI_FORMAT_P016,
            DXGI_FORMAT_Y210,
            DXGI_FORMAT_Y216,
            DXGI_FORMAT_NV11,
            DXGI_FORMAT_AI44,
            DXGI_FORMAT_IA44,
            DXGI_FORMAT_P8,
            DXGI_FORMAT_A8P8,
        };

        for (UINT i = 0; i < std::size(cfsVideo); ++i)
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT fmtSupport = {
                cfsVideo[i], D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE,
            };
            if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,
                &fmtSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT))))
            {
                fmtSupport.Support1 = D3D12_FORMAT_SUPPORT1_NONE;
                fmtSupport.Support2 = D3D12_FORMAT_SUPPORT2_NONE;
            }

            bool any = (fmtSupport.Support1 & (D3D12_FORMAT_SUPPORT1_TEXTURE2D
                | D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT
                | D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT
                | D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER)) ? true : false;

            if (!pPrintInfo)
            {
                if (g_dwViewState != IDM_VIEWALL && !any)
                    continue;

                LVAddText(g_hwndLV, 0, FormatName(fmtSupport.Format));

                LVAddText(g_hwndLV, 1, (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) ? c_szYes : c_szNo);
                LVAddText(g_hwndLV, 2, (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT) ? c_szYes : c_szNo);
                LVAddText(g_hwndLV, 3, (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT) ? c_szYes : c_szNo);
                LVAddText(g_hwndLV, 4, (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER) ? c_szYes : c_szNo);
            }
            else
            {
                TCHAR buff[1024];

                sprintf_s(buff, "Texture2D: %-3s   Input: %-3s   Output: %-3s   Encoder: %-3s",
                    (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) ? c_szYes : c_szNo,
                    (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT) ? c_szYes : c_szNo,
                    (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT) ? c_szYes : c_szNo,
                    (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER) ? c_szYes : c_szNo);

                PrintStringValueLine(FormatName(fmtSupport.Format), buff, pPrintInfo);
            }
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    void D3D11_FillTree(HTREEITEM hTree, ID3D11Device* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
        if (fl > D3D_FEATURE_LEVEL_11_0)
            fl = D3D_FEATURE_LEVEL_11_0;

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, "Direct3D 11.0", TRUE,
            IDI_CAPS, D3D11Info, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, "Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                break;
            }
        }

        if (fl >= D3D_FEATURE_LEVEL_11_0)
        {
            TVAddNodeEx(hTreeD3D, "Shader sample (any filter)", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

            TVAddNodeEx(hTreeD3D, "Shader gather4", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_GATHER, 0);

            TVAddNodeEx(hTreeD3D, "Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

            TVAddNodeEx(hTreeD3D, "Render Target", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_RENDER_TARGET, 0);

            // MSAA (MSAA data for 10level9 is shown under the 10.1 node)
            FillMSAASampleTable(pDevice, g_sampCount11);

            TVAddNodeEx(hTreeD3D, "2x MSAA", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

            TVAddNodeEx(hTreeD3D, "4x MSAA (all required)", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

            TVAddNodeEx(hTreeD3D, "8x MSAA (most required)", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

            TVAddNodeEx(hTreeD3D, "Other MSAA", FALSE, IDI_CAPS, D3D11InfoMSAA,
                (LPARAM)pDevice, (LPARAM)g_sampCount11, 0);
        }
    }

    void D3D11_FillTree1(HTREEITEM hTree, ID3D11Device1* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
        if (fl > D3D_FEATURE_LEVEL_11_1)
            fl = D3D_FEATURE_LEVEL_11_1;

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, "Direct3D 11.1", TRUE,
            IDI_CAPS, D3D11Info1, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, "Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_11_1:
                if (flMask & FLMASK_11_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                break;
            }
        }

        if (fl >= D3D_FEATURE_LEVEL_10_0)
        {
            TVAddNodeEx(hTreeD3D, "IA Vertex Buffer", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER, 0);

            TVAddNodeEx(hTreeD3D, "Shader sample (any filter)", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                TVAddNodeEx(hTreeD3D, "Shader gather4", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_GATHER, 0);
            }

            TVAddNodeEx(hTreeD3D, "Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

            TVAddNodeEx(hTreeD3D, "Render Target", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_RENDER_TARGET, 0);

            TVAddNodeEx(hTreeD3D, "Blendable Render Target", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_BLENDABLE, 0);

            if (fl < D3D_FEATURE_LEVEL_11_1)
            {
                // This is required for 11.1, but is optional for 10.x and 11.0
                TVAddNodeEx(hTreeD3D, "OM Logic Ops", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_OUTPUT_MERGER_LOGIC_OP);
            }

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                TVAddNodeEx(hTreeD3D, "Typed UAV (most required)", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW, 0);

                TVAddNodeEx(hTreeD3D, "UAV Typed Store (most required)", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_UAV_TYPED_STORE);
            }

            // MSAA (MSAA data for 10level9 is shown under the 10.1 node)
            FillMSAASampleTable(pDevice, g_sampCount11_1);

            TVAddNodeEx(hTreeD3D, "2x MSAA", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

            TVAddNodeEx(hTreeD3D, "4x MSAA (all required)", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

            TVAddNodeEx(hTreeD3D, "8x MSAA (most required)", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

            TVAddNodeEx(hTreeD3D, "Other MSAA", FALSE, IDI_CAPS, D3D11InfoMSAA,
                (LPARAM)pDevice, (LPARAM)g_sampCount11_1, 1);

            TVAddNodeEx(hTreeD3D, "MSAA Load", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD, 0);
        }

        if (devType != D3D_DRIVER_TYPE_REFERENCE)
        {
            TVAddNodeEx(hTreeD3D, "Video", FALSE, IDI_CAPS, D3D11InfoVideo, (LPARAM)pDevice, 0, 0);
        }
    }

    void D3D11_FillTree2(HTREEITEM hTree, ID3D11Device2* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
        if (fl > D3D_FEATURE_LEVEL_11_1)
            fl = D3D_FEATURE_LEVEL_11_1;

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, "Direct3D 11.2", TRUE,
            IDI_CAPS, D3D11Info2, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, "Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_11_1:
                if (flMask & FLMASK_11_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                break;
            }
        }

        // The majority of this data is already shown under the DirectX 11.1 node, so we only show the 'new' info

        TVAddNodeEx(hTreeD3D, "Shareable", FALSE, IDI_CAPS, D3D11Info2,
            (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_SHAREABLE);
    }

    void D3D11_FillTree3(HTREEITEM hTree, ID3D11Device3* pDevice, ID3D11Device4* pDevice4, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, (pDevice4) ? "Direct3D 11.3/11.4" : "Direct3D 11.3", TRUE,
            IDI_CAPS, D3D11Info3, (LPARAM)pDevice, 0, (LPARAM)pDevice4);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, "Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_12_2:
                if (flMask & FLMASK_12_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_12_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_12_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_12_1:
                if (flMask & FLMASK_12_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_12_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_12_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_12_0:
                if (flMask & FLMASK_11_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_11_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_1:
                if (flMask & FLMASK_11_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                break;
            }
        }

        // The majority of this data is already shown under the DirectX 11.1 or 11.2 node, so we only show the 'new' info

        TVAddNodeEx(hTreeD3D, "UAV Typed Load", FALSE, IDI_CAPS, D3D11Info3,
            (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD);
    }

    //-----------------------------------------------------------------------------
    void D3D12_FillTree(HTREEITEM hTree, ID3D12Device* pDevice, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = GetD3D12FeatureLevel(pDevice);

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, "Direct3D 12", TRUE, IDI_CAPS, D3D12Info, (LPARAM)pDevice, (LPARAM)fl, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel, (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));

        if (fl != D3D_FEATURE_LEVEL_11_0)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, "Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_12_2:
                TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_12_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_12_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                // Fall thru

            case D3D_FEATURE_LEVEL_12_1:
                TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_12_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_12_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                // Fall thru

            case D3D_FEATURE_LEVEL_12_0:
                TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_11_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_11_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                // Fall thru

            case D3D_FEATURE_LEVEL_11_1:
                TVAddNodeEx(hTreeF, "D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                break;
            }
        }

        TVAddNodeEx(hTreeD3D, "Architecture", FALSE, IDI_CAPS, D3D12Architecture, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, "Extended Shader Features", FALSE, IDI_CAPS, D3D12ExShaderInfo, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, "Multi-GPU", FALSE, IDI_CAPS, D3D12MultiGPU, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, "Video", FALSE, IDI_CAPS, D3D12InfoVideo, (LPARAM)pDevice, 0, 1);
    }
}

//-----------------------------------------------------------------------------
// Name: DXGI_Init()
//-----------------------------------------------------------------------------
VOID DXGI_Init()
{
    // DXGI
    g_dxgi = LoadLibraryEx("dxgi.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_dxgi)
    {
        auto fpCreateDXGIFactory = reinterpret_cast<LPCREATEDXGIFACTORY>(GetProcAddress(g_dxgi, "CreateDXGIFactory1"));
        if (fpCreateDXGIFactory)
        {
            // DXGI 1.5
            HRESULT hr = fpCreateDXGIFactory(IID_PPV_ARGS(&g_DXGIFactory5));
            if (SUCCEEDED(hr))
            {
                g_DXGIFactory4 = g_DXGIFactory5;
                g_DXGIFactory3 = g_DXGIFactory5;
                g_DXGIFactory2 = g_DXGIFactory5;
                g_DXGIFactory1 = g_DXGIFactory5;
                g_DXGIFactory = g_DXGIFactory5;
            }
            else
            {
                g_DXGIFactory5 = nullptr;

                // DXGI 1.4
                hr = fpCreateDXGIFactory(IID_PPV_ARGS(&g_DXGIFactory4));
                if (SUCCEEDED(hr))
                {
                    g_DXGIFactory3 = g_DXGIFactory4;
                    g_DXGIFactory2 = g_DXGIFactory4;
                    g_DXGIFactory1 = g_DXGIFactory4;
                    g_DXGIFactory = g_DXGIFactory4;
                }
                else
                {
                    g_DXGIFactory4 = nullptr;

                    // DXGI 1.3
                    hr = fpCreateDXGIFactory(IID_PPV_ARGS(&g_DXGIFactory3));
                    if (SUCCEEDED(hr))
                    {
                        g_DXGIFactory2 = g_DXGIFactory3;
                        g_DXGIFactory1 = g_DXGIFactory3;
                        g_DXGIFactory = g_DXGIFactory3;
                    }
                    else
                    {
                        g_DXGIFactory3 = nullptr;

                        // DXGI 1.2
                        hr = fpCreateDXGIFactory(IID_PPV_ARGS(&g_DXGIFactory2));
                        if (SUCCEEDED(hr))
                        {
                            g_DXGIFactory1 = g_DXGIFactory2;
                            g_DXGIFactory = g_DXGIFactory2;
                        }
                        else
                        {
                            g_DXGIFactory2 = nullptr;

                            // DXGI 1.1
                            hr = fpCreateDXGIFactory(IID_PPV_ARGS(&g_DXGIFactory1));
                            if (SUCCEEDED(hr))
                                g_DXGIFactory = g_DXGIFactory1;
                            else
                                g_DXGIFactory1 = nullptr;
                        }
                    }
                }
            }
        }
        else
        {
            // DXGI 1.0
            fpCreateDXGIFactory = reinterpret_cast<LPCREATEDXGIFACTORY>(GetProcAddress(g_dxgi, "CreateDXGIFactory"));

            if (fpCreateDXGIFactory != 0)
            {
                HRESULT hr = fpCreateDXGIFactory(IID_PPV_ARGS(&g_DXGIFactory));
                if (FAILED(hr))
                    g_DXGIFactory = nullptr;
            }
        }
    }

    // Direct3D 10.x
    g_d3d10_1 = LoadLibraryEx("d3d10_1.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_d3d10_1)
    {
        g_D3D10CreateDevice1 = reinterpret_cast<PFN_D3D10_CREATE_DEVICE1>(GetProcAddress(g_d3d10_1, "D3D10CreateDevice1"));
    }
    else
    {
        g_d3d10 = LoadLibraryEx("d3d10.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (g_d3d10)
        {
            g_D3D10CreateDevice = reinterpret_cast<LPD3D10CREATEDEVICE>(GetProcAddress(g_d3d10, "D3D10CreateDevice"));
        }
    }

    // Direct3D 11
    g_d3d11 = LoadLibraryEx("d3d11.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_d3d11)
    {
        g_D3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(g_d3d11, "D3D11CreateDevice"));
    }

    // Direct3D 12
    g_d3d12 = LoadLibraryEx("d3d12.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_d3d12)
    {
        g_D3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(g_d3d12, "D3D12CreateDevice"));
    }
}


//-----------------------------------------------------------------------------
// Name: DXGI_FillTree()
//-----------------------------------------------------------------------------
VOID DXGI_FillTree(HWND hwndTV)
{
    if (!g_DXGIFactory)
        return;

    HTREEITEM hTree = TVAddNode(TVI_ROOT, "DXGI Devices", TRUE, IDI_DIRECTX, nullptr, 0, 0);

    // Hardware driver types
    IDXGIAdapter* pAdapter = nullptr;
    IDXGIAdapter1* pAdapter1 = nullptr;
    IDXGIAdapter2* pAdapter2 = nullptr;
    IDXGIAdapter3* pAdapter3 = nullptr;
    HRESULT hr;
    for (UINT iAdapter = 0; ; ++iAdapter)
    {
        if (g_DXGIFactory1)
        {
            hr = g_DXGIFactory1->EnumAdapters1(iAdapter, &pAdapter1);
            pAdapter = pAdapter1;

            if (SUCCEEDED(hr))
            {
                HRESULT hr2 = pAdapter1->QueryInterface(IID_PPV_ARGS(&pAdapter2));
                if (FAILED(hr2))
                    pAdapter2 = nullptr;

                hr2 = pAdapter1->QueryInterface(IID_PPV_ARGS(&pAdapter3));
                if (FAILED(hr2))
                    pAdapter3 = nullptr;
            }
        }
        else
        {
            hr = g_DXGIFactory->EnumAdapters(iAdapter, &pAdapter);
        }

        if (FAILED(hr))
            break;

        if (pAdapter2)
        {
            DXGI_ADAPTER_DESC2 aDesc2;
            pAdapter2->GetDesc2(&aDesc2);

            if (aDesc2.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Skip "always there" Microsoft Basics Display Driver
                pAdapter2->Release();
                pAdapter1->Release();
                continue;
            }
        }

        DXGI_ADAPTER_DESC aDesc;
        hr = pAdapter->GetDesc(&aDesc);
        if (FAILED(hr))
            continue;

        char szDesc[128];
        wcstombs_s(nullptr, szDesc, aDesc.Description, 128);

        HTREEITEM hTreeA;

        // No need for DXGIAdapterInfo3 as there's no extra desc information to display

        if (pAdapter2)
        {
            hTreeA = TVAddNode(hTree, szDesc, TRUE, IDI_CAPS, DXGIAdapterInfo2, iAdapter, (LPARAM)(pAdapter2));
        }
        else if (pAdapter1)
        {
            hTreeA = TVAddNode(hTree, szDesc, TRUE, IDI_CAPS, DXGIAdapterInfo1, iAdapter, (LPARAM)(pAdapter1));
        }
        else
        {
            hTreeA = TVAddNode(hTree, szDesc, TRUE, IDI_CAPS, DXGIAdapterInfo, iAdapter, (LPARAM)(pAdapter));
        }

        // Outputs
        HTREEITEM hTreeO = nullptr;

        IDXGIOutput* pOutput = nullptr;
        for (UINT iOutput = 0; ; ++iOutput)
        {
            hr = pAdapter->EnumOutputs(iOutput, &pOutput);

            if (FAILED(hr))
                break;

            if (iOutput == 0)
            {
                hTreeO = TVAddNode(hTreeA, "Outputs", TRUE, IDI_CAPS, DXGIFeatures, 0, 0);
            }

            DXGI_OUTPUT_DESC oDesc;
            pOutput->GetDesc(&oDesc);

            char szDeviceName[32];
            wcstombs_s(nullptr, szDeviceName, oDesc.DeviceName, 32);

            HTREEITEM hTreeD = TVAddNode(hTreeO, szDeviceName, TRUE, IDI_CAPS, DXGIOutputInfo, iOutput, (LPARAM)pOutput);

            TVAddNode(hTreeD, "Display Modes", FALSE, IDI_CAPS, DXGIOutputModes, iOutput, (LPARAM)pOutput);
        }

        // Direct3D 12
#ifdef EXTRA_DEBUG
        OutputDebugStringA("Direct3D 12\n");
#endif
        ID3D12Device* pDevice12 = nullptr;

        if (pAdapter3 != 0 && g_D3D12CreateDevice != 0)
        {
            hr = g_D3D12CreateDevice(pAdapter3, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice12));
            if (SUCCEEDED(hr))
            {
#ifdef EXTRA_DEBUG
                D3D_FEATURE_LEVEL fl = GetD3D12FeatureLevel(pDevice12);
                OutputDebugString(FLName(fl));
#endif
                D3D12_FillTree(hTreeA, pDevice12, D3D_DRIVER_TYPE_HARDWARE);
            }
            else
            {
#ifdef EXTRA_DEBUG
                char buff[64] = {};
                sprintf_s(buff, ": Failed (%08X)\n", hr);
                OutputDebugStringA(buff);
#endif
                pDevice12 = nullptr;
            }
        }

        // Direct3D 11.x
#ifdef EXTRA_DEBUG
        OutputDebugStringA("Direct3D 11.x\n");
#endif

        ID3D11Device* pDevice11 = nullptr;
        ID3D11Device1* pDevice11_1 = nullptr;
        ID3D11Device2* pDevice11_2 = nullptr;
        ID3D11Device3* pDevice11_3 = nullptr;
        ID3D11Device4* pDevice11_4 = nullptr;
        DWORD flMaskDX11 = 0;
        if (pAdapter1 != nullptr && g_D3D11CreateDevice != nullptr)
        {
            D3D_FEATURE_LEVEL flHigh = (D3D_FEATURE_LEVEL)0;

            for (UINT i = 1 /* Skip 12.2 for DX11 */; i < std::size(g_featureLevels); ++i)
            {
#ifdef EXTRA_DEBUG
                OutputDebugString(FLName(g_featureLevels[i]));
#endif
                hr = g_D3D11CreateDevice(pAdapter1, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                    &g_featureLevels[i], 1,
                    D3D11_SDK_VERSION, &pDevice11, nullptr, nullptr);
                if (SUCCEEDED(hr))
                {
#ifdef EXTRA_DEBUG
                    OutputDebugString(": Success\n");
#endif
                    switch (g_featureLevels[i])
                    {
                    case D3D_FEATURE_LEVEL_9_1:  flMaskDX11 |= FLMASK_9_1; break;
                    case D3D_FEATURE_LEVEL_9_2:  flMaskDX11 |= FLMASK_9_2; break;
                    case D3D_FEATURE_LEVEL_9_3:  flMaskDX11 |= FLMASK_9_3; break;
                    case D3D_FEATURE_LEVEL_10_0: flMaskDX11 |= FLMASK_10_0; break;
                    case D3D_FEATURE_LEVEL_10_1: flMaskDX11 |= FLMASK_10_1; break;
                    case D3D_FEATURE_LEVEL_11_0: flMaskDX11 |= FLMASK_11_0; break;
                    case D3D_FEATURE_LEVEL_11_1: flMaskDX11 |= FLMASK_11_1; break;
                    case D3D_FEATURE_LEVEL_12_0: flMaskDX11 |= FLMASK_12_0; break;
                    case D3D_FEATURE_LEVEL_12_1: flMaskDX11 |= FLMASK_12_1; break;
                    default: break;
                    }

                    if (g_featureLevels[i] > flHigh)
                        flHigh = g_featureLevels[i];
                }
                else
                {
#ifdef EXTRA_DEBUG
                    char buff[64] = {};
                    sprintf_s(buff, ": Failed (%08X)\n", hr);
                    OutputDebugStringA(buff);
#endif
                    pDevice11 = nullptr;
                }

                if (pDevice11)
                {
                    // Some Intel Integrated Graphics WDDM 1.0 drivers will crash if you try to release here
                    // For this application, leaking a few device instances is not a big deal
                    if (aDesc.VendorId != 0x8086)
                        pDevice11->Release();

                    pDevice11 = nullptr;
                }
            }

            if (flHigh > 0)
            {
                hr = g_D3D11CreateDevice(pAdapter1, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, &flHigh, 1,
                    D3D11_SDK_VERSION, &pDevice11, nullptr, nullptr);

                if (SUCCEEDED(hr))
                {
                    hr = pDevice11->QueryInterface(IID_PPV_ARGS(&pDevice11_1));
                    if (FAILED(hr))
                        pDevice11_1 = nullptr;

                    hr = pDevice11->QueryInterface(IID_PPV_ARGS(&pDevice11_2));
                    if (FAILED(hr))
                        pDevice11_2 = nullptr;

                    hr = pDevice11->QueryInterface(IID_PPV_ARGS(&pDevice11_3));
                    if (FAILED(hr))
                        pDevice11_3 = nullptr;

                    hr = pDevice11->QueryInterface(IID_PPV_ARGS(&pDevice11_4));
                    if (FAILED(hr))
                        pDevice11_4 = nullptr;
                }
                else if (FAILED(hr))
                    pDevice11 = nullptr;
            }
        }

        if (pDevice11 || pDevice11_1 || pDevice11_2 || pDevice11_3)
        {
            HTREEITEM hTree11 = (pDevice11_1 || pDevice11_2 || pDevice11_3)
                ? TVAddNode(hTreeA, "Direct3D 11", TRUE, IDI_CAPS, nullptr, 0, 0)
                : hTreeA;

            if (pDevice11)
                D3D11_FillTree(hTree11, pDevice11, flMaskDX11, D3D_DRIVER_TYPE_HARDWARE);

            if (pDevice11_1)
                D3D11_FillTree1(hTree11, pDevice11_1, flMaskDX11, D3D_DRIVER_TYPE_HARDWARE);

            if (pDevice11_2)
                D3D11_FillTree2(hTree11, pDevice11_2, flMaskDX11, D3D_DRIVER_TYPE_HARDWARE);

            if (pDevice11_3)
                D3D11_FillTree3(hTree11, pDevice11_3, pDevice11_4, flMaskDX11, D3D_DRIVER_TYPE_HARDWARE);
        }

        // Direct3D 10.x
#ifdef EXTRA_DEBUG
        OutputDebugStringA("Direct3D 10.x\n");
#endif
        ID3D10Device* pDevice10 = nullptr;
        ID3D10Device1* pDevice10_1 = nullptr;
        DWORD flMaskDX10 = 0;
        if (g_D3D10CreateDevice1)
        {
            // Since 10 & 10.1 are so close, try to create just one device object for both...
            static const D3D10_FEATURE_LEVEL1 lvl[] =
            {
                D3D10_FEATURE_LEVEL_10_1, D3D10_FEATURE_LEVEL_10_0,
                D3D10_FEATURE_LEVEL_9_3, D3D10_FEATURE_LEVEL_9_2, D3D10_FEATURE_LEVEL_9_1
            };

            // Test every feature-level since some devices might be missing some
            D3D10_FEATURE_LEVEL1 flHigh = (D3D10_FEATURE_LEVEL1)0;
            for (UINT i = 0; i < std::size(lvl); ++i)
            {
                if (g_DXGIFactory1 == 0)
                {
                    // Skip 10level9 if using DXGI 1.0
                    if (lvl[i] == D3D10_FEATURE_LEVEL_9_1
                        || lvl[i] == D3D10_FEATURE_LEVEL_9_2
                        || lvl[i] == D3D10_FEATURE_LEVEL_9_3)
                        continue;
                }

#ifdef EXTRA_DEBUG
                OutputDebugString(FLName(lvl[i]));
#endif

                hr = g_D3D10CreateDevice1(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, lvl[i], D3D10_1_SDK_VERSION, &pDevice10_1);
                if (SUCCEEDED(hr))
                {
#ifdef EXTRA_DEBUG
                    OutputDebugString(": Success\n");
#endif

                    switch (lvl[i])
                    {
                    case D3D10_FEATURE_LEVEL_9_1:  flMaskDX10 |= FLMASK_9_1; break;
                    case D3D10_FEATURE_LEVEL_9_2:  flMaskDX10 |= FLMASK_9_2; break;
                    case D3D10_FEATURE_LEVEL_9_3:  flMaskDX10 |= FLMASK_9_3; break;
                    case D3D10_FEATURE_LEVEL_10_0: flMaskDX10 |= FLMASK_10_0; break;
                    case D3D10_FEATURE_LEVEL_10_1: flMaskDX10 |= FLMASK_10_1; break;
                    }

                    if (lvl[i] > flHigh)
                        flHigh = lvl[i];
                }
                else
                {
#ifdef EXTRA_DEBUG
                    char buff[64] = {};
                    sprintf_s(buff, ": Failed (%08X)\n", hr);
                    OutputDebugStringA(buff);
#endif
                    pDevice10_1 = nullptr;
                }

                if (pDevice10_1)
                {
                    pDevice10_1->Release();
                    pDevice10_1 = nullptr;
                }
            }

            if (flHigh > 0)
            {
                hr = g_D3D10CreateDevice1(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, flHigh, D3D10_1_SDK_VERSION, &pDevice10_1);
                if (SUCCEEDED(hr))
                {
                    if (flHigh >= D3D10_FEATURE_LEVEL_10_0)
                    {
                        hr = pDevice10_1->QueryInterface(IID_PPV_ARGS(&pDevice10));
                        if (FAILED(hr))
                            pDevice10 = nullptr;
                    }
                }
                else
                {
                    pDevice10_1 = nullptr;
                }
            }
        }
        else if (g_D3D10CreateDevice)
        {
            hr = g_D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION, &pDevice10);
            if (FAILED(hr))
                pDevice10 = nullptr;
        }

        // Direct3D 10
        if (pDevice10 || pDevice10_1)
        {
            HTREEITEM hTree10 = (pDevice10_1)
                ? TVAddNode(hTreeA, "Direct3D 10", TRUE, IDI_CAPS, nullptr, 0, 0)
                : hTreeA;

            if (pDevice10)
                D3D10_FillTree(hTree10, pDevice10, D3D_DRIVER_TYPE_HARDWARE);

            // Direct3D 10.1 (includes 10level9 feature levels)
            if (pDevice10_1)
                D3D10_FillTree1(hTree10, pDevice10_1, flMaskDX10, D3D_DRIVER_TYPE_HARDWARE);
        }
    }

    // WARP
    DWORD flMaskWARP = FLMASK_9_1 | FLMASK_9_2 | FLMASK_9_3 | FLMASK_10_0 | FLMASK_10_1;
    ID3D10Device1* pDeviceWARP10 = nullptr;
    if (g_D3D10CreateDevice1)
    {
#ifdef EXTRA_DEBUG
        OutputDebugString("WARP10\n");
#endif

        hr = g_D3D10CreateDevice1(nullptr, D3D10_DRIVER_TYPE_WARP, nullptr, 0, D3D10_FEATURE_LEVEL_10_1,
            D3D10_1_SDK_VERSION, &pDeviceWARP10);
        if (FAILED(hr))
            pDeviceWARP10 = nullptr;
    }

    ID3D11Device* pDeviceWARP11 = nullptr;
    ID3D11Device1* pDeviceWARP11_1 = nullptr;
    ID3D11Device2* pDeviceWARP11_2 = nullptr;
    ID3D11Device3* pDeviceWARP11_3 = nullptr;
    ID3D11Device4* pDeviceWARP11_4 = nullptr;
    if (g_D3D11CreateDevice)
    {
#ifdef EXTRA_DEBUG
        OutputDebugString("WARP11\n");
#endif
        D3D_FEATURE_LEVEL fl;
        // Skip 12.2
        hr = g_D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            &g_featureLevels[1], static_cast<UINT>(std::size(g_featureLevels) - 1),
            D3D11_SDK_VERSION, &pDeviceWARP11, &fl, nullptr);
        if (FAILED(hr))
        {
            // Try without 12.x
            hr = g_D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                &g_featureLevels[3], static_cast<UINT>(std::size(g_featureLevels) - 3),
                D3D11_SDK_VERSION, &pDeviceWARP11, &fl, nullptr);

            if (FAILED(hr))
            {
                hr = g_D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                    D3D11_SDK_VERSION, &pDeviceWARP11, &fl, nullptr);
            }
        }
        if (FAILED(hr))
            pDeviceWARP11 = nullptr;
        else
        {
#ifdef EXTRA_DEBUG
            OutputDebugString(FLName(fl));
#endif
            if (fl >= D3D_FEATURE_LEVEL_12_1)
                flMaskWARP |= FLMASK_12_1;

            if (fl >= D3D_FEATURE_LEVEL_12_0)
                flMaskWARP |= FLMASK_12_0;

            if (fl >= D3D_FEATURE_LEVEL_11_1)
                flMaskWARP |= FLMASK_11_1;

            if (fl >= D3D_FEATURE_LEVEL_11_0)
                flMaskWARP |= FLMASK_11_0;

            hr = pDeviceWARP11->QueryInterface(IID_PPV_ARGS(&pDeviceWARP11_1));
            if (FAILED(hr))
                pDeviceWARP11_1 = nullptr;

            hr = pDeviceWARP11->QueryInterface(IID_PPV_ARGS(&pDeviceWARP11_2));
            if (FAILED(hr))
                pDeviceWARP11_2 = nullptr;

            hr = pDeviceWARP11->QueryInterface(IID_PPV_ARGS(&pDeviceWARP11_3));
            if (FAILED(hr))
                pDeviceWARP11_3 = nullptr;

            hr = pDeviceWARP11->QueryInterface(IID_PPV_ARGS(&pDeviceWARP11_4));
            if (FAILED(hr))
                pDeviceWARP11_4 = nullptr;
        }
    }

    ID3D12Device* pDeviceWARP12 = nullptr;

    if (g_D3D12CreateDevice != 0 && g_DXGIFactory4 != 0)
    {
#ifdef EXTRA_DEBUG
        OutputDebugString("WARP12\n");
#endif
        IDXGIAdapter* warpAdapter = nullptr;
        hr = g_DXGIFactory4->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        if (SUCCEEDED(hr))
        {
            hr = g_D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDeviceWARP12));
            if (SUCCEEDED(hr))
            {
#ifdef EXTRA_DEBUG
                D3D_FEATURE_LEVEL fl = GetD3D12FeatureLevel(pDeviceWARP12);
                OutputDebugString(FLName(fl));
#endif
            }
            else
            {
#ifdef EXTRA_DEBUG
                char buff[64] = {};
                sprintf_s(buff, ": Failed (%08X)\n", hr);
                OutputDebugStringA(buff);
#endif
                pDeviceWARP12 = nullptr;
            }

            warpAdapter->Release();
        }
#ifdef EXTRA_DEBUG
        else
        {
            OutputDebugString("WARP12 adapter not found!\n");
        }
#endif
    }

    if (pDeviceWARP10 || pDeviceWARP11 || pDeviceWARP11_1 || pDeviceWARP11_2 || pDeviceWARP11_3 || pDeviceWARP11_4 || pDeviceWARP12)
    {
        HTREEITEM hTreeW = TVAddNode(hTree, "Windows Advanced Rasterization Platform (WARP)", TRUE, IDI_CAPS, nullptr, 0, 0);

        // DirectX 12 (WARP)
        if (pDeviceWARP12)
            D3D12_FillTree(hTreeW, pDeviceWARP12, D3D_DRIVER_TYPE_WARP);

        // DirectX 11.x (WARP)
        if (pDeviceWARP11 || pDeviceWARP11_1 || pDeviceWARP11_2 || pDeviceWARP11_3)
        {
            HTREEITEM hTree11 = (pDeviceWARP11_1 || pDeviceWARP11_2 || pDeviceWARP11_3)
                ? TVAddNode(hTreeW, "Direct3D 11", TRUE, IDI_CAPS, nullptr, 0, 0)
                : hTreeW;

            if (pDeviceWARP11)
                D3D11_FillTree(hTree11, pDeviceWARP11, flMaskWARP, D3D_DRIVER_TYPE_WARP);

            if (pDeviceWARP11_1)
                D3D11_FillTree1(hTree11, pDeviceWARP11_1, flMaskWARP, D3D_DRIVER_TYPE_WARP);

            if (pDeviceWARP11_2)
                D3D11_FillTree2(hTree11, pDeviceWARP11_2, flMaskWARP, D3D_DRIVER_TYPE_WARP);

            if (pDeviceWARP11_3)
                D3D11_FillTree3(hTree11, pDeviceWARP11_3, pDeviceWARP11_4, flMaskWARP, D3D_DRIVER_TYPE_WARP);
        }

        // DirectX 10.x (WARP)
        if (pDeviceWARP10)
        {
            // WARP supported both 10 and 10.1 when first released
            HTREEITEM hTree10 = TVAddNode(hTreeW, "Direct3D 10", TRUE, IDI_CAPS, nullptr, 0, 0);

            D3D10_FillTree(hTree10, pDeviceWARP10, D3D_DRIVER_TYPE_WARP);
            D3D10_FillTree1(hTree10, pDeviceWARP10, flMaskWARP, D3D_DRIVER_TYPE_WARP);
        }
    }

    // REFERENCE
    ID3D10Device1* pDeviceREF10_1 = nullptr;
    ID3D10Device* pDeviceREF10 = nullptr;
    if (g_D3D10CreateDevice1)
    {
        hr = g_D3D10CreateDevice1(nullptr, D3D10_DRIVER_TYPE_REFERENCE, nullptr, 0, D3D10_FEATURE_LEVEL_10_1,
            D3D10_1_SDK_VERSION, &pDeviceREF10_1);
        if (SUCCEEDED(hr))
        {
            hr = pDeviceREF10_1->QueryInterface(IID_PPV_ARGS(&pDeviceREF10));
            if (FAILED(hr))
                pDeviceREF10 = nullptr;
        }
        else
            pDeviceREF10_1 = nullptr;
    }
    else if (g_D3D10CreateDevice != nullptr)
    {
        hr = g_D3D10CreateDevice(nullptr, D3D10_DRIVER_TYPE_REFERENCE, nullptr, 0, D3D10_SDK_VERSION, &pDeviceREF10);
        if (FAILED(hr))
            pDeviceREF10 = nullptr;
    }

    ID3D11Device* pDeviceREF11 = nullptr;
    ID3D11Device1* pDeviceREF11_1 = nullptr;
    ID3D11Device2* pDeviceREF11_2 = nullptr;
    ID3D11Device3* pDeviceREF11_3 = nullptr;
    ID3D11Device4* pDeviceREF11_4 = nullptr;
    DWORD flMaskREF = FLMASK_9_1 | FLMASK_9_2 | FLMASK_9_3 | FLMASK_10_0 | FLMASK_10_1 | FLMASK_11_0;
    if (g_D3D11CreateDevice)
    {
        D3D_FEATURE_LEVEL lvl = D3D_FEATURE_LEVEL_11_1;
        hr = g_D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_REFERENCE, nullptr, 0, &lvl, 1,
            D3D11_SDK_VERSION, &pDeviceREF11, nullptr, nullptr);

        if (SUCCEEDED(hr))
        {
            flMaskREF |= FLMASK_11_1;
            hr = pDeviceREF11->QueryInterface(IID_PPV_ARGS(&pDeviceREF11_1));
            if (FAILED(hr))
                pDeviceREF11_1 = nullptr;

            hr = pDeviceREF11->QueryInterface(IID_PPV_ARGS(&pDeviceREF11_2));
            if (FAILED(hr))
                pDeviceREF11_2 = nullptr;

            hr = pDeviceREF11->QueryInterface(IID_PPV_ARGS(&pDeviceREF11_3));
            if (FAILED(hr))
                pDeviceREF11_3 = nullptr;

            hr = pDeviceREF11->QueryInterface(IID_PPV_ARGS(&pDeviceREF11_4));
            if (FAILED(hr))
                pDeviceREF11_4 = nullptr;
        }
        else
        {
            hr = g_D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_REFERENCE, nullptr, 0, nullptr, 0,
                D3D11_SDK_VERSION, &pDeviceREF11, nullptr, nullptr);
            if (SUCCEEDED(hr))
            {
                hr = pDeviceREF11->QueryInterface(IID_PPV_ARGS(&pDeviceREF11_1));
                if (FAILED(hr))
                    pDeviceREF11_1 = nullptr;
            }
        }
    }

    if (pDeviceREF10 || pDeviceREF10_1 || pDeviceREF11 || pDeviceREF11_1 || pDeviceREF11_2 || pDeviceREF11_3)
    {
        HTREEITEM hTreeR = TVAddNode(hTree, "Reference", TRUE, IDI_CAPS, nullptr, 0, 0);

        // No REF for Direct3D 12

        // Direct3D 11.x (REF)
        if (pDeviceREF11 || pDeviceREF11_1 || pDeviceREF11_2 || pDeviceREF11_3)
        {
            HTREEITEM hTree11 = (pDeviceREF11_1 || pDeviceREF11_2 || pDeviceREF11_3)
                ? TVAddNode(hTreeR, "Direct3D 11", TRUE, IDI_CAPS, nullptr, 0, 0)
                : hTreeR;

            if (pDeviceREF11)
                D3D11_FillTree(hTree11, pDeviceREF11, flMaskREF, D3D_DRIVER_TYPE_REFERENCE);

            if (pDeviceREF11_1)
                D3D11_FillTree1(hTree11, pDeviceREF11_1, flMaskREF, D3D_DRIVER_TYPE_REFERENCE);

            if (pDeviceREF11_2)
                D3D11_FillTree2(hTree11, pDeviceREF11_2, flMaskREF, D3D_DRIVER_TYPE_REFERENCE);

            if (pDeviceREF11_3)
                D3D11_FillTree3(hTree11, pDeviceREF11_3, pDeviceREF11_4, flMaskREF, D3D_DRIVER_TYPE_REFERENCE);
        }

        // Direct3D 10.x (REF)
        if (pDeviceREF10 || pDeviceREF10_1)
        {
            HTREEITEM hTree10 = (pDeviceREF10_1)
                ? TVAddNode(hTreeR, "Direct3D 10", TRUE, IDI_CAPS, nullptr, 0, 0)
                : hTreeR;

            if (pDeviceREF10)
                D3D10_FillTree(hTree10, pDeviceREF10, D3D_DRIVER_TYPE_REFERENCE);

            if (pDeviceREF10_1)
                D3D10_FillTree1(hTree10, pDeviceREF10_1, FLMASK_10_0 | FLMASK_10_1, D3D_DRIVER_TYPE_REFERENCE);
        }
    }

    TreeView_Expand(hwndTV, hTree, TVE_EXPAND);
}


//-----------------------------------------------------------------------------
// Name: DXGI_CleanUp()
//-----------------------------------------------------------------------------
VOID DXGI_CleanUp()
{
    if (g_DXGIFactory)
    {
        SAFE_RELEASE(g_DXGIFactory);
        g_DXGIFactory = nullptr;
        g_DXGIFactory1 = nullptr;
        g_DXGIFactory2 = nullptr;
        g_DXGIFactory3 = nullptr;
        g_DXGIFactory4 = nullptr;
        g_DXGIFactory5 = nullptr;
    }

    if (g_dxgi)
    {
        FreeLibrary(g_dxgi);
        g_dxgi = nullptr;
    }

    if (g_d3d10)
    {
        FreeLibrary(g_d3d10);
        g_d3d10 = nullptr;
    }

    if (g_d3d10_1)
    {
        FreeLibrary(g_d3d10_1);
        g_d3d10_1 = nullptr;
    }

    if (g_d3d11)
    {
        FreeLibrary(g_d3d11);
        g_d3d11 = nullptr;
    }

    if (g_d3d12)
    {
        FreeLibrary(g_d3d12);
        g_d3d12 = nullptr;
    }
}
