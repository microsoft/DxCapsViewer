//-----------------------------------------------------------------------------
// Name: dxgi.cpp
//
// Desc: DirectX Capabilities Viewer for DXGI (Direct3D 10.x / 11.x / 12.0)
//
// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?linkid=2136896
//-----------------------------------------------------------------------------
#include "dxview.h"

#ifdef USING_DIRECTX_HEADERS
#include <directx/dxgiformat.h>
#include <directx/d3d12.h>
#else
#include <d3d12.h>
#endif

#include <D3Dcommon.h>
#include <dxgi1_6.h>
#include <d3d10_1.h>
#include <d3d11_4.h>

// Define for some debug output
//#define EXTRA_DEBUG

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
    // Used to enable the "Agility SDK" components
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\";
}
#endif

//-----------------------------------------------------------------------------

// This mask is only needed for Direct3D 10.x/11 where some devices had 'holes' in the feature support.
enum FLMASK : uint32_t
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
};

#if !defined(NTDDI_WIN10_FE) && !defined(USING_D3D12_AGILITY_SDK)
#define D3D_FEATURE_LEVEL_12_2 static_cast<D3D_FEATURE_LEVEL>(0xc200)
#define D3D_SHADER_MODEL_6_7 static_cast<D3D_SHADER_MODEL>(0x67)
#pragma warning(disable : 4063 4702)
#endif

#if !defined(NTDDI_WIN10_CU) && !defined(USING_D3D12_AGILITY_SDK)
#define D3D_SHADER_MODEL_6_8 static_cast<D3D_SHADER_MODEL>(0x68)
#pragma warning(disable : 4063 4702)
#endif

#if !defined(NTDDI_WIN10_CU) && !defined(USING_D3D12_AGILITY_SDK)
#define D3D_ROOT_SIGNATURE_VERSION_1_2 static_cast<D3D_ROOT_SIGNATURE_VERSION>(0x3)
#pragma warning(disable : 4063 4702)
#endif

#if !defined(NTDDI_WIN11_GE) && !defined(USING_D3D12_AGILITY_SDK)
#define D3D_SHADER_MODEL_6_9 static_cast<D3D_SHADER_MODEL>(0x69)
#define D3D_HIGHEST_SHADER_MODEL static_cast<D3D_SHADER_MODEL>(0x69)
#pragma warning(disable : 4063 4702)
#endif

//-----------------------------------------------------------------------------
namespace
{
    const WCHAR D3D10_NOTE[] = L"Most Direct3D 10 features are required. Tool only shows optional features.";
    const WCHAR D3D10_NOTE1[] = L"Most Direct3D 10.1 features are required. Tool only shows optional features.";
    const WCHAR D3D11_NOTE[] = L"Most Direct3D 11 features are required. Tool only shows optional features.";
    const WCHAR D3D11_NOTE1[] = L"Most Direct3D 11.1 features are required. Tool only shows optional features.";
    const WCHAR D3D11_NOTE2[] = L"Most Direct3D 11.2 features are required. Tool only shows optional features.";
    const WCHAR D3D11_NOTE3[] = L"Most Direct3D 11.3 features are required. Tool only shows optional features.";
    const WCHAR D3D11_NOTE4[] = L"Most Direct3D 11.x features are required. Tool only shows optional features.";
    const WCHAR _10L9_NOTE[] = L"Most 10level9 features are required. Tool only shows optional features.";
    const WCHAR SEE_D3D10[] = L"See Direct3D 10 node for device details.";
    const WCHAR SEE_D3D10_1[] = L"See Direct3D 10.1 node for device details.";
    const WCHAR SEE_D3D11[] = L"See Direct3D 11 node for device details.";
    const WCHAR SEE_D3D11_1[] = L"See Direct3D 11.1 node for device details.";

    const WCHAR FL_NOTE[] = L"This feature summary is derived from hardware feature level";

    static_assert(_countof(D3D10_NOTE) < 80, "String too long");
    static_assert(_countof(D3D10_NOTE1) < 80, "String too long");
    static_assert(_countof(D3D11_NOTE) < 80, "String too long");
    static_assert(_countof(D3D11_NOTE1) < 80, "String too long");
    static_assert(_countof(D3D11_NOTE2) < 80, "String too long");
    static_assert(_countof(D3D11_NOTE3) < 80, "String too long");
    static_assert(_countof(D3D11_NOTE4) < 80, "String too long");
    static_assert(_countof(_10L9_NOTE) < 80, "String too long");
    static_assert(_countof(SEE_D3D10) < 80, "String too long");
    static_assert(_countof(SEE_D3D10_1) < 80, "String too long");
    static_assert(_countof(SEE_D3D11) < 80, "String too long");
    static_assert(_countof(SEE_D3D11_1) < 80, "String too long");

    static_assert(_countof(FL_NOTE) < 80, "String too long");

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
extern const WCHAR c_szYes[];
extern const WCHAR c_szNo[];
extern const WCHAR c_szNA[];

const WCHAR c_szOptYes[] = L"Optional (Yes)";
const WCHAR c_szOptNo[] = L"Optional (No)";

namespace
{
    const WCHAR* szRotation[] =
    {
        L"DXGI_MODE_ROTATION_UNSPECIFIED",
        L"DXGI_MODE_ROTATION_IDENTITY",
        L"DXGI_MODE_ROTATION_ROTATE90",
        L"DXGI_MODE_ROTATION_ROTATE180",
        L"DXGI_MODE_ROTATION_ROTATE270"
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

    D3D_SHADER_MODEL GetD3D12ShaderModel(_In_opt_ ID3D12Device* device)
    {
        if (!device)
            return D3D_SHADER_MODEL_5_1;

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModelOpt = {};
        shaderModelOpt.HighestShaderModel = D3D_HIGHEST_SHADER_MODEL;
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

    D3D_ROOT_SIGNATURE_VERSION GetD3D12RootSignature(_In_opt_ ID3D12Device* device)
    {
        if (!device)
            return D3D_ROOT_SIGNATURE_VERSION_1_0;

        D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSigOpt = {};
        rootSigOpt.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_2;
        HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSigOpt, sizeof(rootSigOpt));
        while (hr == E_INVALIDARG && rootSigOpt.HighestVersion > D3D_ROOT_SIGNATURE_VERSION_1_0)
        {
            rootSigOpt.HighestVersion = static_cast<D3D_ROOT_SIGNATURE_VERSION>(static_cast<int>(rootSigOpt.HighestVersion) - 1);
            hr = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSigOpt, sizeof(rootSigOpt));
        }
        if (FAILED(hr))
        {
            rootSigOpt.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        return rootSigOpt.HighestVersion;
    }

    const WCHAR* D3D12DXRSupported(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12opts = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5))))
        {
            switch (d3d12opts.RaytracingTier)
            {
            case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: break;
            case D3D12_RAYTRACING_TIER_1_0: return L"Optional (Yes - Tier 1.0)";
            case D3D12_RAYTRACING_TIER_1_1: return L"Optional (Yes - Tier 1.1)";
            default: return c_szOptYes;
            }
        }

        return c_szOptNo;
    }

    const WCHAR* D3D12VRSSupported(_In_ ID3D12Device* device)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 d3d12opts = {};
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6))))
        {
            switch (d3d12opts.VariableShadingRateTier)
            {
            case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: break;
            case D3D12_VARIABLE_SHADING_RATE_TIER_1: return L"Optional (Yes - Tier 1)";
            case D3D12_VARIABLE_SHADING_RATE_TIER_2: return L"Optional (Yes - Tier 2)";
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

    template<D3D11_FEATURE feature, typename T>
    T GetD3D11Options(_In_ ID3D11Device* device)
    {
        T opts = {};
        if (FAILED(device->CheckFeatureSupport(feature, &opts, sizeof(T))))
        {
            memset(&opts, 0, sizeof(T));
        }
        return opts;
    }

    template<D3D12_FEATURE feature, typename T>
    T GetD3D12Options(_In_ ID3D12Device* device)
    {
        T opts = {};
        if (FAILED(device->CheckFeatureSupport(feature, &opts, sizeof(T))))
        {
            memset(&opts, 0, sizeof(T));
        }
        return opts;
    }

    //-----------------------------------------------------------------------------
#define ENUMNAME(a) case a: return TEXT(#a)

    const WCHAR* FormatName(DXGI_FORMAT format)
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
            return L"DXGI_FORMAT_UNKNOWN";
        }
    }

    const WCHAR* FLName(D3D10_FEATURE_LEVEL1 lvl)
    {
        switch (lvl)
        {
            ENUMNAME(D3D10_FEATURE_LEVEL_10_0);
            ENUMNAME(D3D10_FEATURE_LEVEL_10_1);
            ENUMNAME(D3D10_FEATURE_LEVEL_9_1);
            ENUMNAME(D3D10_FEATURE_LEVEL_9_2);
            ENUMNAME(D3D10_FEATURE_LEVEL_9_3);

        default:
            return L"D3D10_FEATURE_LEVEL_UNKNOWN";
        }
    }

    const WCHAR* FLName(D3D_FEATURE_LEVEL lvl)
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
            ENUMNAME(D3D_FEATURE_LEVEL_12_2);

        default:
            return L"D3D_FEATURE_LEVEL_UNKNOWN";
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 50);
        }

        DXGI_ADAPTER_DESC desc;
        HRESULT hr = pAdapter->GetDesc(&desc);
        if (FAILED(hr))
            return hr;

        DWORD dvm = (DWORD)(desc.DedicatedVideoMemory / (1024 * 1024));
        DWORD dsm = (DWORD)(desc.DedicatedSystemMemory / (1024 * 1024));
        DWORD ssm = (DWORD)(desc.SharedSystemMemory / (1024 * 1024));

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, L"Description");
            LVAddText(g_hwndLV, 1, L"%s", desc.Description);

            LVAddText(g_hwndLV, 0, L"VendorId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.VendorId);

            LVAddText(g_hwndLV, 0, L"DeviceId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.DeviceId);

            LVAddText(g_hwndLV, 0, L"SubSysId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.SubSysId);

            LVAddText(g_hwndLV, 0, L"Revision");
            LVAddText(g_hwndLV, 1, L"%d", desc.Revision);

            LVAddText(g_hwndLV, 0, L"DedicatedVideoMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", dvm);

            LVAddText(g_hwndLV, 0, L"DedicatedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", dsm);

            LVAddText(g_hwndLV, 0, L"SharedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", ssm);
        }
        else
        {
            PrintStringValueLine(L"Description", desc.Description, pPrintInfo);
            PrintHexValueLine(L"VendorId", desc.VendorId, pPrintInfo);
            PrintHexValueLine(L"DeviceId", desc.DeviceId, pPrintInfo);
            PrintHexValueLine(L"SubSysId", desc.SubSysId, pPrintInfo);
            PrintValueLine(L"Revision", desc.Revision, pPrintInfo);
            PrintValueLine(L"DedicatedVideoMemory (MB)", dvm, pPrintInfo);
            PrintValueLine(L"DedicatedSystemMemory (MB)", dsm, pPrintInfo);
            PrintValueLine(L"SharedSystemMemory (MB)", ssm, pPrintInfo);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 50);
        }

        DXGI_ADAPTER_DESC1 desc;
        HRESULT hr = pAdapter->GetDesc1(&desc);
        if (FAILED(hr))
            return hr;

        auto dvm = static_cast<DWORD>(desc.DedicatedVideoMemory / (1024 * 1024));
        auto dsm = static_cast<DWORD>(desc.DedicatedSystemMemory / (1024 * 1024));
        auto ssm = static_cast<DWORD>(desc.SharedSystemMemory / (1024 * 1024));

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, L"Description");
            LVAddText(g_hwndLV, 1, L"%s", desc.Description);

            LVAddText(g_hwndLV, 0, L"VendorId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.VendorId);

            LVAddText(g_hwndLV, 0, L"DeviceId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.DeviceId);

            LVAddText(g_hwndLV, 0, L"SubSysId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.SubSysId);

            LVAddText(g_hwndLV, 0, L"Revision");
            LVAddText(g_hwndLV, 1, L"%d", desc.Revision);

            LVAddText(g_hwndLV, 0, L"DedicatedVideoMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", dvm);

            LVAddText(g_hwndLV, 0, L"DedicatedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", dsm);

            LVAddText(g_hwndLV, 0, L"SharedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", ssm);

            LVAddText(g_hwndLV, 0, L"Remote");
            LVAddText(g_hwndLV, 1, (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo);
        }
        else
        {
            PrintStringValueLine(L"Description", desc.Description, pPrintInfo);
            PrintHexValueLine(L"VendorId", desc.VendorId, pPrintInfo);
            PrintHexValueLine(L"DeviceId", desc.DeviceId, pPrintInfo);
            PrintHexValueLine(L"SubSysId", desc.SubSysId, pPrintInfo);
            PrintValueLine(L"Revision", desc.Revision, pPrintInfo);
            PrintValueLine(L"DedicatedVideoMemory (MB)", dvm, pPrintInfo);
            PrintValueLine(L"DedicatedSystemMemory (MB)", dsm, pPrintInfo);
            PrintValueLine(L"SharedSystemMemory (MB)", ssm, pPrintInfo);
            PrintStringValueLine(L"Remote", (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo, pPrintInfo);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 50);
        }

        DXGI_ADAPTER_DESC2 desc;
        HRESULT hr = pAdapter->GetDesc2(&desc);
        if (FAILED(hr))
            return hr;

        auto dvm = static_cast<DWORD>(desc.DedicatedVideoMemory / (1024 * 1024));
        auto dsm = static_cast<DWORD>(desc.DedicatedSystemMemory / (1024 * 1024));
        auto ssm = static_cast<DWORD>(desc.SharedSystemMemory / (1024 * 1024));

        const WCHAR* gpg = nullptr;
        const WCHAR* cpg = nullptr;

        switch (desc.GraphicsPreemptionGranularity)
        {
        case DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY:    gpg = L"DMA Buffer"; break;
        case DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY:     gpg = L"Primitive"; break;
        case DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY:      gpg = L"Triangle"; break;
        case DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY:         gpg = L"Pixel"; break;
        case DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY:   gpg = L"Instruction"; break;
        default:                                              gpg = L"Unknown"; break;
        }

        switch (desc.ComputePreemptionGranularity)
        {
        case DXGI_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY:    cpg = L"DMA Buffer"; break;
        case DXGI_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY:      cpg = L"Dispatch"; break;
        case DXGI_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY:  cpg = L"Thread Group"; break;
        case DXGI_COMPUTE_PREEMPTION_THREAD_BOUNDARY:        cpg = L"Thread"; break;
        case DXGI_COMPUTE_PREEMPTION_INSTRUCTION_BOUNDARY:   cpg = L"Instruction"; break;
        default:                                             cpg = L"Unknown"; break;
        }

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, L"Description");
            LVAddText(g_hwndLV, 1, L"%s", desc.Description);

            LVAddText(g_hwndLV, 0, L"VendorId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.VendorId);

            LVAddText(g_hwndLV, 0, L"DeviceId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.DeviceId);

            LVAddText(g_hwndLV, 0, L"SubSysId");
            LVAddText(g_hwndLV, 1, L"0x%08x", desc.SubSysId);

            LVAddText(g_hwndLV, 0, L"Revision");
            LVAddText(g_hwndLV, 1, L"%d", desc.Revision);

            LVAddText(g_hwndLV, 0, L"DedicatedVideoMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", dvm);

            LVAddText(g_hwndLV, 0, L"DedicatedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", dsm);

            LVAddText(g_hwndLV, 0, L"SharedSystemMemory (MB)");
            LVAddText(g_hwndLV, 1, L"%d", ssm);

            LVAddText(g_hwndLV, 0, L"Remote");
            LVAddText(g_hwndLV, 1, (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo);

            LVAddText(g_hwndLV, 0, L"Graphics Preemption Granularity");
            LVAddText(g_hwndLV, 1, gpg);

            LVAddText(g_hwndLV, 0, L"Compute Preemption Granularity");
            LVAddText(g_hwndLV, 1, cpg);
        }
        else
        {
            PrintStringValueLine(L"Description", desc.Description, pPrintInfo);
            PrintHexValueLine(L"VendorId", desc.VendorId, pPrintInfo);
            PrintHexValueLine(L"DeviceId", desc.DeviceId, pPrintInfo);
            PrintHexValueLine(L"SubSysId", desc.SubSysId, pPrintInfo);
            PrintValueLine(L"Revision", desc.Revision, pPrintInfo);
            PrintValueLine(L"DedicatedVideoMemory (MB)", dvm, pPrintInfo);
            PrintValueLine(L"DedicatedSystemMemory (MB)", dsm, pPrintInfo);
            PrintValueLine(L"SharedSystemMemory (MB)", ssm, pPrintInfo);
            PrintStringValueLine(L"Remote", (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) ? c_szYes : c_szNo, pPrintInfo);
            PrintStringValueLine(L"Graphics Preemption Granularity", gpg, pPrintInfo);
            PrintStringValueLine(L"Compute Preemption Granularity", cpg, pPrintInfo);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 40);
        }

        DXGI_OUTPUT_DESC desc;
        HRESULT hr = pOutput->GetDesc(&desc);
        if (FAILED(hr))
            return hr;

        if (!pPrintInfo)
        {
            LVAddText(g_hwndLV, 0, L"DeviceName");
            LVAddText(g_hwndLV, 1, L"%s", desc.DeviceName);

            LVAddText(g_hwndLV, 0, L"AttachedToDesktop");
            LVAddText(g_hwndLV, 1, desc.AttachedToDesktop ? c_szYes : c_szNo);

            LVAddText(g_hwndLV, 0, L"Rotation");
            LVAddText(g_hwndLV, 1, szRotation[desc.Rotation]);
        }
        else
        {
            PrintStringValueLine(L"DeviceName", desc.DeviceName, pPrintInfo);
            PrintStringValueLine(L"AttachedToDesktop", desc.AttachedToDesktop ? c_szYes : c_szNo, pPrintInfo);
            PrintStringValueLine(L"Rotation", szRotation[desc.Rotation], pPrintInfo);
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
            LVAddColumn(g_hwndLV, 0, L"Resolution", 14);
            LVAddColumn(g_hwndLV, 1, L"Pixel Format", 40);
            LVAddColumn(g_hwndLV, 2, L"Refresh Rate", 10);
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
                            LVAddText(g_hwndLV, 0, L"%d x %d", pDesc->Width, pDesc->Height);
                            LVAddText(g_hwndLV, 1, FormatName(pDesc->Format));
                            LVAddText(g_hwndLV, 2, L"%d", RefreshRate(pDesc->RefreshRate));
                        }
                        else
                        {
                            WCHAR szBuff[80];
                            DWORD cchLen;

                            // Calculate Name and Value column x offsets
                            int x1 = (pPrintInfo->dwCurrIndent * DEF_TAB_SIZE * pPrintInfo->dwCharWidth);
                            int x2 = x1 + (30 * pPrintInfo->dwCharWidth);
                            int x3 = x2 + (20 * pPrintInfo->dwCharWidth);
                            int yLine = (pPrintInfo->dwCurrLine * pPrintInfo->dwLineHeight);

                            swprintf_s(szBuff, 80, L"%u x %u", pDesc->Width, pDesc->Height);
                            cchLen = static_cast<DWORD>(wcslen(szBuff));
                            if (FAILED(PrintLine(x1, yLine, szBuff, cchLen, pPrintInfo)))
                                return E_FAIL;

                            wcscpy_s(szBuff, 80, FormatName(pDesc->Format));
                            cchLen = static_cast<DWORD>(wcslen(szBuff));
                            if (FAILED(PrintLine(x2, yLine, szBuff, cchLen, pPrintInfo)))
                                return E_FAIL;

                            swprintf_s(szBuff, 80, L"%u", RefreshRate(pDesc->RefreshRate));
                            cchLen = static_cast<DWORD>(wcslen(szBuff));
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

#define XTOSTRING(a) TEXT(#a) TOSTRING(a)
#define TOSTRING(a) #a

#define XTOSTRING2(a) TEXT(#a) TOSTRING2(a)
#define TOSTRING2(a) "( " #a " )"

    //-----------------------------------------------------------------------------
    HRESULT DXGIFeatures(LPARAM /*lParam1*/, LPARAM /*lParam2*/, PRINTCBINFO* pPrintInfo)
    {
        if (g_DXGIFactory5)
        {
            if (!pPrintInfo)
            {
                LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
                LVAddColumn(g_hwndLV, 1, L"Value", 60);
            }

            BOOL allowTearing = FALSE;
            HRESULT hr = g_DXGIFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL));
            if (FAILED(hr))
                allowTearing = FALSE;

            if (!pPrintInfo)
            {
                LVYESNO(L"Allow tearing", allowTearing);
            }
            else
            {
                PRINTYESNO(L"Allow tearing", allowTearing);
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
        auto opts = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS, D3D11_FEATURE_DATA_D3D11_OPTIONS>(pDevice);

        logicOps = (opts.OutputMergerLogicOp) ? true : false;
        cbPartial = (opts.ConstantBufferPartialUpdate) ? true : false;
        cbOffsetting = (opts.ConstantBufferOffsetting) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D11Ops1(ID3D11Device* pDevice, D3D11_TILED_RESOURCES_TIER& tiled, bool& minmaxfilter, bool& mapdefaultbuff)
    {
        auto opts = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS1, D3D11_FEATURE_DATA_D3D11_OPTIONS1>(pDevice);

        tiled = opts.TiledResourcesTier;
        minmaxfilter = (opts.MinMaxFiltering) ? true : false;
        mapdefaultbuff = (opts.MapOnDefaultBuffers) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D11Ops2(ID3D11Device* pDevice, D3D11_TILED_RESOURCES_TIER& tiled, D3D11_CONSERVATIVE_RASTERIZATION_TIER& crast,
        bool& rovs, bool& pssref)
    {
        auto opts = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS2, D3D11_FEATURE_DATA_D3D11_OPTIONS2>(pDevice);

        crast = opts.ConservativeRasterizationTier;
        tiled = opts.TiledResourcesTier;
        rovs = (opts.ROVsSupported) ? true : false;
        pssref = (opts.PSSpecifiedStencilRefSupported) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D9Ops(ID3D11Device* pDevice, bool& nonpow2, bool& shadows)
    {
        auto d3d9opts = GetD3D11Options<D3D11_FEATURE_D3D9_OPTIONS, D3D11_FEATURE_DATA_D3D9_OPTIONS>(pDevice);
        nonpow2 = (d3d9opts.FullNonPow2TextureSupport) ? true : false;

        auto d3d9shadows = GetD3D11Options<D3D11_FEATURE_D3D9_SHADOW_SUPPORT, D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT>(pDevice);
        shadows = (d3d9shadows.SupportsDepthAsTextureWithLessEqualComparisonFilter) ? true : false;
    }


    //-----------------------------------------------------------------------------
    void CheckD3D9Ops1(ID3D11Device* pDevice, bool& nonpow2, bool& shadows, bool& instancing, bool& cubemapRT)
    {
        auto d3d9opts = GetD3D11Options<D3D11_FEATURE_D3D9_OPTIONS1, D3D11_FEATURE_DATA_D3D9_OPTIONS1>(pDevice);

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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        const WCHAR* shaderModel = nullptr;
        const WCHAR* computeShader = c_szNo;
        const WCHAR* maxTexDim = nullptr;
        const WCHAR* maxCubeDim = nullptr;
        const WCHAR* maxVolDim = nullptr;
        const WCHAR* maxTexRepeat = nullptr;
        const WCHAR* maxAnisotropy = nullptr;
        const WCHAR* maxPrimCount = L"4294967296";
        const WCHAR* maxInputSlots = nullptr;
        const WCHAR* mrt = nullptr;
        const WCHAR* extFormats = nullptr;
        const WCHAR* x2_10BitFormat = nullptr;
        const WCHAR* logic_ops = c_szNo;
        const WCHAR* cb_partial = c_szNA;
        const WCHAR* cb_offsetting = c_szNA;
        const WCHAR* uavSlots = nullptr;
        const WCHAR* uavEveryStage = nullptr;
        const WCHAR* uavOnlyRender = nullptr;
        const WCHAR* nonpow2 = nullptr;
        const WCHAR* bpp16 = c_szNo;
        const WCHAR* shadows = nullptr;
        const WCHAR* cubeRT = nullptr;
        const WCHAR* tiled_rsc = nullptr;
        const WCHAR* binding_rsc = nullptr;
        const WCHAR* minmaxfilter = nullptr;
        const WCHAR* mapdefaultbuff = nullptr;
        const WCHAR* consrv_rast = nullptr;
        const WCHAR* rast_ordered_views = nullptr;
        const WCHAR* ps_stencil_ref = nullptr;
        const WCHAR* instancing = nullptr;
        const WCHAR* vrs = nullptr;
        const WCHAR* meshShaders = nullptr;
        const WCHAR* dxr = nullptr;

        BOOL _10level9 = FALSE;

        switch (fl)
        {
        case D3D_FEATURE_LEVEL_12_2:
            if (pD3D12)
            {
                switch (GetD3D12ShaderModel(pD3D12))
                {
                case D3D_SHADER_MODEL_6_9:
                    shaderModel = L"6.9 (Optional)";
                    computeShader = L"Yes (CS 6.9)";
                    break;
                case D3D_SHADER_MODEL_6_8:
                    shaderModel = L"6.8 (Optional)";
                    computeShader = L"Yes (CS 6.8)";
                    break;
                case D3D_SHADER_MODEL_6_7:
                    shaderModel = L"6.7 (Optional)";
                    computeShader = L"Yes (CS 6.7)";
                    break;
                case D3D_SHADER_MODEL_6_6:
                    shaderModel = L"6.6 (Optional)";
                    computeShader = L"Yes (CS 6.6)";
                    break;
                default:
                    shaderModel = L"6.5";
                    computeShader = L"Yes (CS 6.5)";
                    break;
                }
                vrs = L"Yes - Tier 2";
                meshShaders = c_szYes;
                dxr = L"Yes - Tier 1.1";
            }
            // Fall-through

        case D3D_FEATURE_LEVEL_12_1:
        case D3D_FEATURE_LEVEL_12_0:
            if (!shaderModel)
            {
                switch (GetD3D12ShaderModel(pD3D12))
                {
                case D3D_SHADER_MODEL_6_9:
                    shaderModel = L"6.9 (Optional)";
                    computeShader = L"Yes (CS 6.9)";
                    break;
                case D3D_SHADER_MODEL_6_8:
                    shaderModel = L"6.8 (Optional)";
                    computeShader = L"Yes (CS 6.8)";
                    break;
                case D3D_SHADER_MODEL_6_7:
                    shaderModel = L"6.7 (Optional)";
                    computeShader = L"Yes (CS 6.7)";
                    break;
                case D3D_SHADER_MODEL_6_6:
                    shaderModel = L"6.6 (Optional)";
                    computeShader = L"Yes (CS 6.6)";
                    break;
                case D3D_SHADER_MODEL_6_5:
                    shaderModel = L"6.5 (Optional)";
                    computeShader = L"Yes (CS 6.5)";
                    break;
                case D3D_SHADER_MODEL_6_4:
                    shaderModel = L"6.4 (Optional)";
                    computeShader = L"Yes (CS 6.4)";
                    break;
                case D3D_SHADER_MODEL_6_3:
                    shaderModel = L"6.3 (Optional)";
                    computeShader = L"Yes (CS 6.3)";
                    break;
                case D3D_SHADER_MODEL_6_2:
                    shaderModel = L"6.2 (Optional)";
                    computeShader = L"Yes (CS 6.2)";
                    break;
                case D3D_SHADER_MODEL_6_1:
                    shaderModel = L"6.1 (Optional)";
                    computeShader = L"Yes (CS 6.1)";
                    break;
                case D3D_SHADER_MODEL_6_0:
                    shaderModel = L"6.0 (Optional)";
                    computeShader = L"Yes (CS 6.0)";
                    break;
                default:
                    shaderModel = L"5.1";
                    computeShader = L"Yes (CS 5.1)";
                    break;
                }
            }
            extFormats = c_szYes;
            x2_10BitFormat = c_szYes;
            logic_ops = c_szYes;
            cb_partial = c_szYes;
            cb_offsetting = c_szYes;
            uavEveryStage = c_szYes;
            uavOnlyRender = L"16";
            nonpow2 = L"Full";
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

                auto d3d12opts = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS, D3D12_FEATURE_DATA_D3D12_OPTIONS>(pD3D12);

                switch (d3d12opts.TiledResourcesTier)
                {
                    // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
                case D3D12_TILED_RESOURCES_TIER_2:  tiled_rsc = L"Yes - Tier 2"; break;
                case D3D12_TILED_RESOURCES_TIER_3:  tiled_rsc = L"Yes - Tier 3"; break;
                case D3D12_TILED_RESOURCES_TIER_4:  tiled_rsc = L"Yes - Tier 4"; break;
                default:                            tiled_rsc = c_szYes;        break;
                }

                switch (d3d12opts.ResourceBindingTier)
                {
                    // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
                case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = L"Yes - Tier 2"; break;
                case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = L"Yes - Tier 3"; break;
                default:                            binding_rsc = c_szYes;        break;
                }

                if (fl >= D3D_FEATURE_LEVEL_12_1)
                {
                    switch (d3d12opts.ConservativeRasterizationTier)
                    {
                        // 12.1 requires T1, 12.2 requires T3
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:   consrv_rast = L"Yes - Tier 1";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:   consrv_rast = L"Yes - Tier 2";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:   consrv_rast = L"Yes - Tier 3";  break;
                    default:                                        consrv_rast = c_szYes;         break;
                    }

                    rast_ordered_views = c_szYes;
                }
                else
                {
                    switch (d3d12opts.ConservativeRasterizationTier)
                    {
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:   consrv_rast = c_szOptNo; break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:               consrv_rast = L"Optional (Yes - Tier 1)";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:               consrv_rast = L"Optional (Yes - Tier 2)";  break;
                    case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:               consrv_rast = L"Optional (Yes - Tier 3)";  break;
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
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = L"Yes - Tier 2";  break;
                case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = L"Yes - Tier 3";  break;
                default:                                    tiled_rsc = c_szYes;         break;
                }

                if (fl >= D3D_FEATURE_LEVEL_12_1)
                {
                    switch (crast)
                    {
                        // 12.1 requires T1
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = L"Yes - Tier 1";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = L"Yes - Tier 2";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = L"Yes - Tier 3";  break;
                    default:                                                consrv_rast = c_szYes;         break;
                    }

                    rast_ordered_views = c_szYes;
                }
                else
                {
                    switch (crast)
                    {
                    case D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED:    consrv_rast = c_szOptNo; break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = L"Optional (Yes - Tier 1)";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = L"Optional (Yes - Tier 2)";  break;
                    case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = L"Optional (Yes - Tier 3)";  break;
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
            shaderModel = L"5.0";
            computeShader = L"Yes (CS 5.0)";
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
            uavOnlyRender = L"16";
            nonpow2 = L"Full";
            bpp16 = c_szYes;
            instancing = c_szYes;

            if (pD3D12)
            {
                shaderModel = L"5.1";
                computeShader = L"Yes (CS 5.1)";

                auto d3d12opts = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS, D3D12_FEATURE_DATA_D3D12_OPTIONS>(pD3D12);

                switch (d3d12opts.TiledResourcesTier)
                {
                case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:  tiled_rsc = c_szOptNo; break;
                case D3D12_TILED_RESOURCES_TIER_1:              tiled_rsc = L"Optional (Yes - Tier 1)";  break;
                case D3D12_TILED_RESOURCES_TIER_2:              tiled_rsc = L"Optional (Yes - Tier 2)";  break;
                case D3D12_TILED_RESOURCES_TIER_3:              tiled_rsc = L"Optional (Yes - Tier 3)";  break;
                case D3D12_TILED_RESOURCES_TIER_4:              tiled_rsc = L"Optional (Yes - Tier 4)";  break;
                default:                                        tiled_rsc = c_szOptYes; break;
                }

                switch (d3d12opts.ResourceBindingTier)
                {
                case D3D12_RESOURCE_BINDING_TIER_1: binding_rsc = L"Yes - Tier 1"; break;
                case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = L"Yes - Tier 2"; break;
                case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = L"Yes - Tier 3"; break;
                default:                            binding_rsc = c_szYes;        break;
                }

                switch (d3d12opts.ConservativeRasterizationTier)
                {
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:   consrv_rast = c_szOptNo; break;
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:               consrv_rast = L"Optional (Yes - Tier 1)";  break;
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:               consrv_rast = L"Optional (Yes - Tier 2)";  break;
                case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:               consrv_rast = L"Optional (Yes - Tier 3)";  break;
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
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = L"Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = L"Optional (Yes - Tier 2)";  break;
                case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = L"Optional (Yes - Tier 3)";  break;
                default:                                    tiled_rsc = c_szOptYes; break;
                }

                switch (crast)
                {
                case D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED:    consrv_rast = c_szOptNo; break;
                case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = L"Optional (Yes - Tier 1)";  break;
                case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = L"Optional (Yes - Tier 2)";  break;
                case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = L"Optional (Yes - Tier 3)";  break;
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
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = L"Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = L"Optional (Yes - Tier 2)";  break;
                default:                                    tiled_rsc = c_szOptYes; break;
                }

                minmaxfilter = bMinMaxFilter ? c_szOptYes : c_szOptNo;
                mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
            }
            break;

        case D3D_FEATURE_LEVEL_11_0:
            shaderModel = L"5.0";
            computeShader = L"Yes (CS 5.0)";
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
                shaderModel = L"5.1";
                computeShader = L"Yes (CS 5.1)";

                auto d3d12opts = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS, D3D12_FEATURE_DATA_D3D12_OPTIONS>(pD3D12);

                switch (d3d12opts.TiledResourcesTier)
                {
                case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:  tiled_rsc = c_szOptNo; break;
                case D3D12_TILED_RESOURCES_TIER_1:              tiled_rsc = L"Optional (Yes - Tier 1)";  break;
                case D3D12_TILED_RESOURCES_TIER_2:              tiled_rsc = L"Optional (Yes - Tier 2)";  break;
                case D3D12_TILED_RESOURCES_TIER_3:              tiled_rsc = L"Optional (Yes - Tier 3)";  break;
                case D3D12_TILED_RESOURCES_TIER_4:              tiled_rsc = L"Optional (Yes - Tier 4)";  break;
                default:                                        tiled_rsc = c_szOptYes; break;
                }

                switch (d3d12opts.ResourceBindingTier)
                {
                case D3D12_RESOURCE_BINDING_TIER_1: binding_rsc = L"Yes - Tier 1"; break;
                case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = L"Yes - Tier 2"; break;
                case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = L"Yes - Tier 3"; break;
                default:                            binding_rsc = c_szYes;        break;
                }

                logic_ops = d3d12opts.OutputMergerLogicOp ? c_szOptYes : c_szOptNo;
                consrv_rast = rast_ordered_views = ps_stencil_ref = minmaxfilter = c_szNo;
                mapdefaultbuff = c_szYes;

                uavSlots = L"8";
                uavEveryStage = c_szNo;
                uavOnlyRender = L"8";
                nonpow2 = L"Full";
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
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = L"Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = L"Optional (Yes - Tier 2)";  break;
                case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = L"Optional (Yes - Tier 3)";  break;
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
                case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = L"Optional (Yes - Tier 1)";  break;
                case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = L"Optional (Yes - Tier 2)";  break;
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

                uavSlots = L"8";
                uavEveryStage = c_szNo;
                uavOnlyRender = L"8";
                nonpow2 = L"Full";
            }
            break;

        case D3D_FEATURE_LEVEL_10_1:
            shaderModel = L"4.x";
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

                auto d3d10xhw = GetD3D11Options<D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS>(pD3D);

                if (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
                {
                    computeShader = L"Optional (Yes - CS 4.x)";
                    uavSlots = L"1";
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
                nonpow2 = L"Full";
                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11)
            {
                auto d3d10xhw = GetD3D11Options<D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS>(pD3D11);

                computeShader = (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) ? L"Optional (Yes - CS 4.x)" : c_szOptNo;

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
            shaderModel = L"4.0";
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
                auto d3d10xhw = GetD3D11Options<D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS>(pD3D);

                if (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
                {
                    computeShader = L"Optional (Yes - CS 4.x)";
                    uavSlots = L"1";
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
                nonpow2 = L"Full";
                bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;
            }
            else if (pD3D11)
            {
                auto d3d10xhw = GetD3D11Options<D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS>(pD3D11);

                computeShader = (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) ? L"Optional (Yes - CS 4.0)" : c_szOptNo;

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
            shaderModel = L"2.0 (4_0_level_9_3) [vs_2_a/ps_2_b]";
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
                nonpow2 = bNonPow2 ? L"Optional (Full)" : L"Conditional";
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
                nonpow2 = bNonPow2 ? L"Optional (Full)" : L"Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
            }
            break;

        case D3D_FEATURE_LEVEL_9_2:
            _10level9 = TRUE;
            shaderModel = L"2.0 (4_0_level_9_1)";
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
                nonpow2 = bNonPow2 ? L"Optional (Full)" : L"Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
                instancing = bInstancing ? L"Optional (Simple)" : c_szOptNo;
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
                nonpow2 = bNonPow2 ? L"Optional (Full)" : L"Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
            }
            break;

        case D3D_FEATURE_LEVEL_9_1:
            _10level9 = TRUE;
            shaderModel = L"2.0 (4_0_level_9_1)";
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
                nonpow2 = bNonPow2 ? L"Optional (Full)" : L"Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
                instancing = bInstancing ? L"Optional (Simple)" : c_szOptNo;
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
                nonpow2 = bNonPow2 ? L"Optional (Full)" : L"Conditional";
                shadows = bShadows ? c_szOptYes : c_szOptNo;
            }
            break;

        default:
            return E_FAIL;
        }

        if (d3dType == D3D_DRIVER_TYPE_WARP)
        {
            maxTexDim = (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1) ? L"16777216" : L"65536";
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
            LVLINE(L"Shader Model", shaderModel);
            LVYESNO(L"Geometry Shader", (fl >= D3D_FEATURE_LEVEL_10_0));
            LVYESNO(L"Stream Out", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                if (g_dwViewState == IDM_VIEWALL || computeShader != c_szNo)
                {
                    LVLINE(L"DirectCompute", computeShader);
                }

                LVYESNO(L"Hull & Domain Shaders", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            if (pD3D12)
            {
                LVLINE(L"Variable Rate Shading (VRS)", vrs);
                LVLINE(L"Mesh & Amplification Shaders", meshShaders);
                LVLINE(L"DirectX Raytracing", dxr);
            }

            LVYESNO(L"Texture Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (d3dVer != 0)
            {
                LVYESNO(L"Cubemap Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_1));
            }

            LVYESNO(L"BC4/BC5 Compression", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                LVYESNO(L"BC6H/BC7 Compression", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            LVYESNO(L"Alpha-to-coverage", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D11_1 || pD3D11_2 || pD3D11_3 || pD3D12)
            {
                LVLINE(L"Logic Ops (Output Merger)", logic_ops);
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                LVLINE(L"Constant Buffer Partial Updates", cb_partial);
                LVLINE(L"Constant Buffer Offsetting", cb_offsetting);

                if (uavEveryStage)
                {
                    LVLINE(L"UAVs at Every Stage", uavEveryStage);
                }

                if (uavOnlyRender)
                {
                    LVLINE(L"UAV-only rendering", uavOnlyRender);
                }
            }

            if (pD3D11_2 || pD3D11_3 || pD3D12)
            {
                if (tiled_rsc)
                {
                    LVLINE(L"Tiled Resources", tiled_rsc);
                }
            }

            if (pD3D11_2 || pD3D11_3)
            {
                if (minmaxfilter)
                {
                    LVLINE(L"Min/Max Filtering", minmaxfilter);
                }

                if (mapdefaultbuff)
                {
                    LVLINE(L"Map DEFAULT Buffers", mapdefaultbuff);
                }
            }

            if (pD3D11_3 || pD3D12)
            {
                if (consrv_rast)
                {
                    LVLINE(L"Conservative Rasterization", consrv_rast);
                }

                if (ps_stencil_ref)
                {
                    LVLINE(L"PS-Specified Stencil Ref", ps_stencil_ref);
                }

                if (rast_ordered_views)
                {
                    LVLINE(L"Rasterizer Ordered Views", rast_ordered_views);
                }
            }

            if (pD3D12 && binding_rsc)
            {
                LVLINE(L"Resource Binding", binding_rsc);
            }

            if (g_DXGIFactory1 && !pD3D12)
            {
                LVLINE(L"Extended Formats (BGRA, etc.)", extFormats);

                if (x2_10BitFormat && (g_dwViewState == IDM_VIEWALL || x2_10BitFormat != c_szNo))
                {
                    LVLINE(L"10-bit XR High Color Format", x2_10BitFormat);
                }
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                LVLINE(L"16-bit Formats (565/5551/4444)", bpp16);
            }

            if (nonpow2 && !pD3D12)
            {
                LVLINE(L"Non-Power-of-2 Textures", nonpow2);
            }

            LVLINE(L"Max Texture Dimension", maxTexDim);
            LVLINE(L"Max Cubemap Dimension", maxCubeDim);

            if (maxVolDim)
            {
                LVLINE(L"Max Volume Extent", maxVolDim);
            }

            if (maxTexRepeat)
            {
                LVLINE(L"Max Texture Repeat", maxTexRepeat);
            }

            LVLINE(L"Max Input Slots", maxInputSlots);

            if (uavSlots)
            {
                LVLINE(L"UAV Slots", uavSlots);
            }

            if (maxAnisotropy)
            {
                LVLINE(L"Max Anisotropy", maxAnisotropy);
            }

            LVLINE(L"Max Primitive Count", maxPrimCount);

            if (mrt)
            {
                LVLINE(L"Simultaneous Render Targets", mrt);
            }

            if (_10level9)
            {
                LVYESNO(L"Occlusion Queries", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO(L"Separate Alpha Blend", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO(L"Mirror Once", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO(L"Overlapping Vertex Elements", (fl >= D3D_FEATURE_LEVEL_9_2));
                LVYESNO(L"Independant Write Masks", (fl >= D3D_FEATURE_LEVEL_9_3));

                LVLINE(L"Instancing", instancing);

                if (pD3D11_1 || pD3D11_2 || pD3D11_3)
                {
                    LVLINE(L"Shadow Support", shadows);
                }

                if (pD3D11_2 || pD3D11_3)
                {
                    LVLINE(L"Cubemap Render w/ non-Cube Depth", cubeRT);
                }
            }

            LVLINE(L"Note", FL_NOTE);
        }
        else
        {
            PRINTLINE(L"Shader Model", shaderModel);
            PRINTYESNO(L"Geometry Shader", (fl >= D3D_FEATURE_LEVEL_10_0));
            PRINTYESNO(L"Stream Out", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                PRINTLINE(L"DirectCompute", computeShader);
                PRINTYESNO(L"Hull & Domain Shaders", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            if (pD3D12)
            {
                PRINTLINE(L"Variable Rate Shading (VRS)", vrs);
                PRINTLINE(L"Mesh & Amplification Shaders", meshShaders);
                PRINTLINE(L"DirectX Raytracing", dxr);
            }

            PRINTYESNO(L"Texture Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (d3dVer != 0)
            {
                PRINTYESNO(L"Cubemap Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_1));
            }

            PRINTYESNO(L"BC4/BC5 Compression", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
            {
                PRINTYESNO(L"BC6H/BC7 Compression", (fl >= D3D_FEATURE_LEVEL_11_0));
            }

            PRINTYESNO(L"Alpha-to-coverage", (fl >= D3D_FEATURE_LEVEL_10_0));

            if (pD3D11_1 || pD3D11_2 || pD3D11_3 || pD3D12)
            {
                PRINTLINE(L"Logic Ops (Output Merger)", logic_ops);
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                PRINTLINE(L"Constant Buffer Partial Updates", cb_partial);
                PRINTLINE(L"Constant Buffer Offsetting", cb_offsetting);

                if (uavEveryStage)
                {
                    PRINTLINE(L"UAVs at Every Stage", uavEveryStage);
                }

                if (uavOnlyRender)
                {
                    PRINTLINE(L"UAV-only rendering", uavOnlyRender);
                }
            }

            if (pD3D11_2 || pD3D11_3 || pD3D12)
            {
                if (tiled_rsc)
                {
                    PRINTLINE(L"Tiled Resources", tiled_rsc);
                }
            }

            if (pD3D11_2 || pD3D11_3)
            {
                if (minmaxfilter)
                {
                    PRINTLINE(L"Min/Max Filtering", minmaxfilter);
                }

                if (mapdefaultbuff)
                {
                    PRINTLINE(L"Map DEFAULT Buffers", mapdefaultbuff);
                }
            }

            if (pD3D11_3 || pD3D12)
            {
                if (consrv_rast)
                {
                    PRINTLINE(L"Conservative Rasterization", consrv_rast);
                }

                if (ps_stencil_ref)
                {
                    PRINTLINE(L"PS-Specified Stencil Ref ", ps_stencil_ref);
                }

                if (rast_ordered_views)
                {
                    PRINTLINE(L"Rasterizer Ordered Views", rast_ordered_views);
                }
            }

            if (pD3D12 && binding_rsc)
            {
                PRINTLINE(L"Resource Binding", binding_rsc);
            }

            if (g_DXGIFactory1 && !pD3D12)
            {
                PRINTLINE(L"Extended Formats (BGRA, etc.)", extFormats);

                if (x2_10BitFormat)
                {
                    PRINTLINE(L"10-bit XR High Color Format", x2_10BitFormat);
                }
            }

            if (pD3D11_1 || pD3D11_2 || pD3D11_3)
            {
                PRINTLINE(L"16-bit Formats (565/5551/4444)", bpp16);
            }

            if (nonpow2)
            {
                PRINTLINE(L"Non-Power-of-2 Textures", nonpow2);
            }

            PRINTLINE(L"Max Texture Dimension", maxTexDim);
            PRINTLINE(L"Max Cubemap Dimension", maxCubeDim);

            if (maxVolDim)
            {
                PRINTLINE(L"Max Volume Extent", maxVolDim);
            }

            if (maxTexRepeat)
            {
                PRINTLINE(L"Max Texture Repeat", maxTexRepeat);
            }

            PRINTLINE(L"Max Input Slots", maxInputSlots);

            if (uavSlots)
            {
                PRINTLINE(L"UAV Slots", uavSlots);
            }

            if (maxAnisotropy)
            {
                PRINTLINE(L"Max Anisotropy", maxAnisotropy);
            }

            PRINTLINE(L"Max Primitive Count", maxPrimCount);

            if (mrt)
            {
                PRINTLINE(L"Simultaneous Render Targets", mrt);
            }

            if (_10level9)
            {
                PRINTYESNO(L"Occlusion Queries", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO(L"Separate Alpha Blend", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO(L"Mirror Once", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO(L"Overlapping Vertex Elements", (fl >= D3D_FEATURE_LEVEL_9_2));
                PRINTYESNO(L"Independant Write Masks", (fl >= D3D_FEATURE_LEVEL_9_3));

                PRINTLINE(L"Instancing", instancing);

                if (pD3D11_1 || pD3D11_2 || pD3D11_3)
                {
                    PRINTLINE(L"Shadow Support", shadows);
                }

                if (pD3D11_2 || pD3D11_3)
                {
                    PRINTLINE(L"Cubemap Render w/ non-Cube Depth", cubeRT);
                }
            }

            PRINTLINE(L"Note", FL_NOTE);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);

            if (lParam2 == D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 25);
                LVAddColumn(g_hwndLV, 2, L"Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 60);
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
                    LVYESNO(L"Extended Formats (BGRA, etc.)", ext);
                    LVYESNO(L"10-bit XR High Color Format", x2);
                }

                LVLINE(L"Note", D3D10_NOTE);
            }
            else
            {
                if (g_DXGIFactory1)
                {
                    PRINTYESNO(L"Extended Formats (BGRA, etc.)", ext);
                    PRINTYESNO(L"10-bit XR High Color Format", x2);
                }

                PRINTLINE(L"Note", D3D10_NOTE);
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
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo);

                        WCHAR strBuffer[16];
                        swprintf_s(strBuffer, 16, L"%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    WCHAR strBuffer[32];
                    if (msaa)
                        swprintf_s(strBuffer, 32, L"Yes (%u)", quality);
                    else
                        wcscpy_s(strBuffer, 32, c_szNo);

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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);

            if (lParam2 == D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 25);
                LVAddColumn(g_hwndLV, 2, L"Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 60);
            }
        }

        if (!lParam2)
        {
            // General Direct3D 10.1 device information
            D3D10_FEATURE_LEVEL1 fl = pDevice->GetFeatureLevel();

            const WCHAR* szNote = nullptr;

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
                LVLINE(L"Feature Level", FLName(fl));

                if (g_DXGIFactory1 != nullptr && (fl >= D3D10_FEATURE_LEVEL_10_0))
                {
                    LVYESNO(L"Extended Formats (BGRA, etc.)", ext);
                    LVYESNO(L"10-bit XR High Color Format", x2);
                }

                LVLINE(L"Note", szNote);
            }
            else
            {
                PRINTLINE(L"Feature Level", FLName(fl));

                if (g_DXGIFactory1 != nullptr && (fl >= D3D10_FEATURE_LEVEL_10_0))
                {
                    PRINTYESNO(L"Extended Formats (BGRA, etc.)", ext);
                    PRINTYESNO(L"10-bit XR High Color Format", x2);
                }

                PRINTLINE(L"Note", szNote);
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

                            WCHAR  strBuffer[16];
                        swprintf_s(strBuffer, 16, L"%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    WCHAR strBuffer[32];
                    if (msaa)
                        swprintf_s(strBuffer, 32, L"Yes (%u)", quality);
                    else
                        wcscpy_s(strBuffer, 32, c_szNo);

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


    HRESULT D3D10InfoMSAA(LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D10Device*>(lParam1);
        if (!pDevice)
            return S_OK;

        auto sampCount = reinterpret_cast<BOOL*>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);

            UINT column = 1;
            for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
            {
                if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                    continue;

                WCHAR strBuffer[8];
                swprintf_s(strBuffer, 8, L"%ux", samples);
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
                        WCHAR strBuffer[32];
                        swprintf_s(strBuffer, 32, L"Yes (%u)", sampQ[samples - 1]);
                        LVAddText(g_hwndLV, column, strBuffer);
                    }
                    else
                        LVAddText(g_hwndLV, column, c_szNo);

                    ++column;
                }
            }
            else
            {
                WCHAR buff[1024];
                *buff = 0;

                for (UINT samples = 2; samples <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
                {
                    if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                        continue;

                    WCHAR strBuffer[32];
                    if (sampQ[samples - 1] > 0)
                        swprintf_s(strBuffer, 32, L"%ux: Yes (%u)   ", samples, sampQ[samples - 1]);
                    else
                        swprintf_s(strBuffer, 32, L"%ux: No   ", samples);

                    wcscat_s(buff, 1024, strBuffer);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);

            if (lParam2 == D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 25);
                LVAddColumn(g_hwndLV, 2, L"Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 60);
            }
        }

        if (!lParam2)
        {
            // General Direct3D 11 device information
            D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
            if (fl > D3D_FEATURE_LEVEL_11_0)
                fl = D3D_FEATURE_LEVEL_11_0;

            // CheckFeatureSupport
            auto threading = GetD3D11Options<D3D11_FEATURE_THREADING, D3D11_FEATURE_DATA_THREADING>(pDevice);
            auto doubles = GetD3D11Options<D3D11_FEATURE_DOUBLES, D3D11_FEATURE_DATA_DOUBLES>(pDevice);
            auto d3d10xhw = GetD3D11Options<D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS>(pDevice);

            // Setup note
            const WCHAR* szNote = nullptr;

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
                LVLINE(L"Feature Level", FLName(fl));

                LVYESNO(L"Driver Concurrent Creates", threading.DriverConcurrentCreates);
                LVYESNO(L"Driver Command Lists", threading.DriverCommandLists);
                LVYESNO(L"Double-precision Shaders", doubles.DoublePrecisionFloatShaderOps);
                LVYESNO(L"DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

                LVLINE(L"Note", szNote);
            }
            else
            {
                PRINTLINE(L"Feature Level", FLName(fl));

                PRINTYESNO(L"Driver Concurrent Creates", threading.DriverConcurrentCreates);
                PRINTYESNO(L"Driver Command Lists", threading.DriverCommandLists);
                PRINTYESNO(L"Double-precision Shaders", doubles.DoublePrecisionFloatShaderOps);
                PRINTYESNO(L"DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

                PRINTLINE(L"Note", szNote);
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
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo);

                        WCHAR strBuffer[16];
                        swprintf_s(strBuffer, 16, L"%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    WCHAR strBuffer[32];
                    if (msaa)
                        swprintf_s(strBuffer, 32, L"Yes (%u)", quality);
                    else
                        wcscpy_s(strBuffer, 32, c_szNo);

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
        auto threading = GetD3D11Options<D3D11_FEATURE_THREADING, D3D11_FEATURE_DATA_THREADING>(pDevice);
        auto doubles = GetD3D11Options<D3D11_FEATURE_DOUBLES, D3D11_FEATURE_DATA_DOUBLES>(pDevice);
        auto d3d10xhw = GetD3D11Options<D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS>(pDevice);
        auto d3d11opts = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS, D3D11_FEATURE_DATA_D3D11_OPTIONS>(pDevice);
        auto d3d9opts = GetD3D11Options<D3D11_FEATURE_D3D9_OPTIONS, D3D11_FEATURE_DATA_D3D9_OPTIONS>(pDevice);
        auto d3d11arch = GetD3D11Options<D3D11_FEATURE_ARCHITECTURE_INFO, D3D11_FEATURE_DATA_ARCHITECTURE_INFO>(pDevice);
        auto minprecis = GetD3D11Options<D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT>(pDevice);

        D3D11_FEATURE_DATA_D3D11_OPTIONS1 d3d11opts1 = {};
        if (bDev2)
        {
            d3d11opts1 = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS1, D3D11_FEATURE_DATA_D3D11_OPTIONS1>(pDevice);
        }

        const WCHAR* clearview = nullptr;
        if (d3d11opts.ClearView)
        {
            clearview = d3d11opts1.ClearViewAlsoSupportsDepthOnlyFormats ? L"RTV, UAV, and Depth (Driver sees)" : L"RTV and UAV (Driver sees)";
        }
        else
        {
            clearview = d3d11opts1.ClearViewAlsoSupportsDepthOnlyFormats ? L"RTV, UAV, and Depth (Driver doesn't see)" : L"RTV and UAV (Driver doesn't see)";
        }

        const WCHAR* double_shaders = nullptr;
        if (d3d11opts.ExtendedDoublesShaderInstructions)
        {
            double_shaders = L"Extended";
        }
        else if (doubles.DoublePrecisionFloatShaderOps)
        {
            double_shaders = c_szYes;
        }
        else
        {
            double_shaders = c_szNo;
        }

        const WCHAR* ps_precis = nullptr;
        switch (minprecis.PixelShaderMinPrecision & (D3D11_SHADER_MIN_PRECISION_16_BIT | D3D11_SHADER_MIN_PRECISION_10_BIT))
        {
        case 0:                                 ps_precis = L"Full";         break;
        case D3D11_SHADER_MIN_PRECISION_16_BIT: ps_precis = L"16/32-bit";    break;
        case D3D11_SHADER_MIN_PRECISION_10_BIT: ps_precis = L"10/32-bit";    break;
        default:                                ps_precis = L"10/16/32-bit"; break;
        }

        const WCHAR* other_precis = nullptr;
        switch (minprecis.AllOtherShaderStagesMinPrecision & (D3D11_SHADER_MIN_PRECISION_16_BIT | D3D11_SHADER_MIN_PRECISION_10_BIT))
        {
        case 0:                                 other_precis = L"Full";      break;
        case D3D11_SHADER_MIN_PRECISION_16_BIT: other_precis = L"16/32-bit"; break;
        case D3D11_SHADER_MIN_PRECISION_10_BIT: other_precis = L"10/32-bit"; break;
        default:                                other_precis = L"10/16/32-bit"; break;
        }

        const WCHAR* nonpow2 = (d3d9opts.FullNonPow2TextureSupport) ? L"Full" : L"Conditional";

        if (!pPrintInfo)
        {
            LVLINE(L"Feature Level", FLName(fl));

            LVYESNO(L"Driver Concurrent Creates", threading.DriverConcurrentCreates);
            LVYESNO(L"Driver Command Lists", threading.DriverCommandLists);
            LVLINE(L"Double-precision Shaders", double_shaders);
            LVYESNO(L"DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

            LVYESNO(L"Driver sees DiscardResource/View", d3d11opts.DiscardAPIsSeenByDriver);
            LVYESNO(L"Driver sees COPY_FLAGS", d3d11opts.FlagsForUpdateAndCopySeenByDriver);
            LVLINE(L"ClearView", clearview);
            LVYESNO(L"Copy w/ Overlapping Rect", d3d11opts.CopyWithOverlap);
            LVYESNO(L"CB Partial Update", d3d11opts.ConstantBufferPartialUpdate);
            LVYESNO(L"CB Offsetting", d3d11opts.ConstantBufferOffsetting);
            LVYESNO(L"Map NO_OVERWRITE on Dynamic CB", d3d11opts.MapNoOverwriteOnDynamicConstantBuffer);

            if (fl >= D3D_FEATURE_LEVEL_10_0)
            {
                LVYESNO(L"Map NO_OVERWRITE on Dynamic SRV", d3d11opts.MapNoOverwriteOnDynamicBufferSRV);
                LVYESNO(L"MSAA with ForcedSampleCount=1", d3d11opts.MultisampleRTVWithForcedSampleCountOne);
                LVYESNO(L"Extended resource sharing", d3d11opts.ExtendedResourceSharing);
            }

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                LVYESNO(L"Saturating Add Instruction", d3d11opts.SAD4ShaderInstructions);
            }

            LVYESNO(L"Tile-based Deferred Renderer", d3d11arch.TileBasedDeferredRenderer);

            LVLINE(L"Non-Power-of-2 Textures", nonpow2);

            LVLINE(L"Pixel Shader Precision", ps_precis);
            LVLINE(L"Other Stage Precision", other_precis);
        }
        else
        {
            PRINTLINE(L"Feature Level", FLName(fl));

            PRINTYESNO(L"Driver Concurrent Creates", threading.DriverConcurrentCreates);
            PRINTYESNO(L"Driver Command Lists", threading.DriverCommandLists);
            PRINTLINE(L"Double-precision Shaders", double_shaders);
            PRINTYESNO(L"DirectCompute CS 4.x", d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x);

            PRINTYESNO(L"Driver sees DiscardResource/View", d3d11opts.DiscardAPIsSeenByDriver);
            PRINTYESNO(L"Driver sees COPY_FLAGS", d3d11opts.FlagsForUpdateAndCopySeenByDriver);
            PRINTLINE(L"ClearView", clearview);
            PRINTYESNO(L"Copy w/ Overlapping Rect", d3d11opts.CopyWithOverlap);
            PRINTYESNO(L"CB Partial Update", d3d11opts.ConstantBufferPartialUpdate);
            PRINTYESNO(L"CB Offsetting", d3d11opts.ConstantBufferOffsetting);
            PRINTYESNO(L"Map NO_OVERWRITE on Dynamic CB", d3d11opts.MapNoOverwriteOnDynamicConstantBuffer);

            if (fl >= D3D_FEATURE_LEVEL_10_0)
            {
                PRINTYESNO(L"Map NO_OVERWRITE on Dynamic SRV", d3d11opts.MapNoOverwriteOnDynamicBufferSRV);
                PRINTYESNO(L"MSAA with ForcedSampleCount=1", d3d11opts.MultisampleRTVWithForcedSampleCountOne);
                PRINTYESNO(L"Extended resource sharing", d3d11opts.ExtendedResourceSharing);
            }

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                PRINTYESNO(L"Saturating Add Instruction", d3d11opts.SAD4ShaderInstructions);
            }

            PRINTYESNO(L"Tile-based Deferred Renderer", d3d11arch.TileBasedDeferredRenderer);

            PRINTLINE(L"Non-Power-of-2 Textures", nonpow2);

            PRINTLINE(L"Pixel Shader Precision", ps_precis);
            PRINTLINE(L"Other Stage Precision", other_precis);
        }
    }

    HRESULT D3D11Info1(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
    {
        auto pDevice = reinterpret_cast<ID3D11Device1*>(lParam1);
        if (!pDevice)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);

            if (lParam2 == D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 25);
                LVAddColumn(g_hwndLV, 2, L"Quality Level", 25);
            }
            else
            {
                LVAddColumn(g_hwndLV, 1, L"Value", 60);
            }
        }

        // General Direct3D 11.1 device information
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        if (!lParam2)
        {
            D3D11FeatureSupportInfo1(pDevice, false, pPrintInfo);

            // Setup note
            const WCHAR* szNote = nullptr;

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
                LVLINE(L"Note", szNote);
            }
            else
            {
                PRINTLINE(L"Note", szNote);
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
                        LVAddText(g_hwndLV, 1, msaa ? c_szYes : c_szNo);

                        WCHAR strBuffer[16];
                        swprintf_s(strBuffer, 16, L"%u", msaa ? quality : 0);
                        LVAddText(g_hwndLV, 2, strBuffer);
                    }
                }
                else
                {
                    WCHAR strBuffer[32];
                    if (msaa)
                        swprintf_s(strBuffer, 32, L"Yes (%u)", quality);
                    else
                        wcscpy_s(strBuffer, 32, c_szNo);

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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        // General Direct3D 11.2 device information
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        if (!lParam2)
        {
            D3D11FeatureSupportInfo1(pDevice, true, pPrintInfo);

            auto marker = GetD3D11Options<D3D11_FEATURE_MARKER_SUPPORT, D3D11_FEATURE_DATA_MARKER_SUPPORT>(pDevice);

            // Setup note
            const WCHAR* szNote = nullptr;

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
                LVYESNO(L"Profile Marker Support", marker.Profile);

                LVLINE(L"Note", szNote);
            }
            else
            {
                PRINTYESNO(L"Profile Marker Support", marker.Profile);

                PRINTLINE(L"Note", szNote);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        // General Direct3D 11.3/11.4 device information
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        if (!lParam2)
        {
            D3D11FeatureSupportInfo1(pDevice, true, pPrintInfo);

            auto marker = GetD3D11Options<D3D11_FEATURE_MARKER_SUPPORT, D3D11_FEATURE_DATA_MARKER_SUPPORT>(pDevice);
            auto d3d11opts2 = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS2, D3D11_FEATURE_DATA_D3D11_OPTIONS2>(pDevice);
            auto d3d11opts3 = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS3, D3D11_FEATURE_DATA_D3D11_OPTIONS3>(pDevice);
            auto d3d11opts4 = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS4, D3D11_FEATURE_DATA_D3D11_OPTIONS4>(pDevice);
            auto d3d11opts5 = GetD3D11Options<D3D11_FEATURE_D3D11_OPTIONS5, D3D11_FEATURE_DATA_D3D11_OPTIONS5>(pDevice);
            auto d3d11sc = GetD3D11Options<D3D11_FEATURE_SHADER_CACHE, D3D11_FEATURE_DATA_SHADER_CACHE>(pDevice);
            auto d3d11vm = GetD3D11Options<D3D11_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, D3D11_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT>(pDevice);

            // Setup note
            const WCHAR* szNote = nullptr;

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

            WCHAR tiled_rsc[16];
            if (d3d11opts2.TiledResourcesTier == D3D11_TILED_RESOURCES_NOT_SUPPORTED)
                wcscpy_s(tiled_rsc, L"Not supported");
            else
                swprintf_s(tiled_rsc, L"Tier %d", d3d11opts2.TiledResourcesTier);

            WCHAR consrv_rast[16];
            if (d3d11opts2.ConservativeRasterizationTier == D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED)
                wcscpy_s(consrv_rast, L"Not supported");
            else
                swprintf_s(consrv_rast, L"Tier %d", d3d11opts2.ConservativeRasterizationTier);

            const WCHAR* sharedResTier = nullptr;
            switch (d3d11opts5.SharedResourceTier)
            {
            case D3D11_SHARED_RESOURCE_TIER_0: sharedResTier = L"Yes - Tier 0"; break;
            case D3D11_SHARED_RESOURCE_TIER_1: sharedResTier = L"Yes - Tier 1"; break;
            case D3D11_SHARED_RESOURCE_TIER_2: sharedResTier = L"Yes - Tier 2"; break;
            case D3D11_SHARED_RESOURCE_TIER_3: sharedResTier = L"Yes - Tier 3"; break;
            default: sharedResTier = c_szYes; break;
            }

            WCHAR vmRes[16];
            swprintf_s(vmRes, L"%u", d3d11vm.MaxGPUVirtualAddressBitsPerResource);

            WCHAR vmProcess[16];
            swprintf_s(vmProcess, L"%u", d3d11vm.MaxGPUVirtualAddressBitsPerProcess);

            WCHAR shaderCache[32] = {};
            if (d3d11sc.SupportFlags)
            {
                if (d3d11sc.SupportFlags & D3D11_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE)
                {
                    wcscat_s(shaderCache, L"In-Proc ");
                }
                if (d3d11sc.SupportFlags & D3D11_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE)
                {
                    wcscat_s(shaderCache, L"Disk ");
                }
            }
            else
            {
                wcscpy_s(shaderCache, L"None");
            }

            if (!pPrintInfo)
            {
                LVYESNO(L"Profile Marker Support", marker.Profile);

                LVYESNO(L"Map DEFAULT Textures", d3d11opts2.MapOnDefaultTextures);
                LVYESNO(L"Standard Swizzle", d3d11opts2.StandardSwizzle);
                LVYESNO(L"Unified Memory Architecture (UMA)", d3d11opts2.UnifiedMemoryArchitecture);
                LVYESNO(L"Extended formats TypedUAVLoad", d3d11opts2.TypedUAVLoadAdditionalFormats);

                LVLINE(L"Conservative Rasterization", consrv_rast);
                LVLINE(L"Tiled Resources", tiled_rsc);
                LVYESNO(L"PS-Specified Stencil Ref", d3d11opts2.PSSpecifiedStencilRefSupported);
                LVYESNO(L"Rasterizer Ordered Views", d3d11opts2.ROVsSupported);

                LVYESNO(L"VP/RT from Rast-feeding Shader", d3d11opts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer);

                if (lParam3 != 0)
                {
                    LVLINE(L"Max GPU VM bits per resource", vmRes);
                    LVLINE(L"Max GPU VM bits per process", vmProcess);

                    LVYESNO(L"Extended Shared NV12", d3d11opts4.ExtendedNV12SharedTextureSupported);
                    LVLINE(L"Shader Cache", shaderCache);

                    LVLINE(L"Shared Resource Tier", sharedResTier);
                }

                LVLINE(L"Note", szNote);
            }
            else
            {
                PRINTYESNO(L"Profile Marker Support", marker.Profile);

                PRINTYESNO(L"Map DEFAULT Textures", d3d11opts2.MapOnDefaultTextures);
                PRINTYESNO(L"Standard Swizzle", d3d11opts2.StandardSwizzle);
                PRINTYESNO(L"Unified Memory Architecture (UMA)", d3d11opts2.UnifiedMemoryArchitecture);
                PRINTYESNO(L"Extended formats TypedUAVLoad", d3d11opts2.TypedUAVLoadAdditionalFormats);

                PRINTLINE(L"Conservative Rasterization", consrv_rast);
                PRINTLINE(L"Tiled Resources", tiled_rsc);
                PRINTYESNO(L"PS-Specified Stencil Ref", d3d11opts2.PSSpecifiedStencilRefSupported);
                PRINTYESNO(L"Rasterizer Ordered Views", d3d11opts2.ROVsSupported);

                PRINTYESNO(L"VP/RT from Rast-feeding Shader", d3d11opts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer);

                if (lParam3 != 0)
                {
                    PRINTLINE(L"Max GPU VM bits per resource", vmRes);
                    PRINTLINE(L"Max GPU VM bits per process", vmProcess);

                    PRINTYESNO(L"Extended Shared NV12", d3d11opts4.ExtendedNV12SharedTextureSupported);
                    PRINTLINE(L"Shader Cache", shaderCache);

                    PRINTLINE(L"Shared Resource Tier", sharedResTier);
                }

                PRINTLINE(L"Note", szNote);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);

            UINT column = 1;
            for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
            {
                if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                    continue;

                WCHAR strBuffer[8];
                swprintf_s(strBuffer, 8, L"%ux", samples);
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
                        WCHAR strBuffer[32];
                        swprintf_s(strBuffer, 32, L"Yes (%u)", sampQ[samples - 1]);
                        LVAddText(g_hwndLV, column, strBuffer);
                    }
                    else
                        LVAddText(g_hwndLV, column, c_szNo);

                    ++column;
                }
            }
            else
            {
                WCHAR buff[1024];
                *buff = 0;

                for (UINT samples = 2; samples <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++samples)
                {
                    if (!sampCount[samples - 1] && !IsMSAAPowerOf2(samples))
                        continue;

                    WCHAR strBuffer[32];
                    if (sampQ[samples - 1] > 0)
                        swprintf_s(strBuffer, 32, L"%ux: Yes (%u)   ", samples, sampQ[samples - 1]);
                    else
                        swprintf_s(strBuffer, 32, L"%ux: No   ", samples);

                    wcscat_s(buff, 1024, strBuffer);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Texture2D", 15);
            LVAddColumn(g_hwndLV, 2, L"Input", 15);
            LVAddColumn(g_hwndLV, 3, L"Output", 15);
            LVAddColumn(g_hwndLV, 4, L"Encoder", 15);
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
                WCHAR buff[1024];

                swprintf_s(buff, L"Texture2D: %-3s   Input: %-3s   Output: %-3s   Encoder: %-3s",
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        auto d3d12opts = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS, D3D12_FEATURE_DATA_D3D12_OPTIONS>(pDevice);
        auto d3d12opts2 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS2, D3D12_FEATURE_DATA_D3D12_OPTIONS2>(pDevice);
        auto d3d12opts3 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS3, D3D12_FEATURE_DATA_D3D12_OPTIONS3>(pDevice);
        auto d3d12opts4 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS4, D3D12_FEATURE_DATA_D3D12_OPTIONS4>(pDevice);
        auto d3d12opts5 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS5, D3D12_FEATURE_DATA_D3D12_OPTIONS5>(pDevice);
        auto d3d12opts6 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS6, D3D12_FEATURE_DATA_D3D12_OPTIONS6>(pDevice);

        auto d3d12eheaps = GetD3D12Options<D3D12_FEATURE_EXISTING_HEAPS, D3D12_FEATURE_DATA_EXISTING_HEAPS>(pDevice);
        auto d3d12serial = GetD3D12Options<D3D12_FEATURE_SERIALIZATION, D3D12_FEATURE_DATA_SERIALIZATION>(pDevice);

        const WCHAR* shaderModel = L"Unknown";
        switch (GetD3D12ShaderModel(pDevice))
        {
        case D3D_SHADER_MODEL_6_9: shaderModel = L"6.9"; break;
        case D3D_SHADER_MODEL_6_8: shaderModel = L"6.8"; break;
        case D3D_SHADER_MODEL_6_7: shaderModel = L"6.7"; break;
        case D3D_SHADER_MODEL_6_6: shaderModel = L"6.6"; break;
        case D3D_SHADER_MODEL_6_5: shaderModel = L"6.5"; break;
        case D3D_SHADER_MODEL_6_4: shaderModel = L"6.4"; break;
        case D3D_SHADER_MODEL_6_3: shaderModel = L"6.3"; break;
        case D3D_SHADER_MODEL_6_2: shaderModel = L"6.2"; break;
        case D3D_SHADER_MODEL_6_1: shaderModel = L"6.1"; break;
        case D3D_SHADER_MODEL_6_0: shaderModel = L"6.0"; break;
        case D3D_SHADER_MODEL_5_1: shaderModel = L"5.1"; break;
        }

        const WCHAR* rootSig = L"Unknown";
        switch (GetD3D12RootSignature(pDevice))
        {
        case D3D_ROOT_SIGNATURE_VERSION_1_0: rootSig = L"1.0"; break;
        case D3D_ROOT_SIGNATURE_VERSION_1_1: rootSig = L"1.1"; break;
        case D3D_ROOT_SIGNATURE_VERSION_1_2: rootSig = L"1.2"; break;
        }

        WCHAR heap[16];
        swprintf_s(heap, L"Tier %d", d3d12opts.ResourceHeapTier);

        WCHAR tiled_rsc[64] = {};
        if (d3d12opts.TiledResourcesTier == D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED)
            wcscpy_s(tiled_rsc, L"Not supported");
        else
        {
            swprintf_s(tiled_rsc, L"Tier %d", d3d12opts.TiledResourcesTier);

            if ((d3d12opts.TiledResourcesTier < D3D12_TILED_RESOURCES_TIER_3)
                && d3d12opts5.SRVOnlyTiledResourceTier3)
            {
                wcscat_s(tiled_rsc, L" (plus SRV only Volume textures)");
            }
        }

        WCHAR consrv_rast[16];
        if (d3d12opts.ConservativeRasterizationTier == D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED)
            wcscpy_s(consrv_rast, L"Not supported");
        else
            swprintf_s(consrv_rast, L"Tier %d", d3d12opts.ConservativeRasterizationTier);

        WCHAR binding_rsc[16];
        swprintf_s(binding_rsc, L"Tier %d", d3d12opts.ResourceBindingTier);

        const WCHAR* prgSamplePos = nullptr;
        switch (d3d12opts2.ProgrammableSamplePositionsTier)
        {
        case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED: prgSamplePos = c_szNo; break;
        case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_1: prgSamplePos = L"Yes - Tier 1"; break;
        case D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_2: prgSamplePos = L"Yes - Tier 2"; break;
        default: prgSamplePos = c_szYes; break;
        }

        const WCHAR* viewInstTier = nullptr;
        switch (d3d12opts3.ViewInstancingTier)
        {
        case D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED: viewInstTier = c_szNo; break;
        case D3D12_VIEW_INSTANCING_TIER_1: viewInstTier = L"Yes - Tier 1"; break;
        case D3D12_VIEW_INSTANCING_TIER_2: viewInstTier = L"Yes - Tier 2"; break;
        case D3D12_VIEW_INSTANCING_TIER_3: viewInstTier = L"Yes - Tier 3"; break;
        default: viewInstTier = c_szYes; break;
        }

        const WCHAR* renderPasses = nullptr;
        switch (d3d12opts5.RenderPassesTier)
        {
        case D3D12_RENDER_PASS_TIER_0: renderPasses = c_szNo; break;
        case D3D12_RENDER_PASS_TIER_1: renderPasses = L"Yes - Tier 1"; break;
        case D3D12_RENDER_PASS_TIER_2: renderPasses = L"Yes - Tier 2"; break;
        default: renderPasses = c_szYes; break;
        }

        const WCHAR* dxr = nullptr;
        switch (d3d12opts5.RaytracingTier)
        {
        case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: dxr = c_szNo; break;
        case D3D12_RAYTRACING_TIER_1_0: dxr = L"Yes - Tier 1.0"; break;
        case D3D12_RAYTRACING_TIER_1_1: dxr = L"Yes - Tier 1.1"; break;
        default: dxr = c_szYes; break;
        }

        const WCHAR* heapSerial = nullptr;
        switch (d3d12serial.HeapSerializationTier)
        {
        case D3D12_HEAP_SERIALIZATION_TIER_0: heapSerial = c_szNo; break;
        case D3D12_HEAP_SERIALIZATION_TIER_10: heapSerial = L"Yes - Tier 10"; break;
        default: heapSerial = c_szYes; break;
        }

#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts8 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS8, D3D12_FEATURE_DATA_D3D12_OPTIONS8>(pDevice);
#endif

#if defined(NTDDI_WIN10_NI) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts12 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS12, D3D12_FEATURE_DATA_D3D12_OPTIONS12>(pDevice);
        auto d3d12opts13 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS13, D3D12_FEATURE_DATA_D3D12_OPTIONS13>(pDevice);

        WCHAR vp_flips[16] = {};
        if (d3d12opts13.InvertedViewportHeightFlipsYSupported)
        {
            wcscat_s(vp_flips, L"Y ");
        }
        if (d3d12opts13.InvertedViewportDepthFlipsZSupported)
        {
            wcscat_s(vp_flips, L"Z ");
        }
        if (vp_flips[0] == 0)
        {
            wcscpy_s(vp_flips, c_szNo);
        }
#endif

#if defined(NTDDI_WIN10_CU) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts14 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS14, D3D12_FEATURE_DATA_D3D12_OPTIONS14>(pDevice);
        auto d3d12opts15 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS15, D3D12_FEATURE_DATA_D3D12_OPTIONS15>(pDevice);
        auto d3d12opts16 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS16, D3D12_FEATURE_DATA_D3D12_OPTIONS16>(pDevice);
#endif

#if defined(NTDDI_WIN11_GE) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts17 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS17, D3D12_FEATURE_DATA_D3D12_OPTIONS17>(pDevice);
        auto d3d12opts18 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS18, D3D12_FEATURE_DATA_D3D12_OPTIONS18>(pDevice);
        auto d3d12opts19 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS19, D3D12_FEATURE_DATA_D3D12_OPTIONS19>(pDevice);
        auto d3d12opts20 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS20, D3D12_FEATURE_DATA_D3D12_OPTIONS20>(pDevice);
#endif

#if defined(NTDDI_WIN11_DT) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts21 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS21, D3D12_FEATURE_DATA_D3D12_OPTIONS21>(pDevice);
#endif

        if (!pPrintInfo)
        {
            LVLINE(L"Feature Level", FLName(fl));
            LVLINE(L"Shader Model", shaderModel);
            LVLINE(L"Root Signature", rootSig);

            LVYESNO(L"Standard Swizzle 64KB", d3d12opts.StandardSwizzle64KBSupported);
            LVYESNO(L"Extended formats TypedUAVLoad", d3d12opts.TypedUAVLoadAdditionalFormats);

            LVLINE(L"Conservative Rasterization", consrv_rast);
            LVLINE(L"Resource Binding", binding_rsc);
            LVLINE(L"Tiled Resources", tiled_rsc);
            LVLINE(L"Resource Heap", heap);
            LVYESNO(L"Rasterizer Ordered Views", d3d12opts.ROVsSupported);

            LVYESNO(L"VP/RT without GS Emulation", d3d12opts.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation);

            LVYESNO(L"Depth bound test supported", d3d12opts2.DepthBoundsTestSupported);
            LVLINE(L"Programmable Sample Positions", prgSamplePos);

            LVLINE(L"View Instancing", viewInstTier);

            LVYESNO(L"Casting fully typed formats", d3d12opts3.CastingFullyTypedFormatSupported);
            LVYESNO(L"Copy queue timestamp queries", d3d12opts3.CopyQueueTimestampQueriesSupported);

            LVYESNO(L"Create heaps from existing system memory", d3d12eheaps.Supported);

            LVYESNO(L"64KB aligned MSAA textures", d3d12opts4.MSAA64KBAlignedTextureSupported);

            LVLINE(L"Heap serialization", heapSerial);

            LVLINE(L"Render Passes", renderPasses);

            LVLINE(L"DirectX Raytracing", dxr);

            LVYESNO(L"Background processing supported", d3d12opts6.BackgroundProcessingSupported);

#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
            LVYESNO(L"Unaligned BC texture (not a multiple of 4)", d3d12opts8.UnalignedBlockTexturesSupported);
#endif

#if defined(NTDDI_WIN10_NI) || defined(USING_D3D12_AGILITY_SDK)
            LVYESNO(L"Enhanced Barriers", d3d12opts12.EnhancedBarriersSupported);
            LVYESNO(L"Relaxed format casting", d3d12opts12.RelaxedFormatCastingSupported);
            LVYESNO(L"Alpha blend factor support", d3d12opts13.AlphaBlendFactorSupported);
            LVYESNO(L"Unrestricted buffer/texture copy pitch", d3d12opts13.UnrestrictedBufferTextureCopyPitchSupported);
            LVYESNO(L"Unrestricted vertex alignment", d3d12opts13.UnrestrictedVertexElementAlignmentSupported);
            LVYESNO(L"Copy between textures of any dimension", d3d12opts13.TextureCopyBetweenDimensionsSupported);
            LVLINE(L"Inverted viewport flips support", vp_flips)
#endif

#if defined(NTDDI_WIN10_CU) || defined(USING_D3D12_AGILITY_SDK)
            LVYESNO(L"Independent front/back stencil refmask", d3d12opts14.IndependentFrontAndBackStencilRefMaskSupported);
            LVYESNO(L"Triangle fans primitives", d3d12opts15.TriangleFanSupported);
            LVYESNO(L"Dynamic IB strip-cut support", d3d12opts15.DynamicIndexBufferStripCutSupported);
            LVYESNO(L"Dynamic depth bias support", d3d12opts16.DynamicDepthBiasSupported);
            LVYESNO(L"GPU upload heap support", d3d12opts16.GPUUploadHeapSupported);
#endif

#if defined(NTDDI_WIN11_GE) || defined(USING_D3D12_AGILITY_SDK)
            LVYESNO(L"Non-normalized coordinate samplers", d3d12opts17.NonNormalizedCoordinateSamplersSupported);
            LVYESNO(L"Manual write tracking res", d3d12opts17.ManualWriteTrackingResourceSupported);
#endif
        }
        else
        {
            PRINTLINE(L"Feature Level", FLName(fl));
            PRINTLINE(L"Shader Model", shaderModel);
            PRINTLINE(L"Root Signature", rootSig);

            PRINTYESNO(L"Standard Swizzle 64KB", d3d12opts.StandardSwizzle64KBSupported);
            PRINTYESNO(L"Extended formats TypedUAVLoad", d3d12opts.TypedUAVLoadAdditionalFormats);

            PRINTLINE(L"Conservative Rasterization", consrv_rast);
            PRINTLINE(L"Resource Binding", binding_rsc);
            PRINTLINE(L"Tiled Resources", tiled_rsc);
            PRINTLINE(L"Resource Heap", heap);
            PRINTYESNO(L"Rasterizer Ordered Views", d3d12opts.ROVsSupported);

            PRINTYESNO(L"VP/RT without GS Emulation", d3d12opts.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation);

            PRINTYESNO(L"Depth bound test supported", d3d12opts2.DepthBoundsTestSupported);
            PRINTLINE(L"Programmable Sample Positions", prgSamplePos);

            PRINTLINE(L"View Instancing", viewInstTier);

            PRINTYESNO(L"Casting fully typed formats", d3d12opts3.CastingFullyTypedFormatSupported);
            PRINTYESNO(L"Copy queue timestamp queries", d3d12opts3.CopyQueueTimestampQueriesSupported);

            PRINTYESNO(L"Create heaps from existing system memory", d3d12eheaps.Supported);

            PRINTYESNO(L"64KB aligned MSAA textures", d3d12opts4.MSAA64KBAlignedTextureSupported);

            PRINTLINE(L"Heap serialization", heapSerial);

            PRINTLINE(L"Render Passes", renderPasses);

            PRINTLINE(L"DirectX Raytracing", dxr);

            PRINTYESNO(L"Background processing supported", d3d12opts6.BackgroundProcessingSupported);

#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
            PRINTYESNO(L"Unaligned BC texture (not a multiple of 4)", d3d12opts8.UnalignedBlockTexturesSupported);
#endif

#if defined(NTDDI_WIN10_NI) || defined(USING_D3D12_AGILITY_SDK)
            PRINTYESNO(L"Enhanced Barriers", d3d12opts12.EnhancedBarriersSupported);
            PRINTYESNO(L"Relaxed format casting", d3d12opts12.RelaxedFormatCastingSupported);
            PRINTYESNO(L"Alpha blend factor support", d3d12opts13.AlphaBlendFactorSupported);
            PRINTYESNO(L"Unrestricted buffer/texture copy pitch", d3d12opts13.UnrestrictedBufferTextureCopyPitchSupported);
            PRINTYESNO(L"Unrestricted vertex alignment", d3d12opts13.UnrestrictedVertexElementAlignmentSupported);
            PRINTYESNO(L"Copy between textures of any dimension", d3d12opts13.TextureCopyBetweenDimensionsSupported);
            PRINTLINE(L"Inverted viewport flips support", vp_flips);
#endif

#if defined(NTDDI_WIN10_CU) || defined(USING_D3D12_AGILITY_SDK)
            PRINTYESNO(L"Independent front/back stencil refmask", d3d12opts14.IndependentFrontAndBackStencilRefMaskSupported);
            PRINTYESNO(L"Triangle fan primitives", d3d12opts15.TriangleFanSupported);
            PRINTYESNO(L"Dynamic IB strip-cut support", d3d12opts15.DynamicIndexBufferStripCutSupported);
            PRINTYESNO(L"Dynamic depth bias support", d3d12opts16.DynamicDepthBiasSupported);
            PRINTYESNO(L"GPU upload heap support", d3d12opts16.GPUUploadHeapSupported);
#endif

#if defined(NTDDI_WIN11_GE) || defined(USING_D3D12_AGILITY_SDK)
            PRINTYESNO(L"Non-normalized coordinate samplers", d3d12opts17.NonNormalizedCoordinateSamplersSupported);
            PRINTYESNO(L"Manual write tracking res", d3d12opts17.ManualWriteTrackingResourceSupported);
#endif
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        auto d3d12arch = GetD3D12Options<D3D12_FEATURE_ARCHITECTURE, D3D12_FEATURE_DATA_ARCHITECTURE>(pDevice);

        bool usearch1 = false;
        D3D12_FEATURE_DATA_ARCHITECTURE1 d3d12arch1 = {};
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &d3d12arch1, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));
        if (SUCCEEDED(hr))
            usearch1 = true;
        else
            memset(&d3d12arch1, 0, sizeof(d3d12arch1));

        auto d3d12vm = GetD3D12Options<D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT>(pDevice);

        WCHAR vmRes[16];
        swprintf_s(vmRes, 16, L"%u", d3d12vm.MaxGPUVirtualAddressBitsPerResource);

        WCHAR vmProcess[16];
        swprintf_s(vmProcess, 16, L"%u", d3d12vm.MaxGPUVirtualAddressBitsPerProcess);

        if (!pPrintInfo)
        {
            LVYESNO(L"Tile-based Renderer", d3d12arch.TileBasedRenderer);
            LVYESNO(L"Unified Memory Architecture (UMA)", d3d12arch.UMA);
            LVYESNO(L"Cache Coherent UMA", d3d12arch.CacheCoherentUMA);
            if (usearch1)
            {
                LVYESNO(L"Isolated MMU", d3d12arch1.IsolatedMMU);
            }

            LVLINE(L"Max GPU VM bits per resource", vmRes);
            LVLINE(L"Max GPU VM bits per process", vmProcess);
        }
        else
        {
            PRINTYESNO(L"Tile-based Renderer", d3d12arch.TileBasedRenderer);
            PRINTYESNO(L"Unified Memory Architecture (UMA)", d3d12arch.UMA);
            PRINTYESNO(L"Cache Coherent UMA", d3d12arch.CacheCoherentUMA);
            if (usearch1)
            {
                PRINTYESNO(L"Isolated MMU", d3d12arch1.IsolatedMMU);
            }

            PRINTLINE(L"Max GPU VM bits per resource", vmRes);
            PRINTLINE(L"Max GPU VM bits per process", vmProcess);
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        auto d3d12opts = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS, D3D12_FEATURE_DATA_D3D12_OPTIONS>(pDevice);
        auto d3d12opts1 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS1, D3D12_FEATURE_DATA_D3D12_OPTIONS1>(pDevice);
        auto d3d12opts3 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS3, D3D12_FEATURE_DATA_D3D12_OPTIONS3>(pDevice);
        auto d3d12opts4 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS4, D3D12_FEATURE_DATA_D3D12_OPTIONS4>(pDevice);
        auto d3d12opts6 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS6, D3D12_FEATURE_DATA_D3D12_OPTIONS6>(pDevice);
        auto d3d12opts7 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS7, D3D12_FEATURE_DATA_D3D12_OPTIONS7>(pDevice);

        auto d3d12sc = GetD3D12Options<D3D12_FEATURE_SHADER_CACHE, D3D12_FEATURE_DATA_SHADER_CACHE>(pDevice);

        const WCHAR* precis = nullptr;
        switch (d3d12opts.MinPrecisionSupport & (D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT | D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT))
        {
        case 0:                                         precis = L"Full";         break;
        case D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT: precis = L"16/32-bit";    break;
        case D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT: precis = L"10/32-bit";    break;
        default:                                        precis = L"10/16/32-bit"; break;
        }

        WCHAR lane_count[16];
        swprintf_s(lane_count, L"%u", d3d12opts1.TotalLaneCount);

        WCHAR wave_lane_count[16];
        swprintf_s(wave_lane_count, L"%u/%u", d3d12opts1.WaveLaneCountMin, d3d12opts1.WaveLaneCountMax);

        const WCHAR* vrs = nullptr;
        switch (d3d12opts6.VariableShadingRateTier)
        {
        case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: vrs = c_szNo; break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_1: vrs = L"Yes - 1"; break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_2: vrs = L"Yes - 2"; break;
        default: vrs = c_szYes; break;
        }

        WCHAR vrs_tile_size[16];
        swprintf_s(vrs_tile_size, L"%u", d3d12opts6.ShadingRateImageTileSize);

        const WCHAR* meshShaders = nullptr;
        switch (d3d12opts7.MeshShaderTier)
        {
        case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED:  meshShaders = c_szNo; break;
        case D3D12_MESH_SHADER_TIER_1:              meshShaders = L"Yes - Tier 1"; break;
        default:                                    meshShaders = c_szYes; break;
        }

        const WCHAR* feedbackTier = nullptr;
        switch (d3d12opts7.SamplerFeedbackTier)
        {
        case D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED:  feedbackTier = c_szNo; break;
        case D3D12_SAMPLER_FEEDBACK_TIER_0_9:            feedbackTier = L"Yes - Tier 0.9"; break;
        case D3D12_SAMPLER_FEEDBACK_TIER_1_0:            feedbackTier = L"Yes - Tier 1"; break;
        default:                                         feedbackTier = c_szYes; break;
        }

        const WCHAR* wavemmatier = nullptr;
        const WCHAR* msderiv = nullptr;
        const WCHAR* msrtarrayindex = nullptr;
        WCHAR atomicInt64[64] = {};
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts9 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS9, D3D12_FEATURE_DATA_D3D12_OPTIONS9>(pDevice);

        if (d3d12opts9.AtomicInt64OnTypedResourceSupported)
        {
            wcscat_s(atomicInt64, L"Typed ");
        }

        if (d3d12opts9.AtomicInt64OnGroupSharedSupported)
        {
            wcscat_s(atomicInt64, L"GroupShared ");
        }

        msderiv = (d3d12opts9.DerivativesInMeshAndAmplificationShadersSupported) ? c_szYes : c_szNo;
        msrtarrayindex = (d3d12opts9.MeshShaderSupportsFullRangeRenderTargetArrayIndex) ? c_szYes : c_szNo;

        switch (d3d12opts9.WaveMMATier)
        {
        case D3D12_WAVE_MMA_TIER_NOT_SUPPORTED: wavemmatier = c_szNo; break;
        case D3D12_WAVE_MMA_TIER_1_0:           wavemmatier = L"Yes - Tier 1"; break;
        default:                                wavemmatier = c_szYes; break;
        }
#endif // FE

        WCHAR shaderCache[64] = {};
        if (d3d12sc.SupportFlags)
        {
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO)
            {
                wcscat_s(shaderCache, L"SinglePSO ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_LIBRARY)
            {
                wcscat_s(shaderCache, L"Library ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE)
            {
                wcscat_s(shaderCache, L"In-Proc ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE)
            {
                wcscat_s(shaderCache, L"Disk ");
            }
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_DRIVER_MANAGED_CACHE)
            {
                wcscat_s(shaderCache, L"DrvMng ");
            }
#endif // FE
#if defined(NTDDI_WIN10_CO) || defined(USING_D3D12_AGILITY_SDK)
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_SHADER_CONTROL_CLEAR)
            {
                wcscat_s(shaderCache, L"Clear ");
            }
            if (d3d12sc.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_SHADER_SESSION_DELETE)
            {
                wcscat_s(shaderCache, L"Delete ");
            }
#endif // CO
        }
        else
        {
            wcscpy_s(shaderCache, L"None");
        }

        const WCHAR* vrssum = nullptr;
        const WCHAR* msperprim = nullptr;
#if defined(NTDDI_WIN10_CO) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts10 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS10, D3D12_FEATURE_DATA_D3D12_OPTIONS10 >(pDevice);
        vrssum = (d3d12opts10.VariableRateShadingSumCombinerSupported) ? c_szYes : c_szNo;
        msperprim = (d3d12opts10.MeshShaderPerPrimitiveShadingRateSupported) ? c_szYes : c_szNo;

        auto d3d12opts11 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS11, D3D12_FEATURE_DATA_D3D12_OPTIONS11>(pDevice);
        if (d3d12opts11.AtomicInt64OnDescriptorHeapResourceSupported)
        {
            wcscat_s(atomicInt64, L"DescHeap ");
        }
#endif // CO

        const WCHAR* ms_stats_culled = nullptr;
#if defined(NTDDI_WIN10_NI) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts12 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS12, D3D12_FEATURE_DATA_D3D12_OPTIONS12>(pDevice);
        ms_stats_culled = d3d12opts12.MSPrimitivesPipelineStatisticIncludesCulledPrimitives ? c_szYes : c_szNo;
#endif

        const WCHAR* adv_texture_ops = nullptr;
        const WCHAR* writeable_msaa_txt = nullptr;
#if defined(NTDDI_WIN10_CU) || defined(USING_D3D12_AGILITY_SDK)
        auto d3d12opts14 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS14, D3D12_FEATURE_DATA_D3D12_OPTIONS14>(pDevice);
        adv_texture_ops = d3d12opts14.AdvancedTextureOpsSupported ? c_szYes : c_szNo;
        writeable_msaa_txt = d3d12opts14.WriteableMSAATexturesSupported ? c_szYes : c_szNo;
#endif

        const WCHAR* nonNormalizedCoords = nullptr;
#if defined(NTDDI_WIN10_CU)
        auto d3d12opts17 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS17, D3D12_FEATURE_DATA_D3D12_OPTIONS17>(pDevice);
        nonNormalizedCoords = d3d12opts17.NonNormalizedCoordinateSamplersSupported ? c_szYes : c_szNo;
#endif

        if (!pPrintInfo)
        {
            LVYESNO(L"Double-precision Shaders", d3d12opts.DoublePrecisionFloatShaderOps);

            LVLINE(L"Minimum Precision", precis);
            LVYESNO(L"Native 16-bit Shader Ops", d3d12opts4.Native16BitShaderOpsSupported);

            LVYESNO(L"Wave operations", d3d12opts1.WaveOps);
            if (d3d12opts1.WaveOps)
            {
                LVLINE(L"Wave lane count", wave_lane_count);
                LVLINE(L"Total lane count", lane_count);
                LVYESNO(L"Expanded compute resource states", d3d12opts1.ExpandedComputeResourceStates);
            }

            if (wavemmatier)
            {
                LVLINE(L"Wave MMA", wavemmatier);
            }

            LVYESNO(L"PS-Specified Stencil Ref", d3d12opts.PSSpecifiedStencilRefSupported);

            LVYESNO(L"Barycentrics", d3d12opts3.BarycentricsSupported);

            if (*atomicInt64)
            {
                LVLINE(L"atomic<int64>", atomicInt64);
            }

            LVLINE(L"Variable Rate Shading (VRS)", vrs);
            if (d3d12opts6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
            {
                LVYESNO(L"VRS: Additional shading rates", d3d12opts6.AdditionalShadingRatesSupported);
                LVYESNO(L"VRS: Per-primitive SV_ViewportIndex", d3d12opts6.PerPrimitiveShadingRateSupportedWithViewportIndexing);
                LVLINE(L"VRS: Screen-space tile size", vrs_tile_size);

                if (vrssum)
                {
                    LVLINE(L"VRS: Sum combiner", vrssum);
                }
            }

            LVLINE(L"Mesh & Amplification Shaders", meshShaders);
            if (d3d12opts7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
            {
                if (msderiv)
                {
                    LVLINE(L"MS/AS: Derivatives Support", msderiv);
                }

                if (msperprim)
                {
                    LVLINE(L"MS: Per-Primitive Shading", msperprim);
                }

                if (msrtarrayindex)
                {
                    LVLINE(L"MS: RT Array Index Support", msrtarrayindex);
                }

                if (ms_stats_culled)
                {
                    LVLINE(L"MS: Stats incl culled prims", ms_stats_culled);
                }
            }

            LVLINE(L"Sampler Feedback", feedbackTier);

            LVLINE(L"Shader Cache", shaderCache);

            if (adv_texture_ops)
            {
                LVLINE(L"Advanced texture ops", adv_texture_ops);
            }

            if (writeable_msaa_txt)
            {
                LVLINE(L"Writeable MSAA textures", writeable_msaa_txt);
            }

            if (nonNormalizedCoords)
            {
                LVLINE(L"Non-normalized sampler coordinates", nonNormalizedCoords);
            }
        }
        else
        {
            PRINTYESNO(L"Double-precision Shaders", d3d12opts.DoublePrecisionFloatShaderOps);

            PRINTLINE(L"Minimum Precision", precis);
            PRINTYESNO(L"Native 16-bit Shader Ops", d3d12opts4.Native16BitShaderOpsSupported);

            PRINTYESNO(L"Wave operations", d3d12opts1.WaveOps);
            if (d3d12opts1.WaveOps)
            {
                PRINTLINE(L"Wave lane count", wave_lane_count);
                PRINTLINE(L"Total lane count", lane_count);
                PRINTYESNO(L"Expanded compute resource states", d3d12opts1.ExpandedComputeResourceStates);
            }

            if (wavemmatier)
            {
                PRINTLINE(L"Wave MMA", wavemmatier);
            }

            PRINTYESNO(L"PS-Specified Stencil Ref", d3d12opts.PSSpecifiedStencilRefSupported);

            PRINTYESNO(L"Barycentrics", d3d12opts3.BarycentricsSupported);

            if (*atomicInt64)
            {
                PRINTLINE(L"atomic<int64>", atomicInt64);
            }

            PRINTLINE(L"Variable Rate Shading (VRS)", vrs);
            if (d3d12opts6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
            {
                PRINTYESNO(L"VRS: Additional shading rates", d3d12opts6.AdditionalShadingRatesSupported);
                PRINTYESNO(L"VRS: Per-primitive SV_ViewportIndex", d3d12opts6.PerPrimitiveShadingRateSupportedWithViewportIndexing);
                PRINTLINE(L"VRS: Screen-space tile size", vrs_tile_size);

                if (vrssum)
                {
                    PRINTLINE(L"VRS: Sum combiner", vrssum);
                }
            }

            PRINTLINE(L"Mesh & Amplification Shaders", meshShaders);
            if (d3d12opts7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
            {
                if (msderiv)
                {
                    PRINTLINE(L"MS/AS: Derivatives Support", msderiv);
                }

                if (msperprim)
                {
                    PRINTLINE(L"MS: Per-Primitive Shading", msperprim);
                }

                if (msrtarrayindex)
                {
                    PRINTLINE(L"MS: RT Array Index Support", msrtarrayindex);
                }

                if (ms_stats_culled)
                {
                    PRINTLINE(L"MS: Stats incl culled prims", ms_stats_culled);
                }
            }

            PRINTLINE(L"Sampler Feedback", feedbackTier);

            PRINTLINE(L"Shader Cache", shaderCache);

            if (adv_texture_ops)
            {
                PRINTLINE(L"Advanced texture ops", adv_texture_ops);
            }

            if (writeable_msaa_txt)
            {
                PRINTLINE(L"Writeable MSAA textures", writeable_msaa_txt);
            }

            if (nonNormalizedCoords)
            {
                PRINTLINE(L"Non-normalized sampler coordinates", nonNormalizedCoords);
            }
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Value", 60);
        }

        auto d3d12opts = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS, D3D12_FEATURE_DATA_D3D12_OPTIONS>(pDevice);
        auto d3d12opts4 = GetD3D12Options<D3D12_FEATURE_D3D12_OPTIONS4, D3D12_FEATURE_DATA_D3D12_OPTIONS4>(pDevice);

        auto d3d12xnode = GetD3D12Options<D3D12_FEATURE_CROSS_NODE, D3D12_FEATURE_DATA_CROSS_NODE>(pDevice);

        WCHAR sharing[16];
        switch (d3d12opts.CrossNodeSharingTier)
        {
        case D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED:   wcscpy_s(sharing, c_szNo); break;
        case D3D12_CROSS_NODE_SHARING_TIER_1_EMULATED:      wcscpy_s(sharing, L"Yes - Tier 1 (Emulated)"); break;
        case D3D12_CROSS_NODE_SHARING_TIER_1:               wcscpy_s(sharing, L"Yes - Tier 1"); break;
        case D3D12_CROSS_NODE_SHARING_TIER_2:               wcscpy_s(sharing, L"Yes - Tier 2"); break;
        case D3D12_CROSS_NODE_SHARING_TIER_3:               wcscpy_s(sharing, L"Yes - Tier 3"); break;
        default:                                            wcscpy_s(sharing, c_szYes); break;
        }

        const WCHAR* sharedResTier = nullptr;
        switch (d3d12opts4.SharedResourceCompatibilityTier)
        {
        case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_0: sharedResTier = L"Yes - Tier 0"; break;
        case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_1: sharedResTier = L"Yes - Tier 1"; break;
        case D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_2: sharedResTier = L"Yes - Tier 2"; break;
        default: sharedResTier = c_szYes; break;
        }

        if (!pPrintInfo)
        {
            LVLINE(L"Cross-node Sharing", sharing);
            LVYESNO(L"Cross-adapter Row-Major Texture", d3d12opts.CrossAdapterRowMajorTextureSupported);

            LVLINE(L"Shared Resource Tier", sharedResTier);
            LVYESNO(L"Atomic Shader Instructions", d3d12xnode.AtomicShaderInstructions);
        }
        else
        {
            PRINTLINE(L"Cross-node Sharing", sharing);
            PRINTYESNO(L"Cross-adapter Row-Major Texture", d3d12opts.CrossAdapterRowMajorTextureSupported);

            PRINTLINE(L"Shared Resource Tier", sharedResTier);
            PRINTYESNO(L"Atomic Shader Instructions", d3d12xnode.AtomicShaderInstructions);
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    void D3D10_FillTree(HTREEITEM hTree, ID3D10Device* pDevice, D3D_DRIVER_TYPE devType)
    {
        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, L"Direct3D 10.0", TRUE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, L"Features", FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10(devType));

        TVAddNodeEx(hTreeD3D, L"Shader sample (any filter)", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

        TVAddNodeEx(hTreeD3D, L"Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

        TVAddNodeEx(hTreeD3D, L"Render Target", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_RENDER_TARGET, 0);

        TVAddNodeEx(hTreeD3D, L"Blendable Render Target", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_BLENDABLE, 0);

        // MSAA
        FillMSAASampleTable(pDevice, g_sampCount10, FALSE);

        TVAddNodeEx(hTreeD3D, L"2x MSAA", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

        TVAddNodeEx(hTreeD3D, L"4x MSAA", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

        TVAddNodeEx(hTreeD3D, L"8x MSAA", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

        TVAddNode(hTreeD3D, L"Other MSAA", FALSE, IDI_CAPS, D3D10InfoMSAA, (LPARAM)pDevice, (LPARAM)g_sampCount10);

        TVAddNodeEx(hTreeD3D, L"MSAA Load", FALSE, IDI_CAPS, D3D10Info,
            (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_LOAD, 0);
    }

    void D3D10_FillTree1(HTREEITEM hTree, ID3D10Device1* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D10_FEATURE_LEVEL1 fl = pDevice->GetFeatureLevel();

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, L"Direct3D 10.1", TRUE,
            IDI_CAPS, D3D10Info1, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel, (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));

        if ((g_DXGIFactory1 != nullptr && fl != D3D10_FEATURE_LEVEL_9_1)
            || (g_DXGIFactory1 == nullptr && fl != D3D10_FEATURE_LEVEL_10_0))
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, L"Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D10_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D10_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                // Fall thru

            case D3D10_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, L"D3D10_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                // Fall thru

            case D3D10_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, L"D3D10_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                // Fall thru

            case D3D10_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D10_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D10_1(devType));
                }
                break;
            }
        }

        // Only display for 10.1 devices. 10 and 11 devices are handled in their "native" node
        // Nothing optional about 10level9 feature levels
        if (fl == D3D10_FEATURE_LEVEL_10_1)
        {
            TVAddNodeEx(hTreeD3D, L"Shader sample (any filter)", FALSE, IDI_CAPS, D3D10Info1,
                (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

            TVAddNodeEx(hTreeD3D, L"Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D10Info1,
                (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

            TVAddNodeEx(hTreeD3D, L"Render Target", FALSE, IDI_CAPS, D3D10Info1,
                (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_RENDER_TARGET, 0);
        }

        // MSAA (for all but 10 devices, which are handled in their "native" node)
        if (fl != D3D10_FEATURE_LEVEL_10_0)
        {
            FillMSAASampleTable(pDevice, g_sampCount10_1, (fl != D3D10_FEATURE_LEVEL_10_1));

            if (fl == D3D10_FEATURE_LEVEL_10_1)
            {
                TVAddNodeEx(hTreeD3D, L"2x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

                TVAddNodeEx(hTreeD3D, L"4x MSAA (most required)", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

                TVAddNodeEx(hTreeD3D, L"8x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

                TVAddNode(hTreeD3D, L"Other MSAA", FALSE, IDI_CAPS, D3D10InfoMSAA, (LPARAM)pDevice, (LPARAM)g_sampCount10_1);
            }
            else // 10level9
            {
                TVAddNodeEx(hTreeD3D, L"2x MSAA", FALSE, IDI_CAPS, D3D10Info1,
                    (LPARAM)pDevice, (LPARAM)D3D10_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

                if (fl >= D3D10_FEATURE_LEVEL_9_3)
                {
                    TVAddNodeEx(hTreeD3D, L"4x MSAA", FALSE, IDI_CAPS, D3D10Info1,
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
            LVAddColumn(g_hwndLV, 0, L"Name", c_DefNameLength);
            LVAddColumn(g_hwndLV, 1, L"Texture2D", 15);
            LVAddColumn(g_hwndLV, 2, L"Input", 15);
            LVAddColumn(g_hwndLV, 3, L"Output", 15);
            LVAddColumn(g_hwndLV, 4, L"Encoder", 15);
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
                WCHAR buff[1024];

                swprintf_s(buff, L"Texture2D: %-3s   Input: %-3s   Output: %-3s   Encoder: %-3s",
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

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, L"Direct3D 11.0", TRUE,
            IDI_CAPS, D3D11Info, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, L"Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11(devType));
                }
                break;
            }
        }

        if (fl >= D3D_FEATURE_LEVEL_11_0)
        {
            TVAddNodeEx(hTreeD3D, L"Shader sample (any filter)", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

            TVAddNodeEx(hTreeD3D, L"Shader gather4", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_GATHER, 0);

            TVAddNodeEx(hTreeD3D, L"Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

            TVAddNodeEx(hTreeD3D, L"Render Target", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_RENDER_TARGET, 0);

            // MSAA (MSAA data for 10level9 is shown under the 10.1 node)
            FillMSAASampleTable(pDevice, g_sampCount11);

            TVAddNodeEx(hTreeD3D, L"2x MSAA", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

            TVAddNodeEx(hTreeD3D, L"4x MSAA (all required)", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

            TVAddNodeEx(hTreeD3D, L"8x MSAA (most required)", FALSE, IDI_CAPS, D3D11Info,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

            TVAddNodeEx(hTreeD3D, L"Other MSAA", FALSE, IDI_CAPS, D3D11InfoMSAA,
                (LPARAM)pDevice, (LPARAM)g_sampCount11, 0);
        }
    }

    void D3D11_FillTree1(HTREEITEM hTree, ID3D11Device1* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
        if (fl > D3D_FEATURE_LEVEL_11_1)
            fl = D3D_FEATURE_LEVEL_11_1;

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, L"Direct3D 11.1", TRUE,
            IDI_CAPS, D3D11Info1, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, L"Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_11_1:
                if (flMask & FLMASK_11_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_1(devType));
                }
                break;
            }
        }

        if (fl >= D3D_FEATURE_LEVEL_10_0)
        {
            TVAddNodeEx(hTreeD3D, L"IA Vertex Buffer", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER, 0);

            TVAddNodeEx(hTreeD3D, L"Shader sample (any filter)", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_SAMPLE, 0);

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                TVAddNodeEx(hTreeD3D, L"Shader gather4", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_SHADER_GATHER, 0);
            }

            TVAddNodeEx(hTreeD3D, L"Mipmap Auto-Generation", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MIP_AUTOGEN, 0);

            TVAddNodeEx(hTreeD3D, L"Render Target", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_RENDER_TARGET, 0);

            TVAddNodeEx(hTreeD3D, L"Blendable Render Target", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_BLENDABLE, 0);

            if (fl < D3D_FEATURE_LEVEL_11_1)
            {
                // This is required for 11.1, but is optional for 10.x and 11.0
                TVAddNodeEx(hTreeD3D, L"OM Logic Ops", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_OUTPUT_MERGER_LOGIC_OP);
            }

            if (fl >= D3D_FEATURE_LEVEL_11_0)
            {
                TVAddNodeEx(hTreeD3D, L"Typed UAV (most required)", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW, 0);

                TVAddNodeEx(hTreeD3D, L"UAV Typed Store (most required)", FALSE, IDI_CAPS, D3D11Info1,
                    (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_UAV_TYPED_STORE);
            }

            // MSAA (MSAA data for 10level9 is shown under the 10.1 node)
            FillMSAASampleTable(pDevice, g_sampCount11_1);

            TVAddNodeEx(hTreeD3D, L"2x MSAA", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 2);

            TVAddNodeEx(hTreeD3D, L"4x MSAA (all required)", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 4);

            TVAddNodeEx(hTreeD3D, L"8x MSAA (most required)", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET, 8);

            TVAddNodeEx(hTreeD3D, L"Other MSAA", FALSE, IDI_CAPS, D3D11InfoMSAA,
                (LPARAM)pDevice, (LPARAM)g_sampCount11_1, 1);

            TVAddNodeEx(hTreeD3D, L"MSAA Load", FALSE, IDI_CAPS, D3D11Info1,
                (LPARAM)pDevice, (LPARAM)D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD, 0);
        }

        if (devType != D3D_DRIVER_TYPE_REFERENCE)
        {
            TVAddNodeEx(hTreeD3D, L"Video", FALSE, IDI_CAPS, D3D11InfoVideo, (LPARAM)pDevice, 0, 0);
        }
    }

    void D3D11_FillTree2(HTREEITEM hTree, ID3D11Device2* pDevice, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();
        if (fl > D3D_FEATURE_LEVEL_11_1)
            fl = D3D_FEATURE_LEVEL_11_1;

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, L"Direct3D 11.2", TRUE,
            IDI_CAPS, D3D11Info2, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, L"Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_11_1:
                if (flMask & FLMASK_11_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_2(devType));
                }
                break;
            }
        }

        // The majority of this data is already shown under the DirectX 11.1 node, so we only show the 'new' info

        TVAddNodeEx(hTreeD3D, L"Shareable", FALSE, IDI_CAPS, D3D11Info2,
            (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_SHAREABLE);
    }

    void D3D11_FillTree3(HTREEITEM hTree, ID3D11Device3* pDevice, ID3D11Device4* pDevice4, DWORD flMask, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, (pDevice4) ? L"Direct3D 11.3/11.4" : L"Direct3D 11.3", TRUE,
            IDI_CAPS, D3D11Info3, (LPARAM)pDevice, 0, (LPARAM)pDevice4);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel,
            (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));

        if (fl != D3D_FEATURE_LEVEL_9_1)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, L"Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_12_2:
                if (flMask & FLMASK_12_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_12_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_12_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_12_1:
                if (flMask & FLMASK_12_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_12_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_12_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_12_0:
                if (flMask & FLMASK_11_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_11_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_1:
                if (flMask & FLMASK_11_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_11_0:
                if (flMask & FLMASK_10_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_1:
                if (flMask & FLMASK_10_0)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_10_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_10_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_10_0:
                if (flMask & FLMASK_9_3)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_3", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_3, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_3:
                if (flMask & FLMASK_9_2)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_2", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_2, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                // Fall thru

            case D3D_FEATURE_LEVEL_9_2:
                if (flMask & FLMASK_9_1)
                {
                    TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_9_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                        (LPARAM)D3D_FEATURE_LEVEL_9_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D11_3(devType));
                }
                break;
            }
        }

        // The majority of this data is already shown under the DirectX 11.1 or 11.2 node, so we only show the 'new' info

        TVAddNodeEx(hTreeD3D, L"UAV Typed Load", FALSE, IDI_CAPS, D3D11Info3,
            (LPARAM)pDevice, (LPARAM)-1, (LPARAM)D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD);
    }

    //-----------------------------------------------------------------------------
    void D3D12_FillTree(HTREEITEM hTree, ID3D12Device* pDevice, D3D_DRIVER_TYPE devType)
    {
        D3D_FEATURE_LEVEL fl = GetD3D12FeatureLevel(pDevice);

        HTREEITEM hTreeD3D = TVAddNodeEx(hTree, L"Direct3D 12", TRUE, IDI_CAPS, D3D12Info, (LPARAM)pDevice, (LPARAM)fl, 0);

        TVAddNodeEx(hTreeD3D, FLName(fl), FALSE, IDI_CAPS, D3D_FeatureLevel, (LPARAM)fl, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));

        if (fl != D3D_FEATURE_LEVEL_11_0)
        {
            HTREEITEM hTreeF = TVAddNode(hTreeD3D, L"Additional Feature Levels", TRUE, IDI_CAPS, nullptr, 0, 0);

            switch (fl)
            {
            case D3D_FEATURE_LEVEL_12_2:
                TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_12_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_12_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                // Fall thru

            case D3D_FEATURE_LEVEL_12_1:
                TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_12_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_12_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                // Fall thru

            case D3D_FEATURE_LEVEL_12_0:
                TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_11_1", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_11_1, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                // Fall thru

            case D3D_FEATURE_LEVEL_11_1:
                TVAddNodeEx(hTreeF, L"D3D_FEATURE_LEVEL_11_0", FALSE, IDI_CAPS, D3D_FeatureLevel,
                    (LPARAM)D3D_FEATURE_LEVEL_11_0, (LPARAM)pDevice, D3D_FL_LPARAM3_D3D12(devType));
                break;
            }
        }

        TVAddNodeEx(hTreeD3D, L"Architecture", FALSE, IDI_CAPS, D3D12Architecture, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, L"Extended Shader Features", FALSE, IDI_CAPS, D3D12ExShaderInfo, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, L"Multi-GPU", FALSE, IDI_CAPS, D3D12MultiGPU, (LPARAM)pDevice, 0, 0);

        TVAddNodeEx(hTreeD3D, L"Video", FALSE, IDI_CAPS, D3D12InfoVideo, (LPARAM)pDevice, 0, 1);
    }
}

//-----------------------------------------------------------------------------
// Name: DXGI_Init()
//-----------------------------------------------------------------------------
VOID DXGI_Init()
{
    // DXGI
    g_dxgi = LoadLibraryEx(L"dxgi.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
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
    g_d3d10_1 = LoadLibraryEx(L"d3d10_1.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_d3d10_1)
    {
        g_D3D10CreateDevice1 = reinterpret_cast<PFN_D3D10_CREATE_DEVICE1>(GetProcAddress(g_d3d10_1, "D3D10CreateDevice1"));
    }
    else
    {
        g_d3d10 = LoadLibraryEx(L"d3d10.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (g_d3d10)
        {
            g_D3D10CreateDevice = reinterpret_cast<LPD3D10CREATEDEVICE>(GetProcAddress(g_d3d10, "D3D10CreateDevice"));
        }
    }

    // Direct3D 11
    g_d3d11 = LoadLibraryEx(L"d3d11.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_d3d11)
    {
        g_D3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress(g_d3d11, "D3D11CreateDevice"));
    }

    // Direct3D 12
    g_d3d12 = LoadLibraryEx(L"d3d12.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
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

    HTREEITEM hTree = TVAddNode(TVI_ROOT, L"DXGI Devices", TRUE, IDI_DIRECTX, nullptr, 0, 0);

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

        HTREEITEM hTreeA;

        // No need for DXGIAdapterInfo3 as there's no extra desc information to display

        if (pAdapter2)
        {
            hTreeA = TVAddNode(hTree, aDesc.Description, TRUE, IDI_CAPS, DXGIAdapterInfo2, iAdapter, (LPARAM)(pAdapter2));
        }
        else if (pAdapter1)
        {
            hTreeA = TVAddNode(hTree, aDesc.Description, TRUE, IDI_CAPS, DXGIAdapterInfo1, iAdapter, (LPARAM)(pAdapter1));
        }
        else
        {
            hTreeA = TVAddNode(hTree, aDesc.Description, TRUE, IDI_CAPS, DXGIAdapterInfo, iAdapter, (LPARAM)(pAdapter));
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
                hTreeO = TVAddNode(hTreeA, L"Outputs", TRUE, IDI_CAPS, DXGIFeatures, 0, 0);
            }

            DXGI_OUTPUT_DESC oDesc;
            pOutput->GetDesc(&oDesc);

            HTREEITEM hTreeD = TVAddNode(hTreeO, oDesc.DeviceName, TRUE, IDI_CAPS, DXGIOutputInfo, iOutput, (LPARAM)pOutput);

            TVAddNode(hTreeD, L"Display Modes", FALSE, IDI_CAPS, DXGIOutputModes, iOutput, (LPARAM)pOutput);
        }

        // Direct3D 12
#ifdef EXTRA_DEBUG
        OutputDebugStringW(L"Direct3D 12\n");
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
                WCHAR buff[64] = {};
                swprintf_s(buff, L": Failed (%08X)\n", hr);
                OutputDebugStringW(buff);
#endif
                pDevice12 = nullptr;
            }
        }

        // Direct3D 11.x
#ifdef EXTRA_DEBUG
        OutputDebugStringW(L"Direct3D 11.x\n");
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
                    OutputDebugString(L": Success\n");
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
                    WCHAR buff[64] = {};
                    swprintf_s(buff, L": Failed (%08X)\n", hr);
                    OutputDebugStringW(buff);
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
                ? TVAddNode(hTreeA, L"Direct3D 11", TRUE, IDI_CAPS, nullptr, 0, 0)
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
        OutputDebugStringW(L"Direct3D 10.x\n");
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
                    OutputDebugString(L": Success\n");
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
                    WCHAR buff[64] = {};
                    swprintf_s(buff, L": Failed (%08X)\n", hr);
                    OutputDebugStringW(buff);
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
                ? TVAddNode(hTreeA, L"Direct3D 10", TRUE, IDI_CAPS, nullptr, 0, 0)
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
        OutputDebugString(L"WARP10\n");
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
        OutputDebugString(L"WARP11\n");
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
        OutputDebugString(L"WARP12\n");
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
                WCHAR buff[64] = {};
                swprintf_s(buff, L": Failed (%08X)\n", hr);
                OutputDebugStringW(buff);
#endif
                pDeviceWARP12 = nullptr;
            }

            warpAdapter->Release();
        }
#ifdef EXTRA_DEBUG
        else
        {
            OutputDebugString(L"WARP12 adapter not found!\n");
        }
#endif
    }

    if (pDeviceWARP10 || pDeviceWARP11 || pDeviceWARP11_1 || pDeviceWARP11_2 || pDeviceWARP11_3 || pDeviceWARP11_4 || pDeviceWARP12)
    {
        HTREEITEM hTreeW = TVAddNode(hTree, L"Windows Advanced Rasterization Platform (WARP)", TRUE, IDI_CAPS, nullptr, 0, 0);

        // DirectX 12 (WARP)
        if (pDeviceWARP12)
            D3D12_FillTree(hTreeW, pDeviceWARP12, D3D_DRIVER_TYPE_WARP);

        // DirectX 11.x (WARP)
        if (pDeviceWARP11 || pDeviceWARP11_1 || pDeviceWARP11_2 || pDeviceWARP11_3)
        {
            HTREEITEM hTree11 = (pDeviceWARP11_1 || pDeviceWARP11_2 || pDeviceWARP11_3)
                ? TVAddNode(hTreeW, L"Direct3D 11", TRUE, IDI_CAPS, nullptr, 0, 0)
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
            HTREEITEM hTree10 = TVAddNode(hTreeW, L"Direct3D 10", TRUE, IDI_CAPS, nullptr, 0, 0);

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
        HTREEITEM hTreeR = TVAddNode(hTree, L"Reference", TRUE, IDI_CAPS, nullptr, 0, 0);

        // No REF for Direct3D 12

        // Direct3D 11.x (REF)
        if (pDeviceREF11 || pDeviceREF11_1 || pDeviceREF11_2 || pDeviceREF11_3)
        {
            HTREEITEM hTree11 = (pDeviceREF11_1 || pDeviceREF11_2 || pDeviceREF11_3)
                ? TVAddNode(hTreeR, L"Direct3D 11", TRUE, IDI_CAPS, nullptr, 0, 0)
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
                ? TVAddNode(hTreeR, L"Direct3D 10", TRUE, IDI_CAPS, nullptr, 0, 0)
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
