//-----------------------------------------------------------------------------
// Name: dxg.cpp
//
// Desc: DirectX Capabilities Viewer for Direct3D
//
// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?linkid=2136896
//-----------------------------------------------------------------------------
#include "dxview.h"
#include <d3d9.h>

// Some useful 9EX defines so we don't have to include the 9ex header
#define D3DFMT_D32_LOCKABLE 84
#define D3DFMT_S8_LOCKABLE	85
#define D3DFMT_A1			118

#define D3DCAPS2_CANSHARERESOURCE          0x80000000L

#define D3DPMISCCAPS_POSTBLENDSRGBCONVERT  0x00200000L /* Indicates device can perform conversion to sRGB after blending. */

#define D3DPBLENDCAPS_SRCCOLOR2            0x00004000L
#define D3DPBLENDCAPS_INVSRCCOLOR2         0x00008000L

#define D3DPTFILTERCAPS_CONVOLUTIONMONO    0x00040000L /* Min and Mag for the convolution mono filter */

namespace
{
    using LPDIRECT3D9CREATE9 = IDirect3D9 * (WINAPI*)(UINT SDKVersion);
    using LPDIRECT3D9CREATE9EX = HRESULT(WINAPI*)(UINT SDKVersion, IDirect3D9**);

    LPDIRECT3D9 g_pD3D = nullptr;
    LPDIRECT3D9CREATE9 g_direct3DCreate9 = nullptr;
    LPDIRECT3D9CREATE9EX g_direct3DCreate9Ex = nullptr;
    HMODULE g_hInstD3D = nullptr;

    BOOL g_is9Ex = FALSE;

    BOOL IsAdapterFmtAvailable(UINT iAdapter, D3DDEVTYPE devType, D3DFORMAT fmtAdapter, BOOL bWindowed);
    HRESULT DXGDisplayCaps(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pInfo);

#define CAPSVALDEFex(name,val)           {name, FIELD_OFFSET(D3DCAPS9,val), 0, DXV_9EXCAP}
#define CAPSVALDEF(name,val)           {name, FIELD_OFFSET(D3DCAPS9,val), 0}
#define CAPSFLAGDEFex(name,val,flag)     {name, FIELD_OFFSET(D3DCAPS9,val), flag, DXV_9EXCAP}
#define CAPSFLAGDEF(name,val,flag)     {name, FIELD_OFFSET(D3DCAPS9,val), flag}
#define CAPSMASK16DEF(name,val)        {name, FIELD_OFFSET(D3DCAPS9,val), 0x0FFFFFFF}
#define CAPSFLOATDEF(name,val)         {name, FIELD_OFFSET(D3DCAPS9,val), 0xBFFFFFFF}
#define CAPSSHADERDEF(name,val)        {name, FIELD_OFFSET(D3DCAPS9,val), 0xEFFFFFFF}

#define PRIMCAPSVALDEF(name,val)       {name, FIELD_OFFSET(D3DPRIMCAPS9,val), 0}
#define PRIMCAPSFLAGDEF(name,val,flag) {name, FIELD_OFFSET(D3DPRIMCAPS9,val), flag}

    // NOTE: Remember to update FormatName() when you update this list!!
    D3DFORMAT AllFormatArray[] =
    {
        D3DFMT_R8G8B8,
        D3DFMT_A8R8G8B8,
        D3DFMT_X8R8G8B8,
        D3DFMT_R5G6B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A1R5G5B5,
        D3DFMT_A4R4G4B4,
        D3DFMT_R3G3B2,
        D3DFMT_A8,
        D3DFMT_A8R3G3B2,
        D3DFMT_X4R4G4B4,
        D3DFMT_A2B10G10R10,
        D3DFMT_A8B8G8R8,
        D3DFMT_X8B8G8R8,
        D3DFMT_G16R16,
        D3DFMT_A2R10G10B10,
        D3DFMT_A16B16G16R16,

        D3DFMT_A8P8,
        D3DFMT_P8,

        D3DFMT_L8,
        D3DFMT_A8L8,
        D3DFMT_A4L4,

        D3DFMT_V8U8,
        D3DFMT_L6V5U5,
        D3DFMT_X8L8V8U8,
        D3DFMT_Q8W8V8U8,
        D3DFMT_V16U16,
        D3DFMT_A2W10V10U10,

        D3DFMT_UYVY,
        D3DFMT_R8G8_B8G8,
        D3DFMT_YUY2,
        D3DFMT_G8R8_G8B8,
        D3DFMT_DXT1,
        D3DFMT_DXT2,
        D3DFMT_DXT3,
        D3DFMT_DXT4,
        D3DFMT_DXT5,

        D3DFMT_D16_LOCKABLE,
        D3DFMT_D32,
        D3DFMT_D15S1,
        D3DFMT_D24S8,
        D3DFMT_D24X8,
        D3DFMT_D24X4S4,
        D3DFMT_D16,

        D3DFMT_D32F_LOCKABLE,
        D3DFMT_D24FS8,

        D3DFMT_L16,

        D3DFMT_VERTEXDATA,
        D3DFMT_INDEX16,
        D3DFMT_INDEX32,

        D3DFMT_Q16W16V16U16,

        D3DFMT_MULTI2_ARGB8,

        D3DFMT_R16F,
        D3DFMT_G16R16F,
        D3DFMT_A16B16G16R16F,

        D3DFMT_R32F,
        D3DFMT_G32R32F,
        D3DFMT_A32B32G32R32F,

        D3DFMT_CxV8U8,

        (D3DFORMAT)D3DFMT_D32_LOCKABLE,
        (D3DFORMAT)D3DFMT_S8_LOCKABLE,
        (D3DFORMAT)D3DFMT_A1,
    };
    const int NumFormats = sizeof(AllFormatArray) / sizeof(AllFormatArray[0]);

    // A subset of AllFormatArray...it's those D3DFMTs that could possibly be
    // adapter (display) formats.
    D3DFORMAT AdapterFormatArray[] =
    {
        D3DFMT_A2R10G10B10,
        D3DFMT_X8R8G8B8,
        D3DFMT_R8G8B8,
        D3DFMT_X1R5G5B5,
        D3DFMT_R5G6B5,
    };
    const int NumAdapterFormats = sizeof(AdapterFormatArray) / sizeof(AdapterFormatArray[0]);

    // A subset of AllFormatArray...it's those D3DFMTs that could possibly be
    // back buffer formats.
    D3DFORMAT BBFormatArray[] =
    {
        D3DFMT_A2R10G10B10,
        D3DFMT_A8R8G8B8,
        D3DFMT_X8R8G8B8,
        D3DFMT_A1R5G5B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_R5G6B5,
    };
    const int NumBBFormats = sizeof(BBFormatArray) / sizeof(BBFormatArray[0]);

    // A subset of AllFormatArray...it's those D3DFMTs that could possibly be
    // depth/stencil formats.
    D3DFORMAT DSFormatArray[] =
    {
        D3DFMT_D16_LOCKABLE,
        D3DFMT_D32,
        D3DFMT_D15S1,
        D3DFMT_D24S8,
        D3DFMT_D24X8,
        D3DFMT_D24X4S4,
        D3DFMT_D16,
        D3DFMT_D32F_LOCKABLE,
        D3DFMT_D24FS8,
    };
    const int NumDSFormats = sizeof(DSFormatArray) / sizeof(DSFormatArray[0]);

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF DXGGenCaps[] =
    {
        CAPSVALDEF(L"DeviceType",                   DeviceType),
        CAPSVALDEF(L"AdapterOrdinal",               AdapterOrdinal),

        CAPSVALDEF(L"MaxTextureWidth",              MaxTextureWidth),
        CAPSVALDEF(L"MaxTextureHeight",             MaxTextureHeight),
        CAPSVALDEF(L"MaxVolumeExtent",              MaxVolumeExtent),

        CAPSVALDEF(L"MaxTextureRepeat",             MaxTextureRepeat),
        CAPSVALDEF(L"MaxTextureAspectRatio",        MaxTextureAspectRatio),
        CAPSVALDEF(L"MaxAnisotropy",                MaxAnisotropy),
        CAPSFLOATDEF(L"MaxVertexW",                 MaxVertexW),

        CAPSFLOATDEF(L"GuardBandLeft",              GuardBandLeft),
        CAPSFLOATDEF(L"GuardBandTop",               GuardBandTop),
        CAPSFLOATDEF(L"GuardBandRight",             GuardBandRight),
        CAPSFLOATDEF(L"GuardBandBottom",            GuardBandBottom),

        CAPSFLOATDEF(L"ExtentsAdjust",              ExtentsAdjust),

        CAPSVALDEF(L"MaxTextureBlendStages",        MaxTextureBlendStages),
        CAPSVALDEF(L"MaxSimultaneousTextures",      MaxSimultaneousTextures),

        CAPSVALDEF(L"MaxActiveLights",              MaxActiveLights),
        CAPSVALDEF(L"MaxUserClipPlanes",            MaxUserClipPlanes),
        CAPSVALDEF(L"MaxVertexBlendMatrices",       MaxVertexBlendMatrices),
        CAPSVALDEF(L"MaxVertexBlendMatrixIndex",    MaxVertexBlendMatrixIndex),

        CAPSFLOATDEF(L"MaxPointSize",               MaxPointSize),

        CAPSVALDEF(L"MaxPrimitiveCount",            MaxPrimitiveCount),
        CAPSVALDEF(L"MaxVertexIndex",               MaxVertexIndex),
        CAPSVALDEF(L"MaxStreams",                   MaxStreams),
        CAPSVALDEF(L"MaxStreamStride",              MaxStreamStride),

        CAPSSHADERDEF(L"VertexShaderVersion",       VertexShaderVersion),
        CAPSVALDEF(L"MaxVertexShaderConst",         MaxVertexShaderConst),

        CAPSSHADERDEF(L"PixelShaderVersion",        PixelShaderVersion),
        CAPSFLOATDEF(L"PixelShader1xMaxValue",      PixelShader1xMaxValue),

        CAPSFLOATDEF(L"MaxNpatchTessellationLevel", MaxNpatchTessellationLevel),

        CAPSVALDEF(L"MasterAdapterOrdinal",         MasterAdapterOrdinal),
        CAPSVALDEF(L"AdapterOrdinalInGroup",        AdapterOrdinalInGroup),
        CAPSVALDEF(L"NumberOfAdaptersInGroup",      NumberOfAdaptersInGroup),
        CAPSVALDEF(L"NumSimultaneousRTs",           NumSimultaneousRTs),
        CAPSVALDEF(L"MaxVShaderInstructionsExecuted", MaxVShaderInstructionsExecuted),
        CAPSVALDEF(L"MaxPShaderInstructionsExecuted", MaxPShaderInstructionsExecuted),
        CAPSVALDEF(L"MaxVertexShader30InstructionSlots", MaxVertexShader30InstructionSlots),
        CAPSVALDEF(L"MaxPixelShader30InstructionSlots", MaxPixelShader30InstructionSlots),

        { nullptr, 0, 0 }
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsCaps[] =
    {
        CAPSFLAGDEF(L"D3DCAPS_READ_SCANLINE",        Caps,   D3DCAPS_READ_SCANLINE),
        CAPSFLAGDEFex(L"D3DCAPS_OVERLAY",            Caps,   D3DCAPS_OVERLAY),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsCaps2[] =
    {
        CAPSFLAGDEF(L"D3DCAPS2_CANCALIBRATEGAMMA",   Caps2,  D3DCAPS2_CANCALIBRATEGAMMA),
        CAPSFLAGDEF(L"D3DCAPS2_FULLSCREENGAMMA",     Caps2,  D3DCAPS2_FULLSCREENGAMMA),
        CAPSFLAGDEF(L"D3DCAPS2_CANMANAGERESOURCE",   Caps2,  D3DCAPS2_CANMANAGERESOURCE),
        CAPSFLAGDEF(L"D3DCAPS2_DYNAMICTEXTURES",     Caps2,  D3DCAPS2_DYNAMICTEXTURES),
        CAPSFLAGDEF(L"D3DCAPS2_CANAUTOGENMIPMAP",    Caps2,  D3DCAPS2_CANAUTOGENMIPMAP),
        CAPSFLAGDEFex(L"D3DCAPS2_CANSHARERESOURCE",  Caps2,  D3DCAPS2_CANSHARERESOURCE),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsCaps3[] =
    {
        CAPSFLAGDEF(L"D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD", Caps3,    D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD),
        CAPSFLAGDEF(L"D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION",      Caps3,    D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION),
        CAPSFLAGDEF(L"D3DCAPS3_COPY_TO_VIDMEM",                   Caps3,    D3DCAPS3_COPY_TO_VIDMEM),
        CAPSFLAGDEF(L"D3DCAPS3_COPY_TO_SYSTEMMEM",                Caps3,    D3DCAPS3_COPY_TO_SYSTEMMEM),
        CAPSFLAGDEFex(L"D3DCAPS3_DXVAHD",                         Caps3,    D3DCAPS3_DXVAHD),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsPresentationIntervals[] =
    {
        CAPSFLAGDEF(L"D3DPRESENT_INTERVAL_ONE",   PresentationIntervals, D3DPRESENT_INTERVAL_ONE),
        CAPSFLAGDEF(L"D3DPRESENT_INTERVAL_TWO",   PresentationIntervals, D3DPRESENT_INTERVAL_TWO),
        CAPSFLAGDEF(L"D3DPRESENT_INTERVAL_THREE",   PresentationIntervals, D3DPRESENT_INTERVAL_THREE),
        CAPSFLAGDEF(L"D3DPRESENT_INTERVAL_FOUR",   PresentationIntervals, D3DPRESENT_INTERVAL_FOUR),
        CAPSFLAGDEF(L"D3DPRESENT_INTERVAL_IMMEDIATE",   PresentationIntervals, D3DPRESENT_INTERVAL_IMMEDIATE),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsCursorCaps[] =
    {
        CAPSFLAGDEF(L"D3DCURSORCAPS_COLOR",   CursorCaps,    D3DCURSORCAPS_COLOR),
        CAPSFLAGDEF(L"D3DCURSORCAPS_LOWRES",  CursorCaps,    D3DCURSORCAPS_LOWRES),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsDevCaps[] =
    {
        CAPSFLAGDEF(L"D3DDEVCAPS_EXECUTESYSTEMMEMORY",     DevCaps,    D3DDEVCAPS_EXECUTESYSTEMMEMORY),
        CAPSFLAGDEF(L"D3DDEVCAPS_EXECUTEVIDEOMEMORY",      DevCaps,    D3DDEVCAPS_EXECUTEVIDEOMEMORY),
        CAPSFLAGDEF(L"D3DDEVCAPS_TLVERTEXSYSTEMMEMORY",    DevCaps,    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY),
        CAPSFLAGDEF(L"D3DDEVCAPS_TLVERTEXVIDEOMEMORY",     DevCaps,    D3DDEVCAPS_TLVERTEXVIDEOMEMORY),
        CAPSFLAGDEF(L"D3DDEVCAPS_TEXTURESYSTEMMEMORY",     DevCaps,    D3DDEVCAPS_TEXTURESYSTEMMEMORY),
        CAPSFLAGDEF(L"D3DDEVCAPS_TEXTUREVIDEOMEMORY",      DevCaps,    D3DDEVCAPS_TEXTUREVIDEOMEMORY),
        CAPSFLAGDEF(L"D3DDEVCAPS_DRAWPRIMTLVERTEX",        DevCaps,    D3DDEVCAPS_DRAWPRIMTLVERTEX),
        CAPSFLAGDEF(L"D3DDEVCAPS_CANRENDERAFTERFLIP",      DevCaps,    D3DDEVCAPS_CANRENDERAFTERFLIP),
        CAPSFLAGDEF(L"D3DDEVCAPS_TEXTURENONLOCALVIDMEM",   DevCaps,    D3DDEVCAPS_TEXTURENONLOCALVIDMEM),
        CAPSFLAGDEF(L"D3DDEVCAPS_DRAWPRIMITIVES2",         DevCaps,    D3DDEVCAPS_DRAWPRIMITIVES2),
        CAPSFLAGDEF(L"D3DDEVCAPS_SEPARATETEXTUREMEMORIES", DevCaps,    D3DDEVCAPS_SEPARATETEXTUREMEMORIES),
        CAPSFLAGDEF(L"D3DDEVCAPS_DRAWPRIMITIVES2EX",       DevCaps,    D3DDEVCAPS_DRAWPRIMITIVES2EX),
        CAPSFLAGDEF(L"D3DDEVCAPS_HWTRANSFORMANDLIGHT",     DevCaps,    D3DDEVCAPS_HWTRANSFORMANDLIGHT),
        CAPSFLAGDEF(L"D3DDEVCAPS_CANBLTSYSTONONLOCAL",     DevCaps,    D3DDEVCAPS_CANBLTSYSTONONLOCAL),
        CAPSFLAGDEF(L"D3DDEVCAPS_HWRASTERIZATION",         DevCaps,    D3DDEVCAPS_HWRASTERIZATION),
        CAPSFLAGDEF(L"D3DDEVCAPS_PUREDEVICE",              DevCaps,    D3DDEVCAPS_PUREDEVICE),
        CAPSFLAGDEF(L"D3DDEVCAPS_QUINTICRTPATCHES",        DevCaps,    D3DDEVCAPS_QUINTICRTPATCHES),
        CAPSFLAGDEF(L"D3DDEVCAPS_RTPATCHES",               DevCaps,    D3DDEVCAPS_RTPATCHES),
        CAPSFLAGDEF(L"D3DDEVCAPS_RTPATCHHANDLEZERO",       DevCaps,    D3DDEVCAPS_RTPATCHHANDLEZERO),
        CAPSFLAGDEF(L"D3DDEVCAPS_NPATCHES",                DevCaps,    D3DDEVCAPS_NPATCHES),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsLineCaps[] =
    {
        CAPSFLAGDEF(L"D3DLINECAPS_TEXTURE",           LineCaps,    D3DLINECAPS_TEXTURE),
        CAPSFLAGDEF(L"D3DLINECAPS_ZTEST",             LineCaps,    D3DLINECAPS_ZTEST),
        CAPSFLAGDEF(L"D3DLINECAPS_BLEND",             LineCaps,    D3DLINECAPS_BLEND),
        CAPSFLAGDEF(L"D3DLINECAPS_ALPHACMP",          LineCaps,    D3DLINECAPS_ALPHACMP),
        CAPSFLAGDEF(L"D3DLINECAPS_FOG",               LineCaps,    D3DLINECAPS_FOG),
        CAPSFLAGDEF(L"D3DLINECAPS_ANTIALIAS",         LineCaps,    D3DLINECAPS_ANTIALIAS),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsStencilCaps[] =
    {
        CAPSFLAGDEF(L"D3DSTENCILCAPS_KEEP",           StencilCaps,    D3DSTENCILCAPS_KEEP),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_ZERO",           StencilCaps,    D3DSTENCILCAPS_ZERO),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_REPLACE",        StencilCaps,    D3DSTENCILCAPS_REPLACE),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_INCRSAT",        StencilCaps,    D3DSTENCILCAPS_INCRSAT),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_DECRSAT",        StencilCaps,    D3DSTENCILCAPS_DECRSAT),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_INVERT",         StencilCaps,    D3DSTENCILCAPS_INVERT),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_INCR",           StencilCaps,    D3DSTENCILCAPS_INCR),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_DECR",           StencilCaps,    D3DSTENCILCAPS_DECR),
        CAPSFLAGDEF(L"D3DSTENCILCAPS_TWOSIDED",       StencilCaps,    D3DSTENCILCAPS_TWOSIDED),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsFVFCaps[] =
    {
        CAPSFLAGDEF(L"D3DFVFCAPS_DONOTSTRIPELEMENTS", FVFCaps,    D3DFVFCAPS_DONOTSTRIPELEMENTS),
        CAPSMASK16DEF(L"D3DFVFCAPS_TEXCOORDCOUNTMASK", FVFCaps),
        CAPSFLAGDEF(L"D3DFVFCAPS_PSIZE",              FVFCaps,    D3DFVFCAPS_PSIZE),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsTextureOpCaps[] =
    {
        CAPSFLAGDEF(L"D3DTEXOPCAPS_DISABLE",                     TextureOpCaps, D3DTEXOPCAPS_DISABLE),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_SELECTARG1",                  TextureOpCaps, D3DTEXOPCAPS_SELECTARG1),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_SELECTARG2",                  TextureOpCaps, D3DTEXOPCAPS_SELECTARG2),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATE",                    TextureOpCaps, D3DTEXOPCAPS_MODULATE),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATE2X",                  TextureOpCaps, D3DTEXOPCAPS_MODULATE2X),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATE4X",                  TextureOpCaps, D3DTEXOPCAPS_MODULATE4X),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_ADD",                         TextureOpCaps, D3DTEXOPCAPS_ADD),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_ADDSIGNED",                   TextureOpCaps, D3DTEXOPCAPS_ADDSIGNED),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_ADDSIGNED2X",                 TextureOpCaps, D3DTEXOPCAPS_ADDSIGNED2X),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_SUBTRACT",                    TextureOpCaps, D3DTEXOPCAPS_SUBTRACT),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_ADDSMOOTH",                   TextureOpCaps, D3DTEXOPCAPS_ADDSMOOTH),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BLENDDIFFUSEALPHA",           TextureOpCaps, D3DTEXOPCAPS_BLENDDIFFUSEALPHA),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BLENDTEXTUREALPHA",           TextureOpCaps, D3DTEXOPCAPS_BLENDTEXTUREALPHA),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BLENDFACTORALPHA",            TextureOpCaps, D3DTEXOPCAPS_BLENDFACTORALPHA),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BLENDTEXTUREALPHAPM",         TextureOpCaps, D3DTEXOPCAPS_BLENDTEXTUREALPHAPM),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BLENDCURRENTALPHA",           TextureOpCaps, D3DTEXOPCAPS_BLENDCURRENTALPHA),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_PREMODULATE",                 TextureOpCaps, D3DTEXOPCAPS_PREMODULATE),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR",      TextureOpCaps, D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA",      TextureOpCaps, D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR",   TextureOpCaps, D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA",   TextureOpCaps, D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BUMPENVMAP",                  TextureOpCaps, D3DTEXOPCAPS_BUMPENVMAP),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_BUMPENVMAPLUMINANCE",         TextureOpCaps, D3DTEXOPCAPS_BUMPENVMAPLUMINANCE),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_DOTPRODUCT3",                 TextureOpCaps, D3DTEXOPCAPS_DOTPRODUCT3),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_MULTIPLYADD",                 TextureOpCaps, D3DTEXOPCAPS_MULTIPLYADD),
        CAPSFLAGDEF(L"D3DTEXOPCAPS_LERP",                        TextureOpCaps, D3DTEXOPCAPS_LERP),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsVertexProcessingCaps[] =
    {
        CAPSFLAGDEF(L"D3DVTXPCAPS_DIRECTIONALLIGHTS", VertexProcessingCaps, D3DVTXPCAPS_DIRECTIONALLIGHTS),
        CAPSFLAGDEF(L"D3DVTXPCAPS_LOCALVIEWER",       VertexProcessingCaps, D3DVTXPCAPS_LOCALVIEWER),
        CAPSFLAGDEF(L"D3DVTXPCAPS_MATERIALSOURCE7",   VertexProcessingCaps, D3DVTXPCAPS_MATERIALSOURCE7),
        CAPSFLAGDEF(L"D3DVTXPCAPS_POSITIONALLIGHTS",  VertexProcessingCaps, D3DVTXPCAPS_POSITIONALLIGHTS),
        CAPSFLAGDEF(L"D3DVTXPCAPS_TEXGEN",            VertexProcessingCaps, D3DVTXPCAPS_TEXGEN),
        CAPSFLAGDEF(L"D3DVTXPCAPS_TWEENING",          VertexProcessingCaps, D3DVTXPCAPS_TWEENING),
        CAPSFLAGDEF(L"D3DVTXPCAPS_TEXGEN_SPHEREMAP",  VertexProcessingCaps, D3DVTXPCAPS_TEXGEN_SPHEREMAP),
        CAPSFLAGDEF(L"D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER",  VertexProcessingCaps, D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsPrimMiscCaps[] =
    {
        CAPSFLAGDEF(L"D3DPMISCCAPS_MASKZ",                      PrimitiveMiscCaps,  D3DPMISCCAPS_MASKZ),
        CAPSFLAGDEF(L"D3DPMISCCAPS_CULLNONE",                   PrimitiveMiscCaps,  D3DPMISCCAPS_CULLNONE),
        CAPSFLAGDEF(L"D3DPMISCCAPS_CULLCW",                     PrimitiveMiscCaps,  D3DPMISCCAPS_CULLCW),
        CAPSFLAGDEF(L"D3DPMISCCAPS_CULLCCW",                    PrimitiveMiscCaps,  D3DPMISCCAPS_CULLCCW),
        CAPSFLAGDEF(L"D3DPMISCCAPS_COLORWRITEENABLE",           PrimitiveMiscCaps,  D3DPMISCCAPS_COLORWRITEENABLE),
        CAPSFLAGDEF(L"D3DPMISCCAPS_CLIPPLANESCALEDPOINTS",      PrimitiveMiscCaps,  D3DPMISCCAPS_CLIPPLANESCALEDPOINTS),
        CAPSFLAGDEF(L"D3DPMISCCAPS_CLIPTLVERTS",                PrimitiveMiscCaps,  D3DPMISCCAPS_CLIPTLVERTS),
        CAPSFLAGDEF(L"D3DPMISCCAPS_TSSARGTEMP",                 PrimitiveMiscCaps,  D3DPMISCCAPS_TSSARGTEMP),
        CAPSFLAGDEF(L"D3DPMISCCAPS_BLENDOP",                    PrimitiveMiscCaps,  D3DPMISCCAPS_BLENDOP),
        CAPSFLAGDEF(L"D3DPMISCCAPS_NULLREFERENCE",              PrimitiveMiscCaps,  D3DPMISCCAPS_NULLREFERENCE),
        CAPSFLAGDEF(L"D3DPMISCCAPS_INDEPENDENTWRITEMASKS",      PrimitiveMiscCaps,  D3DPMISCCAPS_INDEPENDENTWRITEMASKS),
        CAPSFLAGDEF(L"D3DPMISCCAPS_PERSTAGECONSTANT",           PrimitiveMiscCaps,  D3DPMISCCAPS_PERSTAGECONSTANT),
        CAPSFLAGDEF(L"D3DPMISCCAPS_FOGANDSPECULARALPHA",        PrimitiveMiscCaps,  D3DPMISCCAPS_FOGANDSPECULARALPHA),
        CAPSFLAGDEF(L"D3DPMISCCAPS_SEPARATEALPHABLEND",         PrimitiveMiscCaps,  D3DPMISCCAPS_SEPARATEALPHABLEND),
        CAPSFLAGDEF(L"D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS",    PrimitiveMiscCaps,  D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS),
        CAPSFLAGDEF(L"D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING", PrimitiveMiscCaps,  D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING),
        CAPSFLAGDEF(L"D3DPMISCCAPS_FOGVERTEXCLAMPED",           PrimitiveMiscCaps,  D3DPMISCCAPS_FOGVERTEXCLAMPED),
        CAPSFLAGDEFex(L"D3DPMISCCAPS_POSTBLENDSRGBCONVERT",     PrimitiveMiscCaps,  D3DPMISCCAPS_POSTBLENDSRGBCONVERT),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsRasterCaps[] =
    {
        CAPSFLAGDEF(L"D3DPRASTERCAPS_DITHER",            RasterCaps,   D3DPRASTERCAPS_DITHER),
        CAPSFLAGDEF(L"D3DPRASTERCAPS_ZTEST",             RasterCaps,   D3DPRASTERCAPS_ZTEST),
        CAPSFLAGDEF(L"D3DPRASTERCAPS_FOGVERTEX",         RasterCaps,   D3DPRASTERCAPS_FOGVERTEX),
        CAPSFLAGDEF(L"D3DPRASTERCAPS_FOGTABLE",          RasterCaps,   D3DPRASTERCAPS_FOGTABLE),
        //    CAPSFLAGDEF(L"D3DPRASTERCAPS_ANTIALIASEDGES",    RasterCaps,   D3DPRASTERCAPS_ANTIALIASEDGES),
            CAPSFLAGDEF(L"D3DPRASTERCAPS_MIPMAPLODBIAS",     RasterCaps,   D3DPRASTERCAPS_MIPMAPLODBIAS),
            //    CAPSFLAGDEF(L"D3DPRASTERCAPS_ZBIAS",             RasterCaps,   D3DPRASTERCAPS_ZBIAS),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_ZBUFFERLESSHSR",    RasterCaps,   D3DPRASTERCAPS_ZBUFFERLESSHSR),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_FOGRANGE",          RasterCaps,   D3DPRASTERCAPS_FOGRANGE),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_ANISOTROPY",        RasterCaps,   D3DPRASTERCAPS_ANISOTROPY),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_WBUFFER",           RasterCaps,   D3DPRASTERCAPS_WBUFFER),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_WFOG",              RasterCaps,   D3DPRASTERCAPS_WFOG),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_ZFOG",              RasterCaps,   D3DPRASTERCAPS_ZFOG),
                CAPSFLAGDEF(L"D3DPRASTERCAPS_COLORPERSPECTIVE",  RasterCaps,   D3DPRASTERCAPS_COLORPERSPECTIVE),
                //    CAPSFLAGDEF(L"D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE",   RasterCaps,   D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE),
                    CAPSFLAGDEF(L"D3DPRASTERCAPS_SCISSORTEST",       RasterCaps,   D3DPRASTERCAPS_SCISSORTEST),
                    CAPSFLAGDEF(L"D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS", RasterCaps,   D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS),
                    CAPSFLAGDEF(L"D3DPRASTERCAPS_DEPTHBIAS",         RasterCaps,   D3DPRASTERCAPS_DEPTHBIAS),
                    CAPSFLAGDEF(L"D3DPRASTERCAPS_MULTISAMPLE_TOGGLE",RasterCaps,   D3DPRASTERCAPS_MULTISAMPLE_TOGGLE),
                    {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsZCmpCaps[] =
    {
        CAPSFLAGDEF(L"D3DPCMPCAPS_NEVER",         ZCmpCaps,  D3DPCMPCAPS_NEVER),
        CAPSFLAGDEF(L"D3DPCMPCAPS_LESS",          ZCmpCaps,  D3DPCMPCAPS_LESS),
        CAPSFLAGDEF(L"D3DPCMPCAPS_EQUAL",         ZCmpCaps,  D3DPCMPCAPS_EQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_LESSEQUAL",     ZCmpCaps,  D3DPCMPCAPS_LESSEQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_GREATER",       ZCmpCaps,  D3DPCMPCAPS_GREATER),
        CAPSFLAGDEF(L"D3DPCMPCAPS_NOTEQUAL",      ZCmpCaps,  D3DPCMPCAPS_NOTEQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_GREATEREQUAL",  ZCmpCaps,  D3DPCMPCAPS_GREATEREQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_ALWAYS",        ZCmpCaps,  D3DPCMPCAPS_ALWAYS),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsSrcBlendCaps[] =
    {
        CAPSFLAGDEF(L"D3DPBLENDCAPS_ZERO",            SrcBlendCaps,  D3DPBLENDCAPS_ZERO),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_ONE",             SrcBlendCaps,  D3DPBLENDCAPS_ONE),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_SRCCOLOR",        SrcBlendCaps,  D3DPBLENDCAPS_SRCCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVSRCCOLOR",     SrcBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_SRCALPHA",        SrcBlendCaps,  D3DPBLENDCAPS_SRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVSRCALPHA",     SrcBlendCaps,  D3DPBLENDCAPS_INVSRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_DESTALPHA",       SrcBlendCaps,  D3DPBLENDCAPS_DESTALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVDESTALPHA",    SrcBlendCaps,  D3DPBLENDCAPS_INVDESTALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_DESTCOLOR",       SrcBlendCaps,  D3DPBLENDCAPS_DESTCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVDESTCOLOR",    SrcBlendCaps,  D3DPBLENDCAPS_INVDESTCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_SRCALPHASAT",     SrcBlendCaps,  D3DPBLENDCAPS_SRCALPHASAT),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_BOTHSRCALPHA",    SrcBlendCaps,  D3DPBLENDCAPS_BOTHSRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_BOTHINVSRCALPHA", SrcBlendCaps,  D3DPBLENDCAPS_BOTHINVSRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_BLENDFACTOR",     SrcBlendCaps,  D3DPBLENDCAPS_BLENDFACTOR),
        CAPSFLAGDEFex(L"D3DPBLENDCAPS_SRCCOLOR2",	 SrcBlendCaps,  D3DPBLENDCAPS_SRCCOLOR2),
        CAPSFLAGDEFex(L"D3DPBLENDCAPS_INVSRCCOLOR2",  SrcBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR2),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsDestBlendCaps[] =
    {
        CAPSFLAGDEF(L"D3DPBLENDCAPS_ZERO",            DestBlendCaps,  D3DPBLENDCAPS_ZERO),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_ONE",             DestBlendCaps,  D3DPBLENDCAPS_ONE),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_SRCCOLOR",        DestBlendCaps,  D3DPBLENDCAPS_SRCCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVSRCCOLOR",     DestBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_SRCALPHA",        DestBlendCaps,  D3DPBLENDCAPS_SRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVSRCALPHA",     DestBlendCaps,  D3DPBLENDCAPS_INVSRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_DESTALPHA",       DestBlendCaps,  D3DPBLENDCAPS_DESTALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVDESTALPHA",    DestBlendCaps,  D3DPBLENDCAPS_INVDESTALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_DESTCOLOR",       DestBlendCaps,  D3DPBLENDCAPS_DESTCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_INVDESTCOLOR",    DestBlendCaps,  D3DPBLENDCAPS_INVDESTCOLOR),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_SRCALPHASAT",     DestBlendCaps,  D3DPBLENDCAPS_SRCALPHASAT),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_BOTHSRCALPHA",    DestBlendCaps,  D3DPBLENDCAPS_BOTHSRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_BOTHINVSRCALPHA", DestBlendCaps,  D3DPBLENDCAPS_BOTHINVSRCALPHA),
        CAPSFLAGDEF(L"D3DPBLENDCAPS_BLENDFACTOR",     DestBlendCaps,  D3DPBLENDCAPS_BLENDFACTOR),
        CAPSFLAGDEFex(L"D3DPBLENDCAPS_SRCCOLOR2",	 DestBlendCaps,  D3DPBLENDCAPS_SRCCOLOR2),
        CAPSFLAGDEFex(L"D3DPBLENDCAPS_INVSRCCOLOR2",  DestBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR2),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsAlphaCmpCaps[] =
    {
        CAPSFLAGDEF(L"D3DPCMPCAPS_NEVER",             AlphaCmpCaps,   D3DPCMPCAPS_NEVER),
        CAPSFLAGDEF(L"D3DPCMPCAPS_LESS",              AlphaCmpCaps,   D3DPCMPCAPS_LESS),
        CAPSFLAGDEF(L"D3DPCMPCAPS_EQUAL",             AlphaCmpCaps,   D3DPCMPCAPS_EQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_LESSEQUAL",         AlphaCmpCaps,   D3DPCMPCAPS_LESSEQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_GREATER",           AlphaCmpCaps,   D3DPCMPCAPS_GREATER),
        CAPSFLAGDEF(L"D3DPCMPCAPS_NOTEQUAL",          AlphaCmpCaps,   D3DPCMPCAPS_NOTEQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_GREATEREQUAL",      AlphaCmpCaps,   D3DPCMPCAPS_GREATEREQUAL),
        CAPSFLAGDEF(L"D3DPCMPCAPS_ALWAYS",            AlphaCmpCaps,   D3DPCMPCAPS_ALWAYS),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsShadeCaps[] =
    {
        CAPSFLAGDEF(L"D3DPSHADECAPS_COLORGOURAUDRGB",      ShadeCaps,  D3DPSHADECAPS_COLORGOURAUDRGB),
        CAPSFLAGDEF(L"D3DPSHADECAPS_SPECULARGOURAUDRGB",   ShadeCaps,  D3DPSHADECAPS_SPECULARGOURAUDRGB),
        CAPSFLAGDEF(L"D3DPSHADECAPS_ALPHAGOURAUDBLEND",    ShadeCaps,  D3DPSHADECAPS_ALPHAGOURAUDBLEND),
        CAPSFLAGDEF(L"D3DPSHADECAPS_FOGGOURAUD",           ShadeCaps,  D3DPSHADECAPS_FOGGOURAUD),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsTextureCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_PERSPECTIVE",        TextureCaps,        D3DPTEXTURECAPS_PERSPECTIVE),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_POW2",               TextureCaps,        D3DPTEXTURECAPS_POW2),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_ALPHA",              TextureCaps,        D3DPTEXTURECAPS_ALPHA),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_SQUAREONLY",         TextureCaps,        D3DPTEXTURECAPS_SQUAREONLY),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE", TextureCaps,  D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_ALPHAPALETTE",       TextureCaps,        D3DPTEXTURECAPS_ALPHAPALETTE),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_NONPOW2CONDITIONAL", TextureCaps,        D3DPTEXTURECAPS_NONPOW2CONDITIONAL),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_PROJECTED",          TextureCaps,        D3DPTEXTURECAPS_PROJECTED),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_CUBEMAP",            TextureCaps,        D3DPTEXTURECAPS_CUBEMAP),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_VOLUMEMAP",          TextureCaps,        D3DPTEXTURECAPS_VOLUMEMAP),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_MIPMAP",             TextureCaps,        D3DPTEXTURECAPS_MIPMAP),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_MIPVOLUMEMAP",       TextureCaps,        D3DPTEXTURECAPS_MIPVOLUMEMAP),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_MIPCUBEMAP",         TextureCaps,        D3DPTEXTURECAPS_MIPCUBEMAP),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_CUBEMAP_POW2",       TextureCaps,        D3DPTEXTURECAPS_CUBEMAP_POW2),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_VOLUMEMAP_POW2",     TextureCaps,        D3DPTEXTURECAPS_VOLUMEMAP_POW2),
        CAPSFLAGDEF(L"D3DPTEXTURECAPS_NOPROJECTEDBUMPENV", TextureCaps,        D3DPTEXTURECAPS_NOPROJECTEDBUMPENV),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsTextureFilterCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPOINT",         TextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFLINEAR",        TextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFANISOTROPIC",   TextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", TextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  TextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFPOINT",         TextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFLINEAR",        TextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPOINT",         TextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFLINEAR",        TextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFANISOTROPIC",   TextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", TextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  TextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD),
        CAPSFLAGDEFex(L"D3DPTFILTERCAPS_CONVOLUTIONMONO", TextureFilterCaps,  D3DPTFILTERCAPS_CONVOLUTIONMONO),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsCubeTextureFilterCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPOINT",         CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFLINEAR",        CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFANISOTROPIC",   CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFPOINT",         CubeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFLINEAR",        CubeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPOINT",         CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFLINEAR",        CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFANISOTROPIC",   CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsVolumeTextureFilterCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPOINT",         VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFLINEAR",        VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFANISOTROPIC",   VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFPOINT",         VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFLINEAR",        VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPOINT",         VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFLINEAR",        VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFANISOTROPIC",   VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsStretchRectFilterCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPOINT",         StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFLINEAR",        StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFANISOTROPIC",   StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFPOINT",         StretchRectFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFLINEAR",        StretchRectFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPOINT",         StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFLINEAR",        StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFANISOTROPIC",   StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsTextureAddressCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_WRAP",              TextureAddressCaps, D3DPTADDRESSCAPS_WRAP),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_MIRROR",            TextureAddressCaps, D3DPTADDRESSCAPS_MIRROR),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_CLAMP",             TextureAddressCaps, D3DPTADDRESSCAPS_CLAMP),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_BORDER",            TextureAddressCaps, D3DPTADDRESSCAPS_BORDER),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_INDEPENDENTUV",     TextureAddressCaps, D3DPTADDRESSCAPS_INDEPENDENTUV),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_MIRRORONCE",        TextureAddressCaps, D3DPTADDRESSCAPS_MIRRORONCE),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsVolumeTextureAddressCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_WRAP",              VolumeTextureAddressCaps, D3DPTADDRESSCAPS_WRAP),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_MIRROR",            VolumeTextureAddressCaps, D3DPTADDRESSCAPS_MIRROR),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_CLAMP",             VolumeTextureAddressCaps, D3DPTADDRESSCAPS_CLAMP),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_BORDER",            VolumeTextureAddressCaps, D3DPTADDRESSCAPS_BORDER),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_INDEPENDENTUV",     VolumeTextureAddressCaps, D3DPTADDRESSCAPS_INDEPENDENTUV),
        CAPSFLAGDEF(L"D3DPTADDRESSCAPS_MIRRORONCE",        VolumeTextureAddressCaps, D3DPTADDRESSCAPS_MIRRORONCE),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsDevCaps2[] =
    {
        CAPSFLAGDEF(L"D3DDEVCAPS2_STREAMOFFSET",           DevCaps2, D3DDEVCAPS2_STREAMOFFSET),
        CAPSFLAGDEF(L"D3DDEVCAPS2_DMAPNPATCH",             DevCaps2, D3DDEVCAPS2_DMAPNPATCH),
        CAPSFLAGDEF(L"D3DDEVCAPS2_ADAPTIVETESSRTPATCH",    DevCaps2, D3DDEVCAPS2_ADAPTIVETESSRTPATCH),
        CAPSFLAGDEF(L"D3DDEVCAPS2_ADAPTIVETESSNPATCH",     DevCaps2, D3DDEVCAPS2_ADAPTIVETESSNPATCH),
        CAPSFLAGDEF(L"D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES", DevCaps2, D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES),
        CAPSFLAGDEF(L"D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH",   DevCaps2, D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH),
        CAPSFLAGDEF(L"D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET",   DevCaps2, D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsDeclTypes[] =
    {
        CAPSFLAGDEF(L"D3DDTCAPS_UBYTE4",                   DeclTypes, D3DDTCAPS_UBYTE4),
        CAPSFLAGDEF(L"D3DDTCAPS_UBYTE4N",                  DeclTypes, D3DDTCAPS_UBYTE4N),
        CAPSFLAGDEF(L"D3DDTCAPS_SHORT2N",                  DeclTypes, D3DDTCAPS_SHORT2N),
        CAPSFLAGDEF(L"D3DDTCAPS_SHORT4N",                  DeclTypes, D3DDTCAPS_SHORT4N),
        CAPSFLAGDEF(L"D3DDTCAPS_USHORT2N",                 DeclTypes, D3DDTCAPS_USHORT2N),
        CAPSFLAGDEF(L"D3DDTCAPS_USHORT4N",                 DeclTypes, D3DDTCAPS_USHORT4N),
        CAPSFLAGDEF(L"D3DDTCAPS_UDEC3",                    DeclTypes, D3DDTCAPS_UDEC3),
        CAPSFLAGDEF(L"D3DDTCAPS_DEC3N",                    DeclTypes, D3DDTCAPS_DEC3N),
        CAPSFLAGDEF(L"D3DDTCAPS_FLOAT16_2",                DeclTypes, D3DDTCAPS_FLOAT16_2),
        CAPSFLAGDEF(L"D3DDTCAPS_FLOAT16_4",                DeclTypes, D3DDTCAPS_FLOAT16_4),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsVS20Caps[] =
    {
        CAPSFLAGDEF(L"D3DVS20CAPS_PREDICATION",            VS20Caps.Caps, D3DVS20CAPS_PREDICATION),
        CAPSVALDEF(L"DynamicFlowControlDepth",             VS20Caps.DynamicFlowControlDepth),
        CAPSVALDEF(L"NumTemps",                            VS20Caps.NumTemps),
        CAPSVALDEF(L"StaticFlowControlDepth",              VS20Caps.StaticFlowControlDepth),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsPS20Caps[] =
    {
        CAPSFLAGDEF(L"D3DPS20CAPS_ARBITRARYSWIZZLE",       PS20Caps.Caps, D3DPS20CAPS_ARBITRARYSWIZZLE),
        CAPSFLAGDEF(L"D3DPS20CAPS_GRADIENTINSTRUCTIONS",   PS20Caps.Caps, D3DPS20CAPS_GRADIENTINSTRUCTIONS),
        CAPSFLAGDEF(L"D3DPS20CAPS_PREDICATION",            PS20Caps.Caps, D3DPS20CAPS_PREDICATION),
        CAPSFLAGDEF(L"D3DPS20CAPS_NODEPENDENTREADLIMIT",   PS20Caps.Caps, D3DPS20CAPS_NODEPENDENTREADLIMIT),
        CAPSFLAGDEF(L"D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT",  PS20Caps.Caps, D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT),
        CAPSVALDEF(L"DynamicFlowControlDepth",             PS20Caps.DynamicFlowControlDepth),
        CAPSVALDEF(L"NumTemps",                            PS20Caps.NumTemps),
        CAPSVALDEF(L"StaticFlowControlDepth",              PS20Caps.StaticFlowControlDepth),
        CAPSVALDEF(L"NumInstructionSlots",                 PS20Caps.NumInstructionSlots),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF CapsVertexTextureFilterCaps[] =
    {
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPOINT",         VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFLINEAR",        VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFANISOTROPIC",   VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFPOINT",         VertexTextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MIPFLINEAR",        VertexTextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPOINT",         VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFLINEAR",        VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFANISOTROPIC",   VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD),
        CAPSFLAGDEF(L"D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD),
        {L"",0,0}
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEFS DXGCapDefs[] =
    {
        {L"",                        nullptr,         (LPARAM)0,               },
        {L"+Caps",                   DXGDisplayCaps,  (LPARAM)DXGGenCaps,      },
            {L"Caps",                  DXGDisplayCaps,  (LPARAM)CapsCaps,        },
            {L"Caps2",                 DXGDisplayCaps,  (LPARAM)CapsCaps2,       },
            {L"Caps3",                 DXGDisplayCaps,  (LPARAM)CapsCaps3,       },
            {L"PresentationIntervals", DXGDisplayCaps,  (LPARAM)CapsPresentationIntervals, },
            {L"CursorCaps",            DXGDisplayCaps,  (LPARAM)CapsCursorCaps,  },
            {L"DevCaps",               DXGDisplayCaps,  (LPARAM)CapsDevCaps,     },
            {L"PrimitiveMiscCaps",     DXGDisplayCaps,  (LPARAM)CapsPrimMiscCaps      },
            {L"RasterCaps",            DXGDisplayCaps,  (LPARAM)CapsRasterCaps        },
            {L"ZCmpCaps",              DXGDisplayCaps,  (LPARAM)CapsZCmpCaps          },
            {L"SrcBlendCaps",          DXGDisplayCaps,  (LPARAM)CapsSrcBlendCaps      },
            {L"DestBlendCaps",         DXGDisplayCaps,  (LPARAM)CapsDestBlendCaps     },
            {L"AlphaCmpCaps",          DXGDisplayCaps,  (LPARAM)CapsAlphaCmpCaps      },
            {L"ShadeCaps",             DXGDisplayCaps,  (LPARAM)CapsShadeCaps         },
            {L"TextureCaps",           DXGDisplayCaps,  (LPARAM)CapsTextureCaps       },

            {L"TextureFilterCaps",         DXGDisplayCaps,  (LPARAM)CapsTextureFilterCaps },
            {L"CubeTextureFilterCaps",     DXGDisplayCaps,  (LPARAM)CapsCubeTextureFilterCaps },
            {L"VolumeTextureFilterCaps",   DXGDisplayCaps,  (LPARAM)CapsVolumeTextureFilterCaps },
            {L"TextureAddressCaps",        DXGDisplayCaps,  (LPARAM)CapsTextureAddressCaps },
            {L"VolumeTextureAddressCaps",  DXGDisplayCaps,  (LPARAM)CapsVolumeTextureAddressCaps },

            {L"LineCaps",              DXGDisplayCaps,  (LPARAM)CapsLineCaps },
            {L"StencilCaps",           DXGDisplayCaps,  (LPARAM)CapsStencilCaps, },
            {L"FVFCaps",               DXGDisplayCaps,  (LPARAM)CapsFVFCaps,     },
            {L"TextureOpCaps",         DXGDisplayCaps,  (LPARAM)CapsTextureOpCaps, },
            {L"VertexProcessingCaps",  DXGDisplayCaps,  (LPARAM)CapsVertexProcessingCaps, },
            {L"DevCaps2",              DXGDisplayCaps,  (LPARAM)CapsDevCaps2, },
            {L"DeclTypes",             DXGDisplayCaps,  (LPARAM)CapsDeclTypes, },
            {L"StretchRectFilterCaps", DXGDisplayCaps,  (LPARAM)CapsStretchRectFilterCaps, },
            {L"VS20Caps",              DXGDisplayCaps,  (LPARAM)CapsVS20Caps, },
            {L"PS20Caps",              DXGDisplayCaps,  (LPARAM)CapsPS20Caps, },
            {L"VertexTextureFilterCaps",DXGDisplayCaps,  (LPARAM)CapsVertexTextureFilterCaps, },

            {L"-",                      nullptr,         (LPARAM)0,                },
        { nullptr, 0, 0 }
    };

    //-----------------------------------------------------------------------------
    const WCHAR* FormatName(D3DFORMAT format)
    {
        switch (format)
        {
        case D3DFMT_UNKNOWN:         return L"D3DFMT_UNKNOWN";
        case D3DFMT_R8G8B8:          return L"D3DFMT_R8G8B8";
        case D3DFMT_A8R8G8B8:        return L"D3DFMT_A8R8G8B8";
        case D3DFMT_X8R8G8B8:        return L"D3DFMT_X8R8G8B8";
        case D3DFMT_R5G6B5:          return L"D3DFMT_R5G6B5";
        case D3DFMT_X1R5G5B5:        return L"D3DFMT_X1R5G5B5";
        case D3DFMT_A1R5G5B5:        return L"D3DFMT_A1R5G5B5";
        case D3DFMT_A4R4G4B4:        return L"D3DFMT_A4R4G4B4";
        case D3DFMT_R3G3B2:          return L"D3DFMT_R3G3B2";
        case D3DFMT_A8:              return L"D3DFMT_A8";
        case D3DFMT_A8R3G3B2:        return L"D3DFMT_A8R3G3B2";
        case D3DFMT_X4R4G4B4:        return L"D3DFMT_X4R4G4B4";
        case D3DFMT_A2B10G10R10:     return L"D3DFMT_A2B10G10R10";
        case D3DFMT_A8B8G8R8:        return L"D3DFMT_A8B8G8R8";
        case D3DFMT_X8B8G8R8:        return L"D3DFMT_X8B8G8R8";
        case D3DFMT_G16R16:          return L"D3DFMT_G16R16";
        case D3DFMT_A2R10G10B10:     return L"D3DFMT_A2R10G10B10";
        case D3DFMT_A16B16G16R16:    return L"D3DFMT_A16B16G16R16";

        case D3DFMT_A8P8:            return L"D3DFMT_A8P8";
        case D3DFMT_P8:              return L"D3DFMT_P8";

        case D3DFMT_L8:              return L"D3DFMT_L8";
        case D3DFMT_A8L8:            return L"D3DFMT_A8L8";
        case D3DFMT_A4L4:            return L"D3DFMT_A4L4";

        case D3DFMT_V8U8:            return L"D3DFMT_V8U8";
        case D3DFMT_L6V5U5:          return L"D3DFMT_L6V5U5";
        case D3DFMT_X8L8V8U8:        return L"D3DFMT_X8L8V8U8";
        case D3DFMT_Q8W8V8U8:        return L"D3DFMT_Q8W8V8U8";
        case D3DFMT_V16U16:          return L"D3DFMT_V16U16";
        case D3DFMT_A2W10V10U10:     return L"D3DFMT_A2W10V10U10";

        case D3DFMT_UYVY:            return L"D3DFMT_UYVY";
        case D3DFMT_R8G8_B8G8:       return L"D3DFMT_R8G8_B8G8";
        case D3DFMT_YUY2:            return L"D3DFMT_YUY2";
        case D3DFMT_G8R8_G8B8:       return L"D3DFMT_G8R8_G8B8";
        case D3DFMT_DXT1:            return L"D3DFMT_DXT1";
        case D3DFMT_DXT2:            return L"D3DFMT_DXT2";
        case D3DFMT_DXT3:            return L"D3DFMT_DXT3";
        case D3DFMT_DXT4:            return L"D3DFMT_DXT4";
        case D3DFMT_DXT5:            return L"D3DFMT_DXT5";

        case D3DFMT_D16_LOCKABLE:    return L"D3DFMT_D16_LOCKABLE";
        case D3DFMT_D32:             return L"D3DFMT_D32";
        case D3DFMT_D15S1:           return L"D3DFMT_D15S1";
        case D3DFMT_D24S8:           return L"D3DFMT_D24S8";
        case D3DFMT_D24X8:           return L"D3DFMT_D24X8";
        case D3DFMT_D24X4S4:         return L"D3DFMT_D24X4S4";
        case D3DFMT_D16:             return L"D3DFMT_D16";
        case D3DFMT_D32F_LOCKABLE:   return L"D3DFMT_D32F_LOCKABLE";
        case D3DFMT_D24FS8:          return L"D3DFMT_D24FS8";

        case D3DFMT_L16:             return L"D3DFMT_L16";

        case D3DFMT_VERTEXDATA:      return L"D3DFMT_VERTEXDATA";
        case D3DFMT_INDEX16:         return L"D3DFMT_INDEX16";
        case D3DFMT_INDEX32:         return L"D3DFMT_INDEX32";

        case D3DFMT_Q16W16V16U16:    return L"D3DFMT_Q16W16V16U16";

        case D3DFMT_MULTI2_ARGB8:    return L"D3DFMT_MULTI2_ARGB8";

        case D3DFMT_R16F:            return L"D3DFMT_R16F";
        case D3DFMT_G16R16F:         return L"D3DFMT_G16R16F";
        case D3DFMT_A16B16G16R16F:   return L"D3DFMT_A16B16G16R16F";

        case D3DFMT_R32F:            return L"D3DFMT_R32F";
        case D3DFMT_G32R32F:         return L"D3DFMT_G32R32F";
        case D3DFMT_A32B32G32R32F:   return L"D3DFMT_A32B32G32R32F";

        case D3DFMT_CxV8U8:          return L"D3DFMT_CxV8U8";

        case D3DFMT_D32_LOCKABLE:	 return L"D3DFMT_D32_LOCKABLE";
        case D3DFMT_S8_LOCKABLE:	 return L"D3DFMT_S8_LOCKABLE";
        case D3DFMT_A1:				 return L"D3DFMT_A1";

        default:                     return L"Unknown format";
        }
    }


    //-----------------------------------------------------------------------------
    const WCHAR* MultiSampleTypeName(D3DMULTISAMPLE_TYPE msType)
    {
        switch (msType)
        {
        case D3DMULTISAMPLE_NONE:       return L"D3DMULTISAMPLE_NONE";
        case D3DMULTISAMPLE_NONMASKABLE: return L"D3DMULTISAMPLE_NONMASKABLE";
        case D3DMULTISAMPLE_2_SAMPLES:  return L"D3DMULTISAMPLE_2_SAMPLES";
        case D3DMULTISAMPLE_3_SAMPLES:  return L"D3DMULTISAMPLE_3_SAMPLES";
        case D3DMULTISAMPLE_4_SAMPLES:  return L"D3DMULTISAMPLE_4_SAMPLES";
        case D3DMULTISAMPLE_5_SAMPLES:  return L"D3DMULTISAMPLE_5_SAMPLES";
        case D3DMULTISAMPLE_6_SAMPLES:  return L"D3DMULTISAMPLE_6_SAMPLES";
        case D3DMULTISAMPLE_7_SAMPLES:  return L"D3DMULTISAMPLE_7_SAMPLES";
        case D3DMULTISAMPLE_8_SAMPLES:  return L"D3DMULTISAMPLE_8_SAMPLES";
        case D3DMULTISAMPLE_9_SAMPLES:  return L"D3DMULTISAMPLE_9_SAMPLES";
        case D3DMULTISAMPLE_10_SAMPLES: return L"D3DMULTISAMPLE_10_SAMPLES";
        case D3DMULTISAMPLE_11_SAMPLES: return L"D3DMULTISAMPLE_11_SAMPLES";
        case D3DMULTISAMPLE_12_SAMPLES: return L"D3DMULTISAMPLE_12_SAMPLES";
        case D3DMULTISAMPLE_13_SAMPLES: return L"D3DMULTISAMPLE_13_SAMPLES";
        case D3DMULTISAMPLE_14_SAMPLES: return L"D3DMULTISAMPLE_14_SAMPLES";
        case D3DMULTISAMPLE_15_SAMPLES: return L"D3DMULTISAMPLE_15_SAMPLES";
        case D3DMULTISAMPLE_16_SAMPLES: return L"D3DMULTISAMPLE_16_SAMPLES";
        default:                        return L"Unknown type";
        }
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the adapter index
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayAdapterInfo(LPARAM lParam1, LPARAM /*lParam2*/,
        _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        auto iAdapter = static_cast<UINT>(lParam1);

        if (!g_pD3D)
            return S_OK;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Name", 15);
            LVAddColumn(g_hwndLV, 1, L"Value", 40);
        }

        D3DADAPTER_IDENTIFIER9 identifier;
        HRESULT hr = g_pD3D->GetAdapterIdentifier(iAdapter, D3DENUM_WHQL_LEVEL, &identifier);
        if (FAILED(hr))
            return hr;

        GUID guid = identifier.DeviceIdentifier;
        WCHAR szGuid[50];
        swprintf_s(szGuid, 50, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
            guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

        WCHAR szVersion[50];
        swprintf_s(szVersion, 50, L"0x%08X-%08X", identifier.DriverVersion.HighPart,
            identifier.DriverVersion.LowPart);

        WideString wideDriver(identifier.Driver);
        WideString wideDescription(identifier.Description);
        WideString wideDeviceName(identifier.DeviceName);

        if (!pPrintInfo)
        {


            LVAddText(g_hwndLV, 0, L"Driver");
            LVAddText(g_hwndLV, 1, L"%s", wideDriver.c_str());

            LVAddText(g_hwndLV, 0, L"Description");
            LVAddText(g_hwndLV, 1, L"%s", wideDescription.c_str());

            LVAddText(g_hwndLV, 0, L"DeviceName");
            LVAddText(g_hwndLV, 1, L"%s", wideDeviceName.c_str());

            LVAddText(g_hwndLV, 0, L"DriverVersion");
            LVAddText(g_hwndLV, 1, L"%s", szVersion);

            LVAddText(g_hwndLV, 0, L"VendorId");
            LVAddText(g_hwndLV, 1, L"0x%08x", identifier.VendorId);

            LVAddText(g_hwndLV, 0, L"DeviceId");
            LVAddText(g_hwndLV, 1, L"0x%08x", identifier.DeviceId);

            LVAddText(g_hwndLV, 0, L"SubSysId");
            LVAddText(g_hwndLV, 1, L"0x%08x", identifier.SubSysId);

            LVAddText(g_hwndLV, 0, L"Revision");
            LVAddText(g_hwndLV, 1, L"%d", identifier.Revision);

            LVAddText(g_hwndLV, 0, L"DeviceIdentifier");
            LVAddText(g_hwndLV, 1, szGuid);

            LVAddText(g_hwndLV, 0, L"WHQLLevel");
            LVAddText(g_hwndLV, 1, L"%d", identifier.WHQLLevel);
        }
        else
        {
            PrintStringValueLine(L"Driver", wideDriver.c_str(), pPrintInfo);
            PrintStringValueLine(L"Description", wideDescription.c_str(), pPrintInfo);
            PrintStringValueLine(L"DriverVersion", szVersion, pPrintInfo);
            PrintHexValueLine(L"VendorId", identifier.VendorId, pPrintInfo);
            PrintHexValueLine(L"DeviceId", identifier.DeviceId, pPrintInfo);
            PrintHexValueLine(L"SubSysId", identifier.SubSysId, pPrintInfo);
            PrintValueLine(L"Revision", identifier.Revision, pPrintInfo);
            PrintStringValueLine(L"DeviceIdentifier", szGuid, pPrintInfo);
            PrintValueLine(L"WHQLLevel", identifier.WHQLLevel, pPrintInfo);
        }
        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the adapter index
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayModes(LPARAM lParam1, LPARAM /*lParam2*/, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        auto iAdapter = static_cast<UINT>(lParam1);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Resolution", 10);
            LVAddColumn(g_hwndLV, 1, L"Pixel Format", 15);
            LVAddColumn(g_hwndLV, 2, L"Refresh Rate", 10);
        }

        for (INT iFormat = 0; iFormat < NumAdapterFormats; iFormat++)
        {
            D3DFORMAT fmt = AdapterFormatArray[iFormat];
            UINT numModes = g_pD3D->GetAdapterModeCount(iAdapter, fmt);
            for (UINT iMode = 0; iMode < numModes; iMode++)
            {
                D3DDISPLAYMODE mode;
                g_pD3D->EnumAdapterModes(iAdapter, fmt, iMode, &mode);
                if (!pPrintInfo)
                {
                    LVAddText(g_hwndLV, 0, L"%d x %d", mode.Width, mode.Height);
                    LVAddText(g_hwndLV, 1, FormatName(mode.Format));
                    LVAddText(g_hwndLV, 2, L"%d", mode.RefreshRate);
                }
                else
                {
                    WCHAR szBuff[80];
                    DWORD cchLen;
                    int   x1, x2, x3, yLine;

                    // Calculate Name and Value column x offsets
                    x1 = (pPrintInfo->dwCurrIndent * DEF_TAB_SIZE * pPrintInfo->dwCharWidth);
                    x2 = x1 + (20 * pPrintInfo->dwCharWidth);
                    x3 = x2 + (20 * pPrintInfo->dwCharWidth);
                    yLine = (pPrintInfo->dwCurrLine * pPrintInfo->dwLineHeight);

                    swprintf_s(szBuff, 80, L"%u x %u", mode.Width, mode.Height);
                    cchLen = static_cast<DWORD>(wcslen(szBuff));
                    if (FAILED(PrintLine(x1, yLine, szBuff, cchLen, pPrintInfo)))
                        return E_FAIL;

                    wcscpy_s(szBuff, 80, FormatName(mode.Format));
                    cchLen = static_cast<DWORD>(wcslen(szBuff));
                    if (FAILED(PrintLine(x2, yLine, szBuff, cchLen, pPrintInfo)))
                        return E_FAIL;

                    swprintf_s(szBuff, 80, L"%u", mode.RefreshRate);
                    cchLen = static_cast<DWORD>(wcslen(szBuff));
                    if (FAILED(PrintLine(x3, yLine, szBuff, cchLen, pPrintInfo)))
                        return E_FAIL;

                    // Advance to next line on page
                    if (FAILED(PrintNextLine(pPrintInfo)))
                        return E_FAIL;
                }
            }
        }
        return S_OK;
    }

    //-----------------------------------------------------------------------------
    // lParam1 is the caps pointer
    // lParam2 is the CAPDEF table we should use
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayCaps(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        auto pCaps = reinterpret_cast<D3DCAPS9*>(lParam1);
        auto pCapDef = reinterpret_cast<CAPDEF*>(lParam2);

        if (pPrintInfo)
            return PrintCapsToDC(pCapDef, (VOID*)pCaps, pPrintInfo);
        else
            AddCapsToLV(pCapDef, (LPVOID)pCaps);

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is bWindowed and the msType
    // lParam3 is the render format
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayMultiSample(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        BOOL bWindowed = (BOOL)LOWORD(lParam2);
        auto msType = static_cast<D3DMULTISAMPLE_TYPE>(HIWORD(lParam2));
        auto fmt = static_cast<D3DFORMAT>(lParam3);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Quality Levels", 30);
        }

        DWORD dwNumQualityLevels;
        if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, fmt, bWindowed, msType, &dwNumQualityLevels)))
        {
            WCHAR str[100];
            if (dwNumQualityLevels == 1)
                swprintf_s(str, 100, L"%u quality level", dwNumQualityLevels);
            else
                swprintf_s(str, 100, L"%u quality levels", dwNumQualityLevels);
            if (!pPrintInfo)
            {
                LVAddText(g_hwndLV, 0, str);
            }
            else
            {
                PrintStringLine(str, pPrintInfo);
            }
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is the adapter fmt
    // lParam3 is bWindowed
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayBackBuffer(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        auto fmtAdapter = static_cast<D3DFORMAT>(lParam2);
        auto bWindowed = static_cast<BOOL>(lParam3);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Back Buffer Formats", 20);
        }

        for (int iFmt = 0; iFmt < NumBBFormats; iFmt++)
        {
            D3DFORMAT fmt = BBFormatArray[iFmt];
            if (SUCCEEDED(g_pD3D->CheckDeviceType(iAdapter, devType, fmtAdapter, fmt, bWindowed)))
            {
                if (!pPrintInfo)
                {
                    LVAddText(g_hwndLV, 0, L"%s", FormatName(fmt));
                }
                else
                {
                    PrintStringLine(FormatName(fmt), pPrintInfo);
                }
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is the adapter fmt
    // lParam3 is unused
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayRenderTarget(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        auto fmtAdapter = static_cast<D3DFORMAT>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Render Target Formats", 20);
        }

        for (int iFmt = 0; iFmt < NumFormats; iFmt++)
        {
            D3DFORMAT fmt = AllFormatArray[iFmt];
            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_RENDERTARGET,
                D3DRTYPE_SURFACE, fmt)))
            {
                if (!pPrintInfo)
                {
                    LVAddText(g_hwndLV, 0, L"%s", FormatName(fmt));
                }
                else
                {
                    PrintStringLine(FormatName(fmt), pPrintInfo);
                }
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is the adapter fmt
    // lParam3 is unused
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayDepthStencil(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        auto fmtAdapter = static_cast<D3DFORMAT>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Depth/Stencil Formats", 20);
        }

        for (int iFmt = 0; iFmt < NumDSFormats; iFmt++)
        {
            D3DFORMAT fmt = DSFormatArray[iFmt];

            if (!g_is9Ex && ((fmt == D3DFMT_D32_LOCKABLE) || (fmt == D3DFMT_S8_LOCKABLE)))
                continue;

            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_DEPTHSTENCIL,
                D3DRTYPE_SURFACE, fmt)))
            {
                if (!pPrintInfo)
                {
                    LVAddText(g_hwndLV, 0, L"%s", FormatName(fmt));
                }
                else
                {
                    PrintStringLine(FormatName(fmt), pPrintInfo);
                }
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is the depth/stencil fmt
    // lParam3 is the msType
    //-----------------------------------------------------------------------------
    HRESULT DXGCheckDSQualityLevels(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        auto fmtDS = static_cast<D3DFORMAT>(lParam2);
        auto msType = static_cast<D3DMULTISAMPLE_TYPE>(lParam3);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Quality Levels", 20);
        }

        DWORD dwNumQualityLevels;
        if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, fmtDS, FALSE, msType, &dwNumQualityLevels)))
        {
            WCHAR str[100];
            if (dwNumQualityLevels == 1)
                swprintf_s(str, 100, L"%u quality level", dwNumQualityLevels);
            else
                swprintf_s(str, 100, L"%u quality levels", dwNumQualityLevels);
            if (!pPrintInfo)
            {
                LVAddText(g_hwndLV, 0, str);
            }
            else
            {
                PrintStringLine(str, pPrintInfo);
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is the adapter fmt
    // lParam3 is unused
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayPlainSurface(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParam3*/, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        D3DFORMAT fmtAdapter = static_cast<D3DFORMAT>(lParam2);

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Plain Surface Formats", 20);
        }

        for (int iFmt = 0; iFmt < NumFormats; iFmt++)
        {
            D3DFORMAT fmt = AllFormatArray[iFmt];

            if (!g_is9Ex && ((fmt == D3DFMT_A1) || (fmt == D3DFMT_D32_LOCKABLE) || (fmt == D3DFMT_S8_LOCKABLE)))
                continue;

            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, 0,
                D3DRTYPE_SURFACE, fmt)))
            {
                if (!pPrintInfo)
                {
                    LVAddText(g_hwndLV, 0, L"%s", FormatName(fmt));
                }
                else
                {
                    PrintStringLine(FormatName(fmt), pPrintInfo);
                }
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // lParam1 is the iAdapter and device type
    // lParam2 is the fmt of the swap chain
    // lParam3 is the D3DRESOURCETYPE
    //-----------------------------------------------------------------------------
    HRESULT DXGDisplayResource(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        UINT iAdapter = LOWORD(lParam1);
        auto devType = static_cast<D3DDEVTYPE>(HIWORD(lParam1));
        auto fmtAdapter = static_cast<D3DFORMAT>(lParam2);
        auto RType = static_cast<D3DRESOURCETYPE>(lParam3);
        const WCHAR* pstr;
        HRESULT hr;
        UINT col = 0;

        D3DCAPS9 Caps;
        g_pD3D->GetDeviceCaps(iAdapter, devType, &Caps);

        if (!pPrintInfo)
        {
            switch (RType)
            {
            case D3DRTYPE_SURFACE:
                LVAddColumn(g_hwndLV, col++, L"Surface Formats", 20);
                break;
            case D3DRTYPE_VOLUME:
                LVAddColumn(g_hwndLV, col++, L"Volume Formats", 20);
                break;
            case D3DRTYPE_TEXTURE:
                LVAddColumn(g_hwndLV, col++, L"Texture Formats", 20);
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                LVAddColumn(g_hwndLV, col++, L"Volume Texture Formats", 20);
                break;
            case D3DRTYPE_CUBETEXTURE:
                LVAddColumn(g_hwndLV, col++, L"Cube Texture Formats", 20);
                break;
            default:
                return E_FAIL;
            }
            LVAddColumn(g_hwndLV, col++, L"0 (Plain)", 22);
            if (RType != D3DRTYPE_VOLUMETEXTURE)
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_RENDERTARGET", 22);
            //        LVAddColumn(g_hwndLV,  col++, "D3DUSAGE_DEPTHSTENCIL", 22);
            if (RType != D3DRTYPE_SURFACE)
            {
                if (RType != D3DRTYPE_VOLUMETEXTURE)
                {
                    LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_AUTOGENMIPMAP", 22);
                    if (RType != D3DRTYPE_CUBETEXTURE)
                    {
                        LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_DMAP", 22);
                    }
                }
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_LEGACYBUMPMAP", 22);
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_SRGBREAD", 18);
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_FILTER", 15);
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_SRGBWRITE", 18);
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING", 18);
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_VERTEXTEXTURE", 18);
                LVAddColumn(g_hwndLV, col++, L"D3DUSAGE_QUERY_WRAPANDMIP", 18);
            }
        }
        const DWORD usageArray[] =
        {
            0,
            D3DUSAGE_RENDERTARGET,
            //        D3DUSAGE_DEPTHSTENCIL,
                    D3DUSAGE_AUTOGENMIPMAP,
                    D3DUSAGE_DMAP,
                    D3DUSAGE_QUERY_LEGACYBUMPMAP,
                    D3DUSAGE_QUERY_SRGBREAD,
                    D3DUSAGE_QUERY_FILTER,
                    D3DUSAGE_QUERY_SRGBWRITE,
                    D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                    D3DUSAGE_QUERY_VERTEXTEXTURE,
                    D3DUSAGE_QUERY_WRAPANDMIP,
        };
        const DWORD numUsages = sizeof(usageArray) / sizeof(usageArray[0]);
        BOOL bFoundSuccess;
        for (int iFmt = 0; iFmt < NumFormats; iFmt++)
        {
            D3DFORMAT fmt = AllFormatArray[iFmt];

            if (!g_is9Ex && ((fmt == D3DFMT_A1) || (fmt == D3DFMT_D32_LOCKABLE) || (fmt == D3DFMT_S8_LOCKABLE)))
                continue;

            // First pass: see if CDF succeeds for any usage
            bFoundSuccess = FALSE;
            for (UINT iUsage = 0; iUsage < numUsages; iUsage++)
            {
                if (RType == D3DRTYPE_SURFACE)
                {
                    if (usageArray[iUsage] != 0 &&
                        usageArray[iUsage] != D3DUSAGE_DEPTHSTENCIL &&
                        usageArray[iUsage] != D3DUSAGE_RENDERTARGET)
                    {
                        continue;
                    }
                }
                if (RType == D3DRTYPE_VOLUMETEXTURE)
                {
                    if (usageArray[iUsage] == D3DUSAGE_DEPTHSTENCIL ||
                        usageArray[iUsage] == D3DUSAGE_RENDERTARGET ||
                        usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP ||
                        usageArray[iUsage] == D3DUSAGE_DMAP)
                    {
                        continue;
                    }
                }
                if (RType == D3DRTYPE_CUBETEXTURE)
                {
                    if (usageArray[iUsage] == D3DUSAGE_DMAP)
                    {
                        continue;
                    }
                }
                if (usageArray[iUsage] == D3DUSAGE_DMAP)
                {
                    if ((Caps.DevCaps2 & D3DDEVCAPS2_DMAPNPATCH) == 0 &&
                        (Caps.DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH) == 0)
                    {
                        continue;
                    }
                }
                if (fmt == D3DFMT_MULTI2_ARGB8 && usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP)
                {
                    continue;
                }
                if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter,
                    usageArray[iUsage], RType, fmt)))
                {
                    bFoundSuccess = TRUE;
                    break;
                }
            }
            if (bFoundSuccess)
            {
                col = 0;
                // Add list item for this format
                if (!pPrintInfo)
                    LVAddText(g_hwndLV, col++, L"%s", FormatName(fmt));
                else
                    PrintStringLine(FormatName(fmt), pPrintInfo);

                // Show which usages it is compatible with
                for (UINT iUsage = 0; iUsage < numUsages; iUsage++)
                {
                    if (RType == D3DRTYPE_SURFACE)
                    {
                        if (usageArray[iUsage] != 0 &&
                            usageArray[iUsage] != D3DUSAGE_DEPTHSTENCIL &&
                            usageArray[iUsage] != D3DUSAGE_RENDERTARGET)
                        {
                            continue;
                        }
                    }
                    if (RType == D3DRTYPE_VOLUMETEXTURE)
                    {
                        if (usageArray[iUsage] == D3DUSAGE_DEPTHSTENCIL ||
                            usageArray[iUsage] == D3DUSAGE_RENDERTARGET ||
                            usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP ||
                            usageArray[iUsage] == D3DUSAGE_DMAP)
                        {
                            continue;
                        }
                    }
                    if (RType == D3DRTYPE_CUBETEXTURE)
                    {
                        if (usageArray[iUsage] == D3DUSAGE_DMAP)
                        {
                            continue;
                        }
                    }
                    hr = S_OK;
                    if (usageArray[iUsage] == D3DUSAGE_DMAP)
                    {
                        if ((Caps.DevCaps2 & D3DDEVCAPS2_DMAPNPATCH) == 0 &&
                            (Caps.DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH) == 0)
                        {
                            hr = E_FAIL;
                        }
                    }
                    if (fmt == D3DFMT_MULTI2_ARGB8 && usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP)
                    {
                        hr = E_FAIL;
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter,
                            usageArray[iUsage], RType, fmt);
                    }
                    if (hr == D3DOK_NOAUTOGEN)
                        pstr = L"No";
                    else if (SUCCEEDED(hr))
                        pstr = L"Yes";
                    else
                        pstr = L"No";
                    if (!pPrintInfo)
                        LVAddText(g_hwndLV, col++, pstr);
                    else
                        PrintStringLine(pstr, pPrintInfo);
                }
            }
        }

        return S_OK;
    }

    //-----------------------------------------------------------------------------
    BOOL IsBBFmt(D3DFORMAT fmt)
    {
        for (int iFmtBackBuffer = 0; iFmtBackBuffer < NumBBFormats; iFmtBackBuffer++)
        {
            D3DFORMAT fmtBackBuffer = BBFormatArray[iFmtBackBuffer];
            if (fmt == fmtBackBuffer)
                return TRUE;
        }
        return FALSE;
    }

    //-----------------------------------------------------------------------------
    BOOL IsDeviceTypeAvailable(UINT iAdapter, D3DDEVTYPE devType)
    {
        for (int iFmtAdapter = 0; iFmtAdapter < NumAdapterFormats; iFmtAdapter++)
        {
            D3DFORMAT fmtAdapter = AdapterFormatArray[iFmtAdapter];
            if (IsAdapterFmtAvailable(iAdapter, devType, fmtAdapter, TRUE) ||
                IsAdapterFmtAvailable(iAdapter, devType, fmtAdapter, FALSE))
            {
                return TRUE;
            }
        }
        return FALSE;
    }


    //-----------------------------------------------------------------------------
    BOOL IsAdapterFmtAvailable(UINT iAdapter, D3DDEVTYPE devType, D3DFORMAT fmtAdapter, BOOL bWindowed)
    {
        if (fmtAdapter == D3DFMT_A2R10G10B10 && bWindowed)
            return FALSE; // D3D extended primary formats are only avaiable for fullscreen mode

        for (int iFmtBackBuffer = 0; iFmtBackBuffer < NumBBFormats; iFmtBackBuffer++)
        {
            D3DFORMAT fmtBackBuffer = BBFormatArray[iFmtBackBuffer];
            if (SUCCEEDED(g_pD3D->CheckDeviceType(iAdapter, devType, fmtAdapter, fmtBackBuffer, bWindowed)))
            {
                return TRUE;
            }
        }
        return FALSE;
    }
}


//-----------------------------------------------------------------------------
// Name: DXG_Init()
//-----------------------------------------------------------------------------
VOID DXG_Init()
{
    g_is9Ex = FALSE;

    g_hInstD3D = LoadLibraryEx(L"d3d9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_hInstD3D)
    {
        g_direct3DCreate9 = reinterpret_cast<LPDIRECT3D9CREATE9>(GetProcAddress(g_hInstD3D, "Direct3DCreate9"));
        g_direct3DCreate9Ex = reinterpret_cast<LPDIRECT3D9CREATE9EX>(GetProcAddress(g_hInstD3D, "Direct3DCreate9Ex"));

        if (g_direct3DCreate9Ex)
        {
            g_is9Ex = SUCCEEDED(g_direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D));
        }

        if (!g_is9Ex && g_direct3DCreate9)
            g_pD3D = g_direct3DCreate9(D3D_SDK_VERSION);
    }
}


//-----------------------------------------------------------------------------
// Name: DXG_FillTree()
//-----------------------------------------------------------------------------
VOID DXG_FillTree(HWND hwndTV)
{
    HRESULT hr;
    D3DDEVTYPE deviceTypeArray[] = { D3DDEVTYPE_HAL, D3DDEVTYPE_SW, D3DDEVTYPE_REF };
    D3DDEVTYPE devType;
    UINT iDevice;
    D3DCAPS9 caps;
    D3DCAPS9* pCapsCopy;

    static const WCHAR* deviceNameArray[] = { L"HAL", L"Software", L"Reference" };
    static const UINT numDeviceTypes = sizeof(deviceTypeArray) / sizeof(deviceTypeArray[0]);

    if (!g_pD3D)
        return;

    HTREEITEM hTree = TVAddNode(TVI_ROOT, L"Direct3D9 Devices", TRUE, IDI_DIRECTX,
        nullptr, 0, 0);

    UINT numAdapters = g_pD3D->GetAdapterCount();
    for (UINT iAdapter = 0; iAdapter < numAdapters; iAdapter++)
    {
        D3DADAPTER_IDENTIFIER9 identifier;
        if (SUCCEEDED(g_pD3D->GetAdapterIdentifier(iAdapter, 0, &identifier)))
        {
            WideString wideDescription(identifier.Description);
            HTREEITEM hTree2 = TVAddNode(hTree, wideDescription.c_str(), TRUE, IDI_CAPS,
                DXGDisplayAdapterInfo, iAdapter, 0);
            (void)TVAddNode(hTree2, L"Display Modes", FALSE, IDI_CAPS,
                DXGDisplayModes, iAdapter, 0);
            HTREEITEM hTree3 = TVAddNode(hTree2, L"D3D Device Types", TRUE, IDI_CAPS,
                nullptr, 0, 0);

            for (iDevice = 0; iDevice < numDeviceTypes; iDevice++)
            {
                devType = deviceTypeArray[iDevice];

                if (devType == D3DDEVTYPE_SW)
                    continue; // we don't register a SW device, so just skip it

                if (!IsDeviceTypeAvailable(iAdapter, devType))
                    continue;

                // Add caps for each device
                hr = g_pD3D->GetDeviceCaps(iAdapter, devType, &caps);
                if (FAILED(hr))
                    memset(&caps, 0, sizeof(caps));
                pCapsCopy = new (std::nothrow) D3DCAPS9;
                if (!pCapsCopy)
                    continue;
                *pCapsCopy = caps;
                HTREEITEM hTree4 = TVAddNode(hTree3, deviceNameArray[iDevice], TRUE, IDI_CAPS, nullptr, 0, 0);
                AddCapsToTV(hTree4, DXGCapDefs, (LPARAM)pCapsCopy);

                // List adapter formats for each device
                HTREEITEM hTree5 = TVAddNode(hTree4, L"Adapter Formats", TRUE, IDI_CAPS, nullptr, 0, 0);
                D3DFORMAT fmtAdapter;
                for (int iFmtAdapter = 0; iFmtAdapter < NumAdapterFormats; iFmtAdapter++)
                {
                    fmtAdapter = AdapterFormatArray[iFmtAdapter];
                    for (BOOL bWindowed = FALSE; bWindowed < 2; bWindowed++)
                    {
                        if (!IsAdapterFmtAvailable(iAdapter, devType, fmtAdapter, bWindowed))
                            continue;

                        WCHAR sz[100];
                        swprintf_s(sz, 100, L"%s %s", FormatName(fmtAdapter), bWindowed ? L"(Windowed)" : L"(Fullscreen)");
                        HTREEITEM hTree6 = TVAddNode(hTree5, sz, TRUE, IDI_CAPS, nullptr, 0, 0);
                        TVAddNodeEx(hTree6, L"Back Buffer Formats", FALSE, IDI_CAPS, DXGDisplayBackBuffer, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)bWindowed);
                        TVAddNodeEx(hTree6, L"Render Target Formats", FALSE, IDI_CAPS, DXGDisplayRenderTarget, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)0);
                        TVAddNodeEx(hTree6, L"Depth/Stencil Formats", FALSE, IDI_CAPS, DXGDisplayDepthStencil, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)0);
                        TVAddNodeEx(hTree6, L"Plain Surface Formats", FALSE, IDI_CAPS, DXGDisplayPlainSurface, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)0);
                        TVAddNodeEx(hTree6, L"Texture Formats", FALSE, IDI_CAPS, DXGDisplayResource, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)D3DRTYPE_TEXTURE);
                        TVAddNodeEx(hTree6, L"Cube Texture Formats", FALSE, IDI_CAPS, DXGDisplayResource, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)D3DRTYPE_CUBETEXTURE);
                        TVAddNodeEx(hTree6, L"Volume Texture Formats", FALSE, IDI_CAPS, DXGDisplayResource, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)D3DRTYPE_VOLUMETEXTURE);
                        HTREEITEM hTree7 = TVAddNode(hTree6, L"Render Format Compatibility", TRUE, IDI_CAPS, nullptr, 0, 0);
                        D3DFORMAT fmtRender;
                        for (int iFmtRender = 0; iFmtRender < NumFormats; iFmtRender++)
                        {
                            fmtRender = AllFormatArray[iFmtRender];
                            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, fmtRender))
                                || (IsBBFmt(fmtRender) && SUCCEEDED(g_pD3D->CheckDeviceType(iAdapter, devType, fmtAdapter, fmtRender, bWindowed))))
                            {
                                HTREEITEM hTree8 = TVAddNode(hTree7, FormatName(fmtRender), TRUE, IDI_CAPS, nullptr, 0, 0);
                                for (D3DMULTISAMPLE_TYPE msType = D3DMULTISAMPLE_NONE; msType <= D3DMULTISAMPLE_16_SAMPLES; msType = (D3DMULTISAMPLE_TYPE)((UINT)msType + 1))
                                {
                                    if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, fmtRender, bWindowed, msType, nullptr)))
                                    {
                                        HTREEITEM hTree9 = TVAddNodeEx(hTree8, MultiSampleTypeName(msType), TRUE, IDI_CAPS, DXGDisplayMultiSample, MAKELPARAM(iAdapter, (UINT)devType), MAKELPARAM(bWindowed, (UINT)msType), (LPARAM)fmtRender);
                                        HTREEITEM hTree10 = TVAddNode(hTree9, L"Compatible Depth/Stencil Formats", TRUE, IDI_CAPS, nullptr, 0, 0);
                                        D3DFORMAT DSFmt;
                                        for (int iFmt = 0; iFmt < NumDSFormats; iFmt++)
                                        {
                                            DSFmt = DSFormatArray[iFmt];
                                            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_DEPTHSTENCIL,
                                                D3DRTYPE_SURFACE, DSFmt)))
                                            {
                                                if (SUCCEEDED(g_pD3D->CheckDepthStencilMatch(iAdapter, devType, fmtAdapter, fmtRender, DSFmt)))
                                                {
                                                    if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, DSFmt, bWindowed, msType, nullptr)))
                                                    {
                                                        (void)TVAddNodeEx(hTree10, FormatName(DSFmt), FALSE, IDI_CAPS, DXGCheckDSQualityLevels, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)DSFmt, (LPARAM)msType);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    TreeView_Expand(hwndTV, hTree, TVE_EXPAND);
}


//-----------------------------------------------------------------------------
// Name: DXG_CleanUp()
// Desc:
//-----------------------------------------------------------------------------
VOID DXG_CleanUp()
{
    SAFE_RELEASE(g_pD3D);

    if (g_hInstD3D)
    {
        g_direct3DCreate9 = nullptr;
        g_direct3DCreate9Ex = nullptr;

        FreeLibrary(g_hInstD3D);
        g_hInstD3D = nullptr;
    }
}


BOOL DXG_Is9Ex()
{
    return g_is9Ex;
}
