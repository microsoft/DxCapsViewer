//-----------------------------------------------------------------------------
// Name: DXG.cpp
//
// Desc: DX Graphics stuff for DirectX caps viewer
//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
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

typedef HRESULT (WINAPI *Direct3DCreate9ExFunc)(UINT SDKVersion, IDirect3D9**);


extern HRESULT PrintValueLine( const CHAR* szText, DWORD dwValue, PRINTCBINFO *lpInfo );
extern HRESULT PrintHexValueLine( const CHAR* szText, DWORD dwValue, PRINTCBINFO *lpInfo );
extern HRESULT PrintStringValueLine( const CHAR* szText, const CHAR* szText2, PRINTCBINFO *lpInfo );
extern HRESULT PrintStringLine( const CHAR* szText, PRINTCBINFO *lpInfo );

LPDIRECT3D9 g_pD3D = NULL; // Direct3D object
BOOL g_is9Ex = FALSE;

BOOL IsBBFmt(D3DFORMAT fmt);
BOOL IsDeviceTypeAvailable(UINT iAdapter, D3DDEVTYPE devType);
BOOL IsAdapterFmtAvailable(UINT iAdapter, D3DDEVTYPE devType, D3DFORMAT fmtAdapter, BOOL bWindowed);
HRESULT DXGDisplayModes( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo );
HRESULT DXGDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo );
HRESULT DXGDisplayMultiSample( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );
HRESULT DXGDisplayBackBuffer( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );
HRESULT DXGDisplayRenderTarget( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );
HRESULT DXGDisplayDepthStencil( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );
HRESULT DXGDisplayPlainSurface( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );
HRESULT DXGDisplayResource( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );
HRESULT DXGCheckDSQualityLevels( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pInfo );

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
int NumFormats = sizeof(AllFormatArray) / sizeof(AllFormatArray[0]);

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
int NumAdapterFormats = sizeof(AdapterFormatArray) / sizeof(AdapterFormatArray[0]);

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
int NumBBFormats = sizeof(BBFormatArray) / sizeof(BBFormatArray[0]);

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
int NumDSFormats = sizeof(DSFormatArray) / sizeof(DSFormatArray[0]);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF DXGGenCaps[] =
{
    CAPSVALDEF("DeviceType",                   DeviceType),
    CAPSVALDEF("AdapterOrdinal",               AdapterOrdinal),

    CAPSVALDEF("MaxTextureWidth",              MaxTextureWidth),
    CAPSVALDEF("MaxTextureHeight",             MaxTextureHeight),
    CAPSVALDEF("MaxVolumeExtent",              MaxVolumeExtent),

    CAPSVALDEF("MaxTextureRepeat",             MaxTextureRepeat),
    CAPSVALDEF("MaxTextureAspectRatio",        MaxTextureAspectRatio),
    CAPSVALDEF("MaxAnisotropy",                MaxAnisotropy),
    CAPSFLOATDEF("MaxVertexW",                 MaxVertexW),

    CAPSFLOATDEF("GuardBandLeft",              GuardBandLeft),
    CAPSFLOATDEF("GuardBandTop",               GuardBandTop),
    CAPSFLOATDEF("GuardBandRight",             GuardBandRight),
    CAPSFLOATDEF("GuardBandBottom",            GuardBandBottom),

    CAPSFLOATDEF("ExtentsAdjust",              ExtentsAdjust),

    CAPSVALDEF("MaxTextureBlendStages",        MaxTextureBlendStages),
    CAPSVALDEF("MaxSimultaneousTextures",      MaxSimultaneousTextures),

    CAPSVALDEF("MaxActiveLights",              MaxActiveLights),
    CAPSVALDEF("MaxUserClipPlanes",            MaxUserClipPlanes),
    CAPSVALDEF("MaxVertexBlendMatrices",       MaxVertexBlendMatrices),
    CAPSVALDEF("MaxVertexBlendMatrixIndex",    MaxVertexBlendMatrixIndex),

    CAPSFLOATDEF("MaxPointSize",               MaxPointSize),

    CAPSVALDEF("MaxPrimitiveCount",            MaxPrimitiveCount),
    CAPSVALDEF("MaxVertexIndex",               MaxVertexIndex),
    CAPSVALDEF("MaxStreams",                   MaxStreams),
    CAPSVALDEF("MaxStreamStride",              MaxStreamStride),

    CAPSSHADERDEF("VertexShaderVersion",       VertexShaderVersion),
    CAPSVALDEF("MaxVertexShaderConst",         MaxVertexShaderConst),

    CAPSSHADERDEF("PixelShaderVersion",        PixelShaderVersion),
    CAPSFLOATDEF("PixelShader1xMaxValue",      PixelShader1xMaxValue),

    CAPSFLOATDEF("MaxNpatchTessellationLevel", MaxNpatchTessellationLevel),

    CAPSVALDEF("MasterAdapterOrdinal",         MasterAdapterOrdinal),
    CAPSVALDEF("AdapterOrdinalInGroup",        AdapterOrdinalInGroup),
    CAPSVALDEF("NumberOfAdaptersInGroup",      NumberOfAdaptersInGroup),
    CAPSVALDEF("NumSimultaneousRTs",           NumSimultaneousRTs),
    CAPSVALDEF("MaxVShaderInstructionsExecuted", MaxVShaderInstructionsExecuted),
    CAPSVALDEF("MaxPShaderInstructionsExecuted", MaxPShaderInstructionsExecuted),
    CAPSVALDEF("MaxVertexShader30InstructionSlots", MaxVertexShader30InstructionSlots),
    CAPSVALDEF("MaxPixelShader30InstructionSlots", MaxPixelShader30InstructionSlots),
    
    { NULL, 0, 0 }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsCaps[] =
{
    CAPSFLAGDEF("D3DCAPS_READ_SCANLINE",        Caps,   D3DCAPS_READ_SCANLINE),
    CAPSFLAGDEFex("D3DCAPS_OVERLAY",            Caps,   D3DCAPS_OVERLAY),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsCaps2[] =
{
    CAPSFLAGDEF("D3DCAPS2_CANCALIBRATEGAMMA",   Caps2,  D3DCAPS2_CANCALIBRATEGAMMA),
    CAPSFLAGDEF("D3DCAPS2_FULLSCREENGAMMA",     Caps2,  D3DCAPS2_FULLSCREENGAMMA),
    CAPSFLAGDEF("D3DCAPS2_CANMANAGERESOURCE",   Caps2,  D3DCAPS2_CANMANAGERESOURCE),
    CAPSFLAGDEF("D3DCAPS2_DYNAMICTEXTURES",     Caps2,  D3DCAPS2_DYNAMICTEXTURES),
    CAPSFLAGDEF("D3DCAPS2_CANAUTOGENMIPMAP",    Caps2,  D3DCAPS2_CANAUTOGENMIPMAP),
    CAPSFLAGDEFex("D3DCAPS2_CANSHARERESOURCE",  Caps2,  D3DCAPS2_CANSHARERESOURCE),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsCaps3[] =
{
    CAPSFLAGDEF("D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD", Caps3,    D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD),
    CAPSFLAGDEF("D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION",      Caps3,    D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION),
    CAPSFLAGDEF("D3DCAPS3_COPY_TO_VIDMEM",                   Caps3,    D3DCAPS3_COPY_TO_VIDMEM),
    CAPSFLAGDEF("D3DCAPS3_COPY_TO_SYSTEMMEM",                Caps3,    D3DCAPS3_COPY_TO_SYSTEMMEM),
    CAPSFLAGDEFex("D3DCAPS3_DXVAHD",                         Caps3,    D3DCAPS3_DXVAHD),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsPresentationIntervals[] =
{
    CAPSFLAGDEF("D3DPRESENT_INTERVAL_ONE",   PresentationIntervals, D3DPRESENT_INTERVAL_ONE),
    CAPSFLAGDEF("D3DPRESENT_INTERVAL_TWO",   PresentationIntervals, D3DPRESENT_INTERVAL_TWO),
    CAPSFLAGDEF("D3DPRESENT_INTERVAL_THREE",   PresentationIntervals, D3DPRESENT_INTERVAL_THREE),
    CAPSFLAGDEF("D3DPRESENT_INTERVAL_FOUR",   PresentationIntervals, D3DPRESENT_INTERVAL_FOUR),
    CAPSFLAGDEF("D3DPRESENT_INTERVAL_IMMEDIATE",   PresentationIntervals, D3DPRESENT_INTERVAL_IMMEDIATE),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsCursorCaps[] =
{
    CAPSFLAGDEF("D3DCURSORCAPS_COLOR",   CursorCaps,    D3DCURSORCAPS_COLOR),
    CAPSFLAGDEF("D3DCURSORCAPS_LOWRES",  CursorCaps,    D3DCURSORCAPS_LOWRES),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsDevCaps[] =
{
    CAPSFLAGDEF("D3DDEVCAPS_EXECUTESYSTEMMEMORY",     DevCaps,    D3DDEVCAPS_EXECUTESYSTEMMEMORY),
    CAPSFLAGDEF("D3DDEVCAPS_EXECUTEVIDEOMEMORY",      DevCaps,    D3DDEVCAPS_EXECUTEVIDEOMEMORY),
    CAPSFLAGDEF("D3DDEVCAPS_TLVERTEXSYSTEMMEMORY",    DevCaps,    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY),
    CAPSFLAGDEF("D3DDEVCAPS_TLVERTEXVIDEOMEMORY",     DevCaps,    D3DDEVCAPS_TLVERTEXVIDEOMEMORY),
    CAPSFLAGDEF("D3DDEVCAPS_TEXTURESYSTEMMEMORY",     DevCaps,    D3DDEVCAPS_TEXTURESYSTEMMEMORY),
    CAPSFLAGDEF("D3DDEVCAPS_TEXTUREVIDEOMEMORY",      DevCaps,    D3DDEVCAPS_TEXTUREVIDEOMEMORY),
    CAPSFLAGDEF("D3DDEVCAPS_DRAWPRIMTLVERTEX",        DevCaps,    D3DDEVCAPS_DRAWPRIMTLVERTEX),
    CAPSFLAGDEF("D3DDEVCAPS_CANRENDERAFTERFLIP",      DevCaps,    D3DDEVCAPS_CANRENDERAFTERFLIP),
    CAPSFLAGDEF("D3DDEVCAPS_TEXTURENONLOCALVIDMEM",   DevCaps,    D3DDEVCAPS_TEXTURENONLOCALVIDMEM),
    CAPSFLAGDEF("D3DDEVCAPS_DRAWPRIMITIVES2",         DevCaps,    D3DDEVCAPS_DRAWPRIMITIVES2),
    CAPSFLAGDEF("D3DDEVCAPS_SEPARATETEXTUREMEMORIES", DevCaps,    D3DDEVCAPS_SEPARATETEXTUREMEMORIES),
    CAPSFLAGDEF("D3DDEVCAPS_DRAWPRIMITIVES2EX",       DevCaps,    D3DDEVCAPS_DRAWPRIMITIVES2EX),
    CAPSFLAGDEF("D3DDEVCAPS_HWTRANSFORMANDLIGHT",     DevCaps,    D3DDEVCAPS_HWTRANSFORMANDLIGHT),
    CAPSFLAGDEF("D3DDEVCAPS_CANBLTSYSTONONLOCAL",     DevCaps,    D3DDEVCAPS_CANBLTSYSTONONLOCAL),
    CAPSFLAGDEF("D3DDEVCAPS_HWRASTERIZATION",         DevCaps,    D3DDEVCAPS_HWRASTERIZATION),
    CAPSFLAGDEF("D3DDEVCAPS_PUREDEVICE",              DevCaps,    D3DDEVCAPS_PUREDEVICE),
    CAPSFLAGDEF("D3DDEVCAPS_QUINTICRTPATCHES",        DevCaps,    D3DDEVCAPS_QUINTICRTPATCHES),
    CAPSFLAGDEF("D3DDEVCAPS_RTPATCHES",               DevCaps,    D3DDEVCAPS_RTPATCHES),
    CAPSFLAGDEF("D3DDEVCAPS_RTPATCHHANDLEZERO",       DevCaps,    D3DDEVCAPS_RTPATCHHANDLEZERO),
    CAPSFLAGDEF("D3DDEVCAPS_NPATCHES",                DevCaps,    D3DDEVCAPS_NPATCHES),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsLineCaps[] =
{
    CAPSFLAGDEF("D3DLINECAPS_TEXTURE",           LineCaps,    D3DLINECAPS_TEXTURE ),
    CAPSFLAGDEF("D3DLINECAPS_ZTEST",             LineCaps,    D3DLINECAPS_ZTEST   ),
    CAPSFLAGDEF("D3DLINECAPS_BLEND",             LineCaps,    D3DLINECAPS_BLEND   ),
    CAPSFLAGDEF("D3DLINECAPS_ALPHACMP",          LineCaps,    D3DLINECAPS_ALPHACMP),
    CAPSFLAGDEF("D3DLINECAPS_FOG",               LineCaps,    D3DLINECAPS_FOG     ),
    CAPSFLAGDEF("D3DLINECAPS_ANTIALIAS",         LineCaps,    D3DLINECAPS_ANTIALIAS),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsStencilCaps[] =
{
    CAPSFLAGDEF("D3DSTENCILCAPS_KEEP",           StencilCaps,    D3DSTENCILCAPS_KEEP),
    CAPSFLAGDEF("D3DSTENCILCAPS_ZERO",           StencilCaps,    D3DSTENCILCAPS_ZERO),
    CAPSFLAGDEF("D3DSTENCILCAPS_REPLACE",        StencilCaps,    D3DSTENCILCAPS_REPLACE),
    CAPSFLAGDEF("D3DSTENCILCAPS_INCRSAT",        StencilCaps,    D3DSTENCILCAPS_INCRSAT),
    CAPSFLAGDEF("D3DSTENCILCAPS_DECRSAT",        StencilCaps,    D3DSTENCILCAPS_DECRSAT),
    CAPSFLAGDEF("D3DSTENCILCAPS_INVERT",         StencilCaps,    D3DSTENCILCAPS_INVERT),
    CAPSFLAGDEF("D3DSTENCILCAPS_INCR",           StencilCaps,    D3DSTENCILCAPS_INCR),
    CAPSFLAGDEF("D3DSTENCILCAPS_DECR",           StencilCaps,    D3DSTENCILCAPS_DECR),
    CAPSFLAGDEF("D3DSTENCILCAPS_TWOSIDED",       StencilCaps,    D3DSTENCILCAPS_TWOSIDED),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsFVFCaps[] =
{
    CAPSFLAGDEF("D3DFVFCAPS_DONOTSTRIPELEMENTS", FVFCaps,    D3DFVFCAPS_DONOTSTRIPELEMENTS),
    CAPSMASK16DEF("D3DFVFCAPS_TEXCOORDCOUNTMASK", FVFCaps),
    CAPSFLAGDEF("D3DFVFCAPS_PSIZE",              FVFCaps,    D3DFVFCAPS_PSIZE),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsTextureOpCaps[] =
{
    CAPSFLAGDEF("D3DTEXOPCAPS_DISABLE",                     TextureOpCaps, D3DTEXOPCAPS_DISABLE                       ),
    CAPSFLAGDEF("D3DTEXOPCAPS_SELECTARG1",                  TextureOpCaps, D3DTEXOPCAPS_SELECTARG1                    ),
    CAPSFLAGDEF("D3DTEXOPCAPS_SELECTARG2",                  TextureOpCaps, D3DTEXOPCAPS_SELECTARG2                    ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATE",                    TextureOpCaps, D3DTEXOPCAPS_MODULATE                      ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATE2X",                  TextureOpCaps, D3DTEXOPCAPS_MODULATE2X                    ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATE4X",                  TextureOpCaps, D3DTEXOPCAPS_MODULATE4X                    ),
    CAPSFLAGDEF("D3DTEXOPCAPS_ADD",                         TextureOpCaps, D3DTEXOPCAPS_ADD                           ),
    CAPSFLAGDEF("D3DTEXOPCAPS_ADDSIGNED",                   TextureOpCaps, D3DTEXOPCAPS_ADDSIGNED                     ),
    CAPSFLAGDEF("D3DTEXOPCAPS_ADDSIGNED2X",                 TextureOpCaps, D3DTEXOPCAPS_ADDSIGNED2X                   ),
    CAPSFLAGDEF("D3DTEXOPCAPS_SUBTRACT",                    TextureOpCaps, D3DTEXOPCAPS_SUBTRACT                      ),
    CAPSFLAGDEF("D3DTEXOPCAPS_ADDSMOOTH",                   TextureOpCaps, D3DTEXOPCAPS_ADDSMOOTH                     ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BLENDDIFFUSEALPHA",           TextureOpCaps, D3DTEXOPCAPS_BLENDDIFFUSEALPHA             ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BLENDTEXTUREALPHA",           TextureOpCaps, D3DTEXOPCAPS_BLENDTEXTUREALPHA             ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BLENDFACTORALPHA",            TextureOpCaps, D3DTEXOPCAPS_BLENDFACTORALPHA              ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BLENDTEXTUREALPHAPM",         TextureOpCaps, D3DTEXOPCAPS_BLENDTEXTUREALPHAPM           ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BLENDCURRENTALPHA",           TextureOpCaps, D3DTEXOPCAPS_BLENDCURRENTALPHA             ),
    CAPSFLAGDEF("D3DTEXOPCAPS_PREMODULATE",                 TextureOpCaps, D3DTEXOPCAPS_PREMODULATE                   ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR",      TextureOpCaps, D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR        ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA",      TextureOpCaps, D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA        ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR",   TextureOpCaps, D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR     ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA",   TextureOpCaps, D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA     ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BUMPENVMAP",                  TextureOpCaps, D3DTEXOPCAPS_BUMPENVMAP                    ),
    CAPSFLAGDEF("D3DTEXOPCAPS_BUMPENVMAPLUMINANCE",         TextureOpCaps, D3DTEXOPCAPS_BUMPENVMAPLUMINANCE           ),
    CAPSFLAGDEF("D3DTEXOPCAPS_DOTPRODUCT3",                 TextureOpCaps, D3DTEXOPCAPS_DOTPRODUCT3                   ),
    CAPSFLAGDEF("D3DTEXOPCAPS_MULTIPLYADD",                 TextureOpCaps, D3DTEXOPCAPS_MULTIPLYADD                   ),
    CAPSFLAGDEF("D3DTEXOPCAPS_LERP",                        TextureOpCaps, D3DTEXOPCAPS_LERP                   ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsVertexProcessingCaps[] =
{
    CAPSFLAGDEF("D3DVTXPCAPS_DIRECTIONALLIGHTS", VertexProcessingCaps, D3DVTXPCAPS_DIRECTIONALLIGHTS),
    CAPSFLAGDEF("D3DVTXPCAPS_LOCALVIEWER",       VertexProcessingCaps, D3DVTXPCAPS_LOCALVIEWER),
    CAPSFLAGDEF("D3DVTXPCAPS_MATERIALSOURCE7",   VertexProcessingCaps, D3DVTXPCAPS_MATERIALSOURCE7),
    CAPSFLAGDEF("D3DVTXPCAPS_POSITIONALLIGHTS",  VertexProcessingCaps, D3DVTXPCAPS_POSITIONALLIGHTS),
    CAPSFLAGDEF("D3DVTXPCAPS_TEXGEN",            VertexProcessingCaps, D3DVTXPCAPS_TEXGEN),
    CAPSFLAGDEF("D3DVTXPCAPS_TWEENING",          VertexProcessingCaps, D3DVTXPCAPS_TWEENING),
    CAPSFLAGDEF("D3DVTXPCAPS_TEXGEN_SPHEREMAP",  VertexProcessingCaps, D3DVTXPCAPS_TEXGEN_SPHEREMAP),
    CAPSFLAGDEF("D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER",  VertexProcessingCaps, D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF PrimCaps[] =
{
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsPrimMiscCaps[] =
{
    CAPSFLAGDEF("D3DPMISCCAPS_MASKZ",                      PrimitiveMiscCaps,  D3DPMISCCAPS_MASKZ),
    CAPSFLAGDEF("D3DPMISCCAPS_CULLNONE",                   PrimitiveMiscCaps,  D3DPMISCCAPS_CULLNONE),
    CAPSFLAGDEF("D3DPMISCCAPS_CULLCW",                     PrimitiveMiscCaps,  D3DPMISCCAPS_CULLCW),
    CAPSFLAGDEF("D3DPMISCCAPS_CULLCCW",                    PrimitiveMiscCaps,  D3DPMISCCAPS_CULLCCW),
    CAPSFLAGDEF("D3DPMISCCAPS_COLORWRITEENABLE",           PrimitiveMiscCaps,  D3DPMISCCAPS_COLORWRITEENABLE),
    CAPSFLAGDEF("D3DPMISCCAPS_CLIPPLANESCALEDPOINTS",      PrimitiveMiscCaps,  D3DPMISCCAPS_CLIPPLANESCALEDPOINTS),
    CAPSFLAGDEF("D3DPMISCCAPS_CLIPTLVERTS",                PrimitiveMiscCaps,  D3DPMISCCAPS_CLIPTLVERTS),
    CAPSFLAGDEF("D3DPMISCCAPS_TSSARGTEMP",                 PrimitiveMiscCaps,  D3DPMISCCAPS_TSSARGTEMP),
    CAPSFLAGDEF("D3DPMISCCAPS_BLENDOP",                    PrimitiveMiscCaps,  D3DPMISCCAPS_BLENDOP),
    CAPSFLAGDEF("D3DPMISCCAPS_NULLREFERENCE",              PrimitiveMiscCaps,  D3DPMISCCAPS_NULLREFERENCE),
    CAPSFLAGDEF("D3DPMISCCAPS_INDEPENDENTWRITEMASKS",      PrimitiveMiscCaps,  D3DPMISCCAPS_INDEPENDENTWRITEMASKS),
    CAPSFLAGDEF("D3DPMISCCAPS_PERSTAGECONSTANT",           PrimitiveMiscCaps,  D3DPMISCCAPS_PERSTAGECONSTANT),
    CAPSFLAGDEF("D3DPMISCCAPS_FOGANDSPECULARALPHA",        PrimitiveMiscCaps,  D3DPMISCCAPS_FOGANDSPECULARALPHA),
    CAPSFLAGDEF("D3DPMISCCAPS_SEPARATEALPHABLEND",         PrimitiveMiscCaps,  D3DPMISCCAPS_SEPARATEALPHABLEND),
    CAPSFLAGDEF("D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS",    PrimitiveMiscCaps,  D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS),
    CAPSFLAGDEF("D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING", PrimitiveMiscCaps,  D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING),
    CAPSFLAGDEF("D3DPMISCCAPS_FOGVERTEXCLAMPED",           PrimitiveMiscCaps,  D3DPMISCCAPS_FOGVERTEXCLAMPED),
    CAPSFLAGDEFex("D3DPMISCCAPS_POSTBLENDSRGBCONVERT",     PrimitiveMiscCaps,  D3DPMISCCAPS_POSTBLENDSRGBCONVERT),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsRasterCaps[] =
{
    CAPSFLAGDEF("D3DPRASTERCAPS_DITHER",            RasterCaps,   D3DPRASTERCAPS_DITHER),
    CAPSFLAGDEF("D3DPRASTERCAPS_ZTEST",             RasterCaps,   D3DPRASTERCAPS_ZTEST),
    CAPSFLAGDEF("D3DPRASTERCAPS_FOGVERTEX",         RasterCaps,   D3DPRASTERCAPS_FOGVERTEX),
    CAPSFLAGDEF("D3DPRASTERCAPS_FOGTABLE",          RasterCaps,   D3DPRASTERCAPS_FOGTABLE),
//    CAPSFLAGDEF("D3DPRASTERCAPS_ANTIALIASEDGES",    RasterCaps,   D3DPRASTERCAPS_ANTIALIASEDGES),
    CAPSFLAGDEF("D3DPRASTERCAPS_MIPMAPLODBIAS",     RasterCaps,   D3DPRASTERCAPS_MIPMAPLODBIAS),
//    CAPSFLAGDEF("D3DPRASTERCAPS_ZBIAS",             RasterCaps,   D3DPRASTERCAPS_ZBIAS),
    CAPSFLAGDEF("D3DPRASTERCAPS_ZBUFFERLESSHSR",    RasterCaps,   D3DPRASTERCAPS_ZBUFFERLESSHSR),
    CAPSFLAGDEF("D3DPRASTERCAPS_FOGRANGE",          RasterCaps,   D3DPRASTERCAPS_FOGRANGE),
    CAPSFLAGDEF("D3DPRASTERCAPS_ANISOTROPY",        RasterCaps,   D3DPRASTERCAPS_ANISOTROPY),
    CAPSFLAGDEF("D3DPRASTERCAPS_WBUFFER",           RasterCaps,   D3DPRASTERCAPS_WBUFFER),
    CAPSFLAGDEF("D3DPRASTERCAPS_WFOG",              RasterCaps,   D3DPRASTERCAPS_WFOG),
    CAPSFLAGDEF("D3DPRASTERCAPS_ZFOG",              RasterCaps,   D3DPRASTERCAPS_ZFOG),
    CAPSFLAGDEF("D3DPRASTERCAPS_COLORPERSPECTIVE",  RasterCaps,   D3DPRASTERCAPS_COLORPERSPECTIVE),
//    CAPSFLAGDEF("D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE",   RasterCaps,   D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE),
    CAPSFLAGDEF("D3DPRASTERCAPS_SCISSORTEST",       RasterCaps,   D3DPRASTERCAPS_SCISSORTEST),
    CAPSFLAGDEF("D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS", RasterCaps,   D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS),
    CAPSFLAGDEF("D3DPRASTERCAPS_DEPTHBIAS",         RasterCaps,   D3DPRASTERCAPS_DEPTHBIAS),
    CAPSFLAGDEF("D3DPRASTERCAPS_MULTISAMPLE_TOGGLE",RasterCaps,   D3DPRASTERCAPS_MULTISAMPLE_TOGGLE),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsZCmpCaps[] =
{
    CAPSFLAGDEF("D3DPCMPCAPS_NEVER",         ZCmpCaps,  D3DPCMPCAPS_NEVER        ),
    CAPSFLAGDEF("D3DPCMPCAPS_LESS",          ZCmpCaps,  D3DPCMPCAPS_LESS         ),
    CAPSFLAGDEF("D3DPCMPCAPS_EQUAL",         ZCmpCaps,  D3DPCMPCAPS_EQUAL        ),
    CAPSFLAGDEF("D3DPCMPCAPS_LESSEQUAL",     ZCmpCaps,  D3DPCMPCAPS_LESSEQUAL    ),
    CAPSFLAGDEF("D3DPCMPCAPS_GREATER",       ZCmpCaps,  D3DPCMPCAPS_GREATER      ),
    CAPSFLAGDEF("D3DPCMPCAPS_NOTEQUAL",      ZCmpCaps,  D3DPCMPCAPS_NOTEQUAL     ),
    CAPSFLAGDEF("D3DPCMPCAPS_GREATEREQUAL",  ZCmpCaps,  D3DPCMPCAPS_GREATEREQUAL ),
    CAPSFLAGDEF("D3DPCMPCAPS_ALWAYS",        ZCmpCaps,  D3DPCMPCAPS_ALWAYS       ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsSrcBlendCaps[] =
{
    CAPSFLAGDEF("D3DPBLENDCAPS_ZERO",            SrcBlendCaps,  D3DPBLENDCAPS_ZERO            ),
    CAPSFLAGDEF("D3DPBLENDCAPS_ONE",             SrcBlendCaps,  D3DPBLENDCAPS_ONE             ),
    CAPSFLAGDEF("D3DPBLENDCAPS_SRCCOLOR",        SrcBlendCaps,  D3DPBLENDCAPS_SRCCOLOR        ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVSRCCOLOR",     SrcBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR     ),
    CAPSFLAGDEF("D3DPBLENDCAPS_SRCALPHA",        SrcBlendCaps,  D3DPBLENDCAPS_SRCALPHA        ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVSRCALPHA",     SrcBlendCaps,  D3DPBLENDCAPS_INVSRCALPHA     ),
    CAPSFLAGDEF("D3DPBLENDCAPS_DESTALPHA",       SrcBlendCaps,  D3DPBLENDCAPS_DESTALPHA       ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVDESTALPHA",    SrcBlendCaps,  D3DPBLENDCAPS_INVDESTALPHA    ),
    CAPSFLAGDEF("D3DPBLENDCAPS_DESTCOLOR",       SrcBlendCaps,  D3DPBLENDCAPS_DESTCOLOR       ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVDESTCOLOR",    SrcBlendCaps,  D3DPBLENDCAPS_INVDESTCOLOR    ),
    CAPSFLAGDEF("D3DPBLENDCAPS_SRCALPHASAT",     SrcBlendCaps,  D3DPBLENDCAPS_SRCALPHASAT     ),
    CAPSFLAGDEF("D3DPBLENDCAPS_BOTHSRCALPHA",    SrcBlendCaps,  D3DPBLENDCAPS_BOTHSRCALPHA    ),
    CAPSFLAGDEF("D3DPBLENDCAPS_BOTHINVSRCALPHA", SrcBlendCaps,  D3DPBLENDCAPS_BOTHINVSRCALPHA ),
    CAPSFLAGDEF("D3DPBLENDCAPS_BLENDFACTOR",     SrcBlendCaps,  D3DPBLENDCAPS_BLENDFACTOR ),
    CAPSFLAGDEFex("D3DPBLENDCAPS_SRCCOLOR2",	 SrcBlendCaps,  D3DPBLENDCAPS_SRCCOLOR2       ),
    CAPSFLAGDEFex("D3DPBLENDCAPS_INVSRCCOLOR2",  SrcBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR2    ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsDestBlendCaps[] =
{
    CAPSFLAGDEF("D3DPBLENDCAPS_ZERO",            DestBlendCaps,  D3DPBLENDCAPS_ZERO            ),
    CAPSFLAGDEF("D3DPBLENDCAPS_ONE",             DestBlendCaps,  D3DPBLENDCAPS_ONE             ),
    CAPSFLAGDEF("D3DPBLENDCAPS_SRCCOLOR",        DestBlendCaps,  D3DPBLENDCAPS_SRCCOLOR        ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVSRCCOLOR",     DestBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR     ),
    CAPSFLAGDEF("D3DPBLENDCAPS_SRCALPHA",        DestBlendCaps,  D3DPBLENDCAPS_SRCALPHA        ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVSRCALPHA",     DestBlendCaps,  D3DPBLENDCAPS_INVSRCALPHA     ),
    CAPSFLAGDEF("D3DPBLENDCAPS_DESTALPHA",       DestBlendCaps,  D3DPBLENDCAPS_DESTALPHA       ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVDESTALPHA",    DestBlendCaps,  D3DPBLENDCAPS_INVDESTALPHA    ),
    CAPSFLAGDEF("D3DPBLENDCAPS_DESTCOLOR",       DestBlendCaps,  D3DPBLENDCAPS_DESTCOLOR       ),
    CAPSFLAGDEF("D3DPBLENDCAPS_INVDESTCOLOR",    DestBlendCaps,  D3DPBLENDCAPS_INVDESTCOLOR    ),
    CAPSFLAGDEF("D3DPBLENDCAPS_SRCALPHASAT",     DestBlendCaps,  D3DPBLENDCAPS_SRCALPHASAT     ),
    CAPSFLAGDEF("D3DPBLENDCAPS_BOTHSRCALPHA",    DestBlendCaps,  D3DPBLENDCAPS_BOTHSRCALPHA    ),
    CAPSFLAGDEF("D3DPBLENDCAPS_BOTHINVSRCALPHA", DestBlendCaps,  D3DPBLENDCAPS_BOTHINVSRCALPHA ),
    CAPSFLAGDEF("D3DPBLENDCAPS_BLENDFACTOR",     DestBlendCaps,  D3DPBLENDCAPS_BLENDFACTOR ),
    CAPSFLAGDEFex("D3DPBLENDCAPS_SRCCOLOR2",	 DestBlendCaps,  D3DPBLENDCAPS_SRCCOLOR2        ),
    CAPSFLAGDEFex("D3DPBLENDCAPS_INVSRCCOLOR2",  DestBlendCaps,  D3DPBLENDCAPS_INVSRCCOLOR2     ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsAlphaCmpCaps[] =
{
    CAPSFLAGDEF("D3DPCMPCAPS_NEVER",             AlphaCmpCaps,   D3DPCMPCAPS_NEVER                 ),
    CAPSFLAGDEF("D3DPCMPCAPS_LESS",              AlphaCmpCaps,   D3DPCMPCAPS_LESS                  ),
    CAPSFLAGDEF("D3DPCMPCAPS_EQUAL",             AlphaCmpCaps,   D3DPCMPCAPS_EQUAL                 ),
    CAPSFLAGDEF("D3DPCMPCAPS_LESSEQUAL",         AlphaCmpCaps,   D3DPCMPCAPS_LESSEQUAL             ),
    CAPSFLAGDEF("D3DPCMPCAPS_GREATER",           AlphaCmpCaps,   D3DPCMPCAPS_GREATER               ),
    CAPSFLAGDEF("D3DPCMPCAPS_NOTEQUAL",          AlphaCmpCaps,   D3DPCMPCAPS_NOTEQUAL              ),
    CAPSFLAGDEF("D3DPCMPCAPS_GREATEREQUAL",      AlphaCmpCaps,   D3DPCMPCAPS_GREATEREQUAL          ),
    CAPSFLAGDEF("D3DPCMPCAPS_ALWAYS",            AlphaCmpCaps,   D3DPCMPCAPS_ALWAYS                ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsShadeCaps[] =
{
    CAPSFLAGDEF("D3DPSHADECAPS_COLORGOURAUDRGB",      ShadeCaps,  D3DPSHADECAPS_COLORGOURAUDRGB     ),
    CAPSFLAGDEF("D3DPSHADECAPS_SPECULARGOURAUDRGB",   ShadeCaps,  D3DPSHADECAPS_SPECULARGOURAUDRGB  ),
    CAPSFLAGDEF("D3DPSHADECAPS_ALPHAGOURAUDBLEND",    ShadeCaps,  D3DPSHADECAPS_ALPHAGOURAUDBLEND   ),
    CAPSFLAGDEF("D3DPSHADECAPS_FOGGOURAUD",           ShadeCaps,  D3DPSHADECAPS_FOGGOURAUD          ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsTextureCaps[] =
{
    CAPSFLAGDEF("D3DPTEXTURECAPS_PERSPECTIVE",        TextureCaps,        D3DPTEXTURECAPS_PERSPECTIVE       ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_POW2",               TextureCaps,        D3DPTEXTURECAPS_POW2              ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_ALPHA",              TextureCaps,        D3DPTEXTURECAPS_ALPHA             ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_SQUAREONLY",         TextureCaps,        D3DPTEXTURECAPS_SQUAREONLY        ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE", TextureCaps,  D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE),
    CAPSFLAGDEF("D3DPTEXTURECAPS_ALPHAPALETTE",       TextureCaps,        D3DPTEXTURECAPS_ALPHAPALETTE      ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_NONPOW2CONDITIONAL", TextureCaps,        D3DPTEXTURECAPS_NONPOW2CONDITIONAL ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_PROJECTED",          TextureCaps,        D3DPTEXTURECAPS_PROJECTED ),
    CAPSFLAGDEF("D3DPTEXTURECAPS_CUBEMAP",            TextureCaps,        D3DPTEXTURECAPS_CUBEMAP),
    CAPSFLAGDEF("D3DPTEXTURECAPS_VOLUMEMAP",          TextureCaps,        D3DPTEXTURECAPS_VOLUMEMAP),
    CAPSFLAGDEF("D3DPTEXTURECAPS_MIPMAP",             TextureCaps,        D3DPTEXTURECAPS_MIPMAP),
    CAPSFLAGDEF("D3DPTEXTURECAPS_MIPVOLUMEMAP",       TextureCaps,        D3DPTEXTURECAPS_MIPVOLUMEMAP),
    CAPSFLAGDEF("D3DPTEXTURECAPS_MIPCUBEMAP",         TextureCaps,        D3DPTEXTURECAPS_MIPCUBEMAP),
    CAPSFLAGDEF("D3DPTEXTURECAPS_CUBEMAP_POW2",       TextureCaps,        D3DPTEXTURECAPS_CUBEMAP_POW2),
    CAPSFLAGDEF("D3DPTEXTURECAPS_VOLUMEMAP_POW2",     TextureCaps,        D3DPTEXTURECAPS_VOLUMEMAP_POW2),
    CAPSFLAGDEF("D3DPTEXTURECAPS_NOPROJECTEDBUMPENV", TextureCaps,        D3DPTEXTURECAPS_NOPROJECTEDBUMPENV),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsTextureFilterCaps[] =
{
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPOINT",         TextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFLINEAR",        TextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFANISOTROPIC",   TextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", TextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  TextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFPOINT",         TextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFLINEAR",        TextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPOINT",         TextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFLINEAR",        TextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFANISOTROPIC",   TextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", TextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD    ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  TextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD ),
    CAPSFLAGDEFex("D3DPTFILTERCAPS_CONVOLUTIONMONO", TextureFilterCaps,  D3DPTFILTERCAPS_CONVOLUTIONMONO ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsCubeTextureFilterCaps[] =
{
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPOINT",         CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFLINEAR",        CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFANISOTROPIC",   CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  CubeTextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFPOINT",         CubeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFLINEAR",        CubeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPOINT",         CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFLINEAR",        CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFANISOTROPIC",   CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD    ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  CubeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsVolumeTextureFilterCaps[] =
{
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPOINT",         VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFLINEAR",        VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFANISOTROPIC",   VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFPOINT",         VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFLINEAR",        VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPOINT",         VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFLINEAR",        VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFANISOTROPIC",   VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD    ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  VolumeTextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsStretchRectFilterCaps[] =
{
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPOINT",         StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFLINEAR",        StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFANISOTROPIC",   StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  StretchRectFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFPOINT",         StretchRectFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFLINEAR",        StretchRectFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPOINT",         StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFLINEAR",        StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFANISOTROPIC",   StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD    ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  StretchRectFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsTextureAddressCaps[] =
{
    CAPSFLAGDEF("D3DPTADDRESSCAPS_WRAP",              TextureAddressCaps, D3DPTADDRESSCAPS_WRAP             ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_MIRROR",            TextureAddressCaps, D3DPTADDRESSCAPS_MIRROR           ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_CLAMP",             TextureAddressCaps, D3DPTADDRESSCAPS_CLAMP            ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_BORDER",            TextureAddressCaps, D3DPTADDRESSCAPS_BORDER           ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_INDEPENDENTUV",     TextureAddressCaps, D3DPTADDRESSCAPS_INDEPENDENTUV    ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_MIRRORONCE",        TextureAddressCaps, D3DPTADDRESSCAPS_MIRRORONCE    ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsVolumeTextureAddressCaps[] =
{
    CAPSFLAGDEF("D3DPTADDRESSCAPS_WRAP",              VolumeTextureAddressCaps, D3DPTADDRESSCAPS_WRAP             ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_MIRROR",            VolumeTextureAddressCaps, D3DPTADDRESSCAPS_MIRROR           ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_CLAMP",             VolumeTextureAddressCaps, D3DPTADDRESSCAPS_CLAMP            ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_BORDER",            VolumeTextureAddressCaps, D3DPTADDRESSCAPS_BORDER           ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_INDEPENDENTUV",     VolumeTextureAddressCaps, D3DPTADDRESSCAPS_INDEPENDENTUV    ),
    CAPSFLAGDEF("D3DPTADDRESSCAPS_MIRRORONCE",        VolumeTextureAddressCaps, D3DPTADDRESSCAPS_MIRRORONCE    ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsDevCaps2[] =
{
    CAPSFLAGDEF("D3DDEVCAPS2_STREAMOFFSET",           DevCaps2, D3DDEVCAPS2_STREAMOFFSET             ),
    CAPSFLAGDEF("D3DDEVCAPS2_DMAPNPATCH",             DevCaps2, D3DDEVCAPS2_DMAPNPATCH           ),
    CAPSFLAGDEF("D3DDEVCAPS2_ADAPTIVETESSRTPATCH",    DevCaps2, D3DDEVCAPS2_ADAPTIVETESSRTPATCH            ),
    CAPSFLAGDEF("D3DDEVCAPS2_ADAPTIVETESSNPATCH",     DevCaps2, D3DDEVCAPS2_ADAPTIVETESSNPATCH           ),
    CAPSFLAGDEF("D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES", DevCaps2, D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES           ),
    CAPSFLAGDEF("D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH",   DevCaps2, D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH           ),
    CAPSFLAGDEF("D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET",   DevCaps2, D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET           ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsDeclTypes[] =
{
    CAPSFLAGDEF("D3DDTCAPS_UBYTE4",                   DeclTypes, D3DDTCAPS_UBYTE4       ),
    CAPSFLAGDEF("D3DDTCAPS_UBYTE4N",                  DeclTypes, D3DDTCAPS_UBYTE4N      ),
    CAPSFLAGDEF("D3DDTCAPS_SHORT2N",                  DeclTypes, D3DDTCAPS_SHORT2N      ),
    CAPSFLAGDEF("D3DDTCAPS_SHORT4N",                  DeclTypes, D3DDTCAPS_SHORT4N      ),
    CAPSFLAGDEF("D3DDTCAPS_USHORT2N",                 DeclTypes, D3DDTCAPS_USHORT2N     ),
    CAPSFLAGDEF("D3DDTCAPS_USHORT4N",                 DeclTypes, D3DDTCAPS_USHORT4N     ),
    CAPSFLAGDEF("D3DDTCAPS_UDEC3",                    DeclTypes, D3DDTCAPS_UDEC3        ),
    CAPSFLAGDEF("D3DDTCAPS_DEC3N",                    DeclTypes, D3DDTCAPS_DEC3N        ),
    CAPSFLAGDEF("D3DDTCAPS_FLOAT16_2",                DeclTypes, D3DDTCAPS_FLOAT16_2    ),
    CAPSFLAGDEF("D3DDTCAPS_FLOAT16_4",                DeclTypes, D3DDTCAPS_FLOAT16_4    ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsVS20Caps[] =
{
    CAPSFLAGDEF("D3DVS20CAPS_PREDICATION",            VS20Caps.Caps, D3DVS20CAPS_PREDICATION ),
    CAPSVALDEF("DynamicFlowControlDepth",             VS20Caps.DynamicFlowControlDepth       ),
    CAPSVALDEF("NumTemps",                            VS20Caps.NumTemps                      ),
    CAPSVALDEF("StaticFlowControlDepth",              VS20Caps.StaticFlowControlDepth        ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsPS20Caps[] =
{
    CAPSFLAGDEF("D3DPS20CAPS_ARBITRARYSWIZZLE",       PS20Caps.Caps, D3DPS20CAPS_ARBITRARYSWIZZLE      ),
    CAPSFLAGDEF("D3DPS20CAPS_GRADIENTINSTRUCTIONS",   PS20Caps.Caps, D3DPS20CAPS_GRADIENTINSTRUCTIONS  ),
    CAPSFLAGDEF("D3DPS20CAPS_PREDICATION",            PS20Caps.Caps, D3DPS20CAPS_PREDICATION           ),
    CAPSFLAGDEF("D3DPS20CAPS_NODEPENDENTREADLIMIT",   PS20Caps.Caps, D3DPS20CAPS_NODEPENDENTREADLIMIT  ),
    CAPSFLAGDEF("D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT",  PS20Caps.Caps, D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT ),
    CAPSVALDEF("DynamicFlowControlDepth",             PS20Caps.DynamicFlowControlDepth                 ),
    CAPSVALDEF("NumTemps",                            PS20Caps.NumTemps                                ),
    CAPSVALDEF("StaticFlowControlDepth",              PS20Caps.StaticFlowControlDepth                  ),
    CAPSVALDEF("NumInstructionSlots",                 PS20Caps.NumInstructionSlots                     ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF CapsVertexTextureFilterCaps[] =
{
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPOINT",         VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFLINEAR",        VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFANISOTROPIC",   VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFPYRAMIDALQUAD", VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFPYRAMIDALQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MINFGAUSSIANQUAD",  VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFGAUSSIANQUAD   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFPOINT",         VertexTextureFilterCaps,  D3DPTFILTERCAPS_MIPFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MIPFLINEAR",        VertexTextureFilterCaps,  D3DPTFILTERCAPS_MIPFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPOINT",         VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPOINT         ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFLINEAR",        VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFLINEAR        ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFANISOTROPIC",   VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFANISOTROPIC   ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD", VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD    ),
    CAPSFLAGDEF("D3DPTFILTERCAPS_MAGFGAUSSIANQUAD",  VertexTextureFilterCaps,  D3DPTFILTERCAPS_MAGFGAUSSIANQUAD ),
    {"",0,0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEFS DXGCapDefs[] =
{
    {"",                        NULL,            (LPARAM)0,                },
    {"+Caps",                   DXGDisplayCaps,  (LPARAM)DXGGenCaps,       },
      {"Caps",                  DXGDisplayCaps,  (LPARAM)CapsCaps,        },
      {"Caps2",                 DXGDisplayCaps,  (LPARAM)CapsCaps2,       },
      {"Caps3",                 DXGDisplayCaps,  (LPARAM)CapsCaps3,       },
      {"PresentationIntervals", DXGDisplayCaps,  (LPARAM)CapsPresentationIntervals, },
      {"CursorCaps",            DXGDisplayCaps,  (LPARAM)CapsCursorCaps,  },
      {"DevCaps",               DXGDisplayCaps,  (LPARAM)CapsDevCaps,     },
      {"PrimitiveMiscCaps",     DXGDisplayCaps,  (LPARAM)CapsPrimMiscCaps      },
      {"RasterCaps",            DXGDisplayCaps,  (LPARAM)CapsRasterCaps        },
      {"ZCmpCaps",              DXGDisplayCaps,  (LPARAM)CapsZCmpCaps          },
      {"SrcBlendCaps",          DXGDisplayCaps,  (LPARAM)CapsSrcBlendCaps      },
      {"DestBlendCaps",         DXGDisplayCaps,  (LPARAM)CapsDestBlendCaps     },
      {"AlphaCmpCaps",          DXGDisplayCaps,  (LPARAM)CapsAlphaCmpCaps      },
      {"ShadeCaps",             DXGDisplayCaps,  (LPARAM)CapsShadeCaps         },
      {"TextureCaps",           DXGDisplayCaps,  (LPARAM)CapsTextureCaps       },

      {"TextureFilterCaps",         DXGDisplayCaps,  (LPARAM)CapsTextureFilterCaps },
      {"CubeTextureFilterCaps",     DXGDisplayCaps,  (LPARAM)CapsCubeTextureFilterCaps },
      {"VolumeTextureFilterCaps",   DXGDisplayCaps,  (LPARAM)CapsVolumeTextureFilterCaps },
      {"TextureAddressCaps",        DXGDisplayCaps,  (LPARAM)CapsTextureAddressCaps },
      {"VolumeTextureAddressCaps",  DXGDisplayCaps,  (LPARAM)CapsVolumeTextureAddressCaps },

      {"LineCaps",              DXGDisplayCaps,  (LPARAM)CapsLineCaps },
      {"StencilCaps",           DXGDisplayCaps,  (LPARAM)CapsStencilCaps, },
      {"FVFCaps",               DXGDisplayCaps,  (LPARAM)CapsFVFCaps,     },
      {"TextureOpCaps",         DXGDisplayCaps,  (LPARAM)CapsTextureOpCaps, },
      {"VertexProcessingCaps",  DXGDisplayCaps,  (LPARAM)CapsVertexProcessingCaps, },
      {"DevCaps2",              DXGDisplayCaps,  (LPARAM)CapsDevCaps2, },
      {"DeclTypes",             DXGDisplayCaps,  (LPARAM)CapsDeclTypes, },
      {"StretchRectFilterCaps", DXGDisplayCaps,  (LPARAM)CapsStretchRectFilterCaps, },
      {"VS20Caps",              DXGDisplayCaps,  (LPARAM)CapsVS20Caps, },
      {"PS20Caps",              DXGDisplayCaps,  (LPARAM)CapsPS20Caps, },
      {"VertexTextureFilterCaps",DXGDisplayCaps,  (LPARAM)CapsVertexTextureFilterCaps, },
      
      {"-",                      NULL,            (LPARAM)0,                },
    { NULL, 0, 0 }
};

//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
TCHAR* FormatName(D3DFORMAT format)
{
    switch(format)
    {
    case D3DFMT_UNKNOWN:         return TEXT("D3DFMT_UNKNOWN");
    case D3DFMT_R8G8B8:          return TEXT("D3DFMT_R8G8B8");
    case D3DFMT_A8R8G8B8:        return TEXT("D3DFMT_A8R8G8B8");
    case D3DFMT_X8R8G8B8:        return TEXT("D3DFMT_X8R8G8B8");
    case D3DFMT_R5G6B5:          return TEXT("D3DFMT_R5G6B5");
    case D3DFMT_X1R5G5B5:        return TEXT("D3DFMT_X1R5G5B5");
    case D3DFMT_A1R5G5B5:        return TEXT("D3DFMT_A1R5G5B5");
    case D3DFMT_A4R4G4B4:        return TEXT("D3DFMT_A4R4G4B4");
    case D3DFMT_R3G3B2:          return TEXT("D3DFMT_R3G3B2");
    case D3DFMT_A8:              return TEXT("D3DFMT_A8");
    case D3DFMT_A8R3G3B2:        return TEXT("D3DFMT_A8R3G3B2");
    case D3DFMT_X4R4G4B4:        return TEXT("D3DFMT_X4R4G4B4");
    case D3DFMT_A2B10G10R10:     return TEXT("D3DFMT_A2B10G10R10");
    case D3DFMT_A8B8G8R8:        return TEXT("D3DFMT_A8B8G8R8");
    case D3DFMT_X8B8G8R8:        return TEXT("D3DFMT_X8B8G8R8");
    case D3DFMT_G16R16:          return TEXT("D3DFMT_G16R16");
    case D3DFMT_A2R10G10B10:     return TEXT("D3DFMT_A2R10G10B10");
    case D3DFMT_A16B16G16R16:    return TEXT("D3DFMT_A16B16G16R16");

    case D3DFMT_A8P8:            return TEXT("D3DFMT_A8P8");
    case D3DFMT_P8:              return TEXT("D3DFMT_P8");

    case D3DFMT_L8:              return TEXT("D3DFMT_L8");
    case D3DFMT_A8L8:            return TEXT("D3DFMT_A8L8");
    case D3DFMT_A4L4:            return TEXT("D3DFMT_A4L4");

    case D3DFMT_V8U8:            return TEXT("D3DFMT_V8U8");
    case D3DFMT_L6V5U5:          return TEXT("D3DFMT_L6V5U5");
    case D3DFMT_X8L8V8U8:        return TEXT("D3DFMT_X8L8V8U8");
    case D3DFMT_Q8W8V8U8:        return TEXT("D3DFMT_Q8W8V8U8");
    case D3DFMT_V16U16:          return TEXT("D3DFMT_V16U16");
    case D3DFMT_A2W10V10U10:     return TEXT("D3DFMT_A2W10V10U10");

    case D3DFMT_UYVY:            return TEXT("D3DFMT_UYVY");
    case D3DFMT_R8G8_B8G8:       return TEXT("D3DFMT_R8G8_B8G8");
    case D3DFMT_YUY2:            return TEXT("D3DFMT_YUY2");
    case D3DFMT_G8R8_G8B8:       return TEXT("D3DFMT_G8R8_G8B8");
    case D3DFMT_DXT1:            return TEXT("D3DFMT_DXT1");
    case D3DFMT_DXT2:            return TEXT("D3DFMT_DXT2");
    case D3DFMT_DXT3:            return TEXT("D3DFMT_DXT3");
    case D3DFMT_DXT4:            return TEXT("D3DFMT_DXT4");
    case D3DFMT_DXT5:            return TEXT("D3DFMT_DXT5");

    case D3DFMT_D16_LOCKABLE:    return TEXT("D3DFMT_D16_LOCKABLE");
    case D3DFMT_D32:             return TEXT("D3DFMT_D32");
    case D3DFMT_D15S1:           return TEXT("D3DFMT_D15S1");
    case D3DFMT_D24S8:           return TEXT("D3DFMT_D24S8");
    case D3DFMT_D24X8:           return TEXT("D3DFMT_D24X8");
    case D3DFMT_D24X4S4:         return TEXT("D3DFMT_D24X4S4");
    case D3DFMT_D16:             return TEXT("D3DFMT_D16");
    case D3DFMT_D32F_LOCKABLE:   return TEXT("D3DFMT_D32F_LOCKABLE");
    case D3DFMT_D24FS8:          return TEXT("D3DFMT_D24FS8");

    case D3DFMT_L16:             return TEXT("D3DFMT_L16");

    case D3DFMT_VERTEXDATA:      return TEXT("D3DFMT_VERTEXDATA");
    case D3DFMT_INDEX16:         return TEXT("D3DFMT_INDEX16");
    case D3DFMT_INDEX32:         return TEXT("D3DFMT_INDEX32");

    case D3DFMT_Q16W16V16U16:    return TEXT("D3DFMT_Q16W16V16U16");

    case D3DFMT_MULTI2_ARGB8:    return TEXT("D3DFMT_MULTI2_ARGB8");

    case D3DFMT_R16F:            return TEXT("D3DFMT_R16F");
    case D3DFMT_G16R16F:         return TEXT("D3DFMT_G16R16F");
    case D3DFMT_A16B16G16R16F:   return TEXT("D3DFMT_A16B16G16R16F");
    
    case D3DFMT_R32F:            return TEXT("D3DFMT_R32F");
    case D3DFMT_G32R32F:         return TEXT("D3DFMT_G32R32F");
    case D3DFMT_A32B32G32R32F:   return TEXT("D3DFMT_A32B32G32R32F");

    case D3DFMT_CxV8U8:          return TEXT("D3DFMT_CxV8U8");

    case D3DFMT_D32_LOCKABLE:	return TEXT("D3DFMT_D32_LOCKABLE");
    case D3DFMT_S8_LOCKABLE:	return TEXT("D3DFMT_S8_LOCKABLE");
    case D3DFMT_A1:				return TEXT("D3DFMT_A1");

    default:                     return TEXT("Unknown format");
    }
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
TCHAR* MultiSampleTypeName(D3DMULTISAMPLE_TYPE msType)
{
    switch(msType)
    {
    case D3DMULTISAMPLE_NONE:       return TEXT("D3DMULTISAMPLE_NONE");
    case D3DMULTISAMPLE_NONMASKABLE:   return TEXT("D3DMULTISAMPLE_NONMASKABLE");
    case D3DMULTISAMPLE_2_SAMPLES:  return TEXT("D3DMULTISAMPLE_2_SAMPLES");
    case D3DMULTISAMPLE_3_SAMPLES:  return TEXT("D3DMULTISAMPLE_3_SAMPLES");
    case D3DMULTISAMPLE_4_SAMPLES:  return TEXT("D3DMULTISAMPLE_4_SAMPLES");
    case D3DMULTISAMPLE_5_SAMPLES:  return TEXT("D3DMULTISAMPLE_5_SAMPLES");
    case D3DMULTISAMPLE_6_SAMPLES:  return TEXT("D3DMULTISAMPLE_6_SAMPLES");
    case D3DMULTISAMPLE_7_SAMPLES:  return TEXT("D3DMULTISAMPLE_7_SAMPLES");
    case D3DMULTISAMPLE_8_SAMPLES:  return TEXT("D3DMULTISAMPLE_8_SAMPLES");
    case D3DMULTISAMPLE_9_SAMPLES:  return TEXT("D3DMULTISAMPLE_9_SAMPLES");
    case D3DMULTISAMPLE_10_SAMPLES: return TEXT("D3DMULTISAMPLE_10_SAMPLES");
    case D3DMULTISAMPLE_11_SAMPLES: return TEXT("D3DMULTISAMPLE_11_SAMPLES");
    case D3DMULTISAMPLE_12_SAMPLES: return TEXT("D3DMULTISAMPLE_12_SAMPLES");
    case D3DMULTISAMPLE_13_SAMPLES: return TEXT("D3DMULTISAMPLE_13_SAMPLES");
    case D3DMULTISAMPLE_14_SAMPLES: return TEXT("D3DMULTISAMPLE_14_SAMPLES");
    case D3DMULTISAMPLE_15_SAMPLES: return TEXT("D3DMULTISAMPLE_15_SAMPLES");
    case D3DMULTISAMPLE_16_SAMPLES: return TEXT("D3DMULTISAMPLE_16_SAMPLES");
    default:                        return TEXT("Unknown type");
    }
}

//-----------------------------------------------------------------------------
// Name:
// Desc:
//     lParam1 is the iAdapter
//     lParam2 is not used
//-----------------------------------------------------------------------------
HRESULT DXGDisplayAdapterInfo( LPARAM lParam1, LPARAM lParam2,
                         PRINTCBINFO* pPrintInfo )
{
    HRESULT hr;
    UINT iAdapter = (UINT)lParam1;
    D3DADAPTER_IDENTIFIER9 identifier;
    GUID guid;
    TCHAR szGuid[50];
    TCHAR szVersion[50];

    if (g_pD3D == NULL)
        return S_OK;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Name", 15);
        LVAddColumn(g_hwndLV, 1, "Value", 40);
    }

    if (FAILED(hr = g_pD3D->GetAdapterIdentifier(iAdapter, D3DENUM_WHQL_LEVEL, &identifier)))
        return hr;

    guid = identifier.DeviceIdentifier;
    sprintf_s(szGuid, sizeof(szGuid), TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
        guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
        guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    sprintf_s(szVersion, sizeof(szVersion), TEXT("0x%08X-%08X"), identifier.DriverVersion.HighPart,
        identifier.DriverVersion.LowPart);

    if ( pPrintInfo == NULL)
    {
        LVAddText( g_hwndLV, 0, "Driver");
        LVAddText( g_hwndLV, 1, "%s", identifier.Driver);

        LVAddText( g_hwndLV, 0, "Description");
        LVAddText( g_hwndLV, 1, "%s", identifier.Description);

        LVAddText( g_hwndLV, 0, "DeviceName");
        LVAddText( g_hwndLV, 1, "%s", identifier.DeviceName);

        LVAddText( g_hwndLV, 0, "DriverVersion");
        LVAddText( g_hwndLV, 1, "%s", szVersion);

        LVAddText( g_hwndLV, 0, "VendorId");
        LVAddText( g_hwndLV, 1, "0x%08x", identifier.VendorId);

        LVAddText( g_hwndLV, 0, "DeviceId");
        LVAddText( g_hwndLV, 1, "0x%08x", identifier.DeviceId);

        LVAddText( g_hwndLV, 0, "SubSysId");
        LVAddText( g_hwndLV, 1, "0x%08x", identifier.SubSysId);

        LVAddText( g_hwndLV, 0, "Revision");
        LVAddText( g_hwndLV, 1, "%d", identifier.Revision);

        LVAddText( g_hwndLV, 0, "DeviceIdentifier");
        LVAddText( g_hwndLV, 1, szGuid);

        LVAddText( g_hwndLV, 0, "WHQLLevel");
        LVAddText( g_hwndLV, 1, "%d", identifier.WHQLLevel);
    }
    else
    {
        PrintStringValueLine( "Driver", identifier.Driver, pPrintInfo );
        PrintStringValueLine( "Description", identifier.Description, pPrintInfo );
        PrintStringValueLine( "DriverVersion", szVersion, pPrintInfo );
        PrintHexValueLine( "VendorId", identifier.VendorId, pPrintInfo );
        PrintHexValueLine( "DeviceId", identifier.DeviceId, pPrintInfo );
        PrintHexValueLine( "SubSysId", identifier.SubSysId, pPrintInfo );
        PrintValueLine( "Revision", identifier.Revision, pPrintInfo );
        PrintStringValueLine( "DeviceIdentifier", szGuid, pPrintInfo );
        PrintValueLine( "WHQLLevel", identifier.WHQLLevel, pPrintInfo );
    }
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//     lParam1 is the iAdapter
//     lParam2 is not used
//-----------------------------------------------------------------------------
HRESULT DXGDisplayModes( LPARAM lParam1, LPARAM lParam2,
                         PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = (UINT)lParam1;
    INT iFormat;
    UINT iMode;
    D3DFORMAT fmt;
    D3DDISPLAYMODE mode;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Resolution", 10);
        LVAddColumn(g_hwndLV, 1, "Pixel Format", 15);
        LVAddColumn(g_hwndLV, 2, "Refresh Rate", 10);
    }

    for (iFormat = 0; iFormat < NumAdapterFormats; iFormat++)
    {
        fmt = AdapterFormatArray[iFormat];
        UINT numModes = g_pD3D->GetAdapterModeCount(iAdapter, fmt);
        for (iMode = 0; iMode < numModes; iMode++)
        {
            g_pD3D->EnumAdapterModes(iAdapter, fmt, iMode, &mode);
            if (pPrintInfo == NULL)
            {
                LVAddText( g_hwndLV, 0, "%d x %d", mode.Width, mode.Height);
                LVAddText( g_hwndLV, 1, FormatName(mode.Format));
                LVAddText( g_hwndLV, 2, "%d", mode.RefreshRate);
            }
            else
            {
                char  szBuff[80];
                DWORD cchLen;
                int   x1, x2, x3, yLine;

                // Calculate Name and Value column x offsets
                x1   = (pPrintInfo->dwCurrIndent * DEF_TAB_SIZE * pPrintInfo->dwCharWidth);
                x2    = x1 + (20 * pPrintInfo->dwCharWidth);
                x3    = x2 + (20 * pPrintInfo->dwCharWidth);
                yLine   = (pPrintInfo->dwCurrLine * pPrintInfo->dwLineHeight);

                sprintf_s (szBuff, sizeof(szBuff), "%d x %d", mode.Width, mode.Height);
                cchLen = _tcslen (szBuff);
                if( FAILED( PrintLine (x1, yLine, szBuff, cchLen, pPrintInfo) ) )
                    return E_FAIL;

                strcpy_s (szBuff, sizeof(szBuff), FormatName(mode.Format));
                cchLen = _tcslen (szBuff);
                if( FAILED( PrintLine (x2, yLine, szBuff, cchLen, pPrintInfo) ) )
                    return E_FAIL;

                sprintf_s (szBuff, sizeof(szBuff), "%d", mode.RefreshRate);
                cchLen = _tcslen (szBuff);
                if( FAILED( PrintLine (x3, yLine, szBuff, cchLen, pPrintInfo) ) )
                    return E_FAIL;

                // Advance to next line on page
                if( FAILED( PrintNextLine(pPrintInfo) ) )
                    return E_FAIL;
            }
        }
    }
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//     lParam1 is the caps pointer
//     lParam2 is the CAPDEF table we should use
//-----------------------------------------------------------------------------
HRESULT DXGDisplayCaps( LPARAM lParam1, LPARAM lParam2,
                        PRINTCBINFO* pPrintInfo )
{
    D3DCAPS9* pCaps = (D3DCAPS9*)lParam1;
    CAPDEF* pCapDef = (CAPDEF*)lParam2;

    if( pPrintInfo )
        return PrintCapsToDC( pCapDef, (VOID*)pCaps, pPrintInfo );
    else
        AddCapsToLV(pCapDef, (LPVOID)pCaps);

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is bWindowed and the msType
//     lParam3 is the render format
//-----------------------------------------------------------------------------
HRESULT DXGDisplayMultiSample( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    BOOL bWindowed = (BOOL)LOWORD(lParam2);
    D3DMULTISAMPLE_TYPE msType = (D3DMULTISAMPLE_TYPE)HIWORD(lParam2);
    D3DFORMAT fmt = (D3DFORMAT)lParam3;
    DWORD dwNumQualityLevels;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Quality Levels", 30);
    }

    if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, fmt, bWindowed, msType, &dwNumQualityLevels)))
    {
        TCHAR str[100];
        if( dwNumQualityLevels == 1 )
            sprintf_s( str, sizeof(str), "%d quality level", dwNumQualityLevels);
        else
            sprintf_s( str, sizeof(str), "%d quality levels", dwNumQualityLevels);
        if (pPrintInfo == NULL)
        {
            LVAddText( g_hwndLV, 0, str);
        }
        else
        {
            PrintStringLine(str, pPrintInfo);
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is the adapter fmt
//     lParam3 is bWindowed
//-----------------------------------------------------------------------------
HRESULT DXGDisplayBackBuffer( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    D3DFORMAT fmtAdapter = (D3DFORMAT)lParam2;
    BOOL bWindowed = (BOOL)lParam3;
    D3DFORMAT fmt;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Back Buffer Formats", 20);
    }

    for (int iFmt = 0; iFmt < NumBBFormats; iFmt++)
    {
        fmt = BBFormatArray[iFmt];
        if (SUCCEEDED(g_pD3D->CheckDeviceType(iAdapter, devType, fmtAdapter, fmt, bWindowed)))
        {
            if (pPrintInfo == NULL)
            {
                LVAddText( g_hwndLV, 0, "%s", FormatName(fmt));
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
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is the adapter fmt
//     lParam3 is unused
//-----------------------------------------------------------------------------
HRESULT DXGDisplayRenderTarget( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    D3DFORMAT fmtAdapter = (D3DFORMAT)lParam2;
    D3DFORMAT fmt;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Render Target Formats", 20);
    }

    for (int iFmt = 0; iFmt < NumFormats; iFmt++)
    {
        fmt = AllFormatArray[iFmt];
        if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_RENDERTARGET,
            D3DRTYPE_SURFACE, fmt)))
        {
            if (pPrintInfo == NULL)
            {
                LVAddText( g_hwndLV, 0, "%s", FormatName(fmt));
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
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is the adapter fmt
//     lParam3 is unused
//-----------------------------------------------------------------------------
HRESULT DXGDisplayDepthStencil( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    D3DFORMAT fmtAdapter = (D3DFORMAT)lParam2;
    D3DFORMAT fmt;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Depth/Stencil Formats", 20);
    }

    for (int iFmt = 0; iFmt < NumDSFormats; iFmt++)
    {
        fmt = DSFormatArray[iFmt];

        if (!g_is9Ex && ((fmt == D3DFMT_D32_LOCKABLE) || (fmt == D3DFMT_S8_LOCKABLE)))
            continue;

        if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_DEPTHSTENCIL,
            D3DRTYPE_SURFACE, fmt)))
        {
            if (pPrintInfo == NULL)
            {
                LVAddText( g_hwndLV, 0, "%s", FormatName(fmt));
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
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is the depth/stencil fmt
//     lParam3 is the msType
//-----------------------------------------------------------------------------
HRESULT DXGCheckDSQualityLevels( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    D3DFORMAT fmtDS = (D3DFORMAT)lParam2;
    D3DMULTISAMPLE_TYPE msType = (D3DMULTISAMPLE_TYPE)lParam3;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Quality Levels", 20);
    }

    DWORD dwNumQualityLevels;
    if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, fmtDS, FALSE, msType, &dwNumQualityLevels)))
    {
        TCHAR str[100];
        if( dwNumQualityLevels == 1 )
            sprintf_s( str, sizeof(str), "%d quality level", dwNumQualityLevels);
        else
            sprintf_s( str, sizeof(str), "%d quality levels", dwNumQualityLevels);
        if (pPrintInfo == NULL)
        {
            LVAddText( g_hwndLV, 0, str);
        }
        else
        {
            PrintStringLine(str, pPrintInfo);
        }
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is the adapter fmt
//     lParam3 is unused
//-----------------------------------------------------------------------------
HRESULT DXGDisplayPlainSurface( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    D3DFORMAT fmtAdapter = (D3DFORMAT)lParam2;
    D3DFORMAT fmt;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Plain Surface Formats", 20);
    }

    for (int iFmt = 0; iFmt < NumFormats; iFmt++)
    {
        fmt = AllFormatArray[iFmt];
        
        if (!g_is9Ex && ((fmt == D3DFMT_A1) || (fmt == D3DFMT_D32_LOCKABLE) || (fmt == D3DFMT_S8_LOCKABLE)))
            continue;

        if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, 0, 
            D3DRTYPE_SURFACE, fmt)))
        {
            if (pPrintInfo == NULL)
            {
                LVAddText( g_hwndLV, 0, "%s", FormatName(fmt));
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
// Name:
// Desc:
//     lParam1 is the iAdapter and device type
//     lParam2 is the fmt of the swap chain
//     lParam3 is the D3DRESOURCETYPE
//-----------------------------------------------------------------------------
HRESULT DXGDisplayResource( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo )
{
    UINT iAdapter = LOWORD(lParam1);
    D3DDEVTYPE devType = (D3DDEVTYPE)HIWORD(lParam1);
    D3DFORMAT fmtAdapter = (D3DFORMAT)lParam2;
    D3DRESOURCETYPE RType = (D3DRESOURCETYPE)lParam3;
    D3DFORMAT fmt;
    TCHAR* pstr;
    HRESULT hr;
    D3DCAPS9 Caps;
    UINT col = 0;

    g_pD3D->GetDeviceCaps( iAdapter, devType, &Caps );

    if( pPrintInfo == NULL )
    {
        switch( RType )
        {
        case D3DRTYPE_SURFACE:
            LVAddColumn(g_hwndLV, col++, "Surface Formats", 20);
            break;
        case D3DRTYPE_VOLUME:
            LVAddColumn(g_hwndLV, col++, "Volume Formats", 20);
            break;
        case D3DRTYPE_TEXTURE:
            LVAddColumn(g_hwndLV, col++, "Texture Formats", 20);
            break;
        case D3DRTYPE_VOLUMETEXTURE:
            LVAddColumn(g_hwndLV, col++, "Volume Texture Formats", 20);
            break;
        case D3DRTYPE_CUBETEXTURE:
            LVAddColumn(g_hwndLV, col++, "Cube Texture Formats", 20);
            break;
        default:
            return E_FAIL;
        }
        LVAddColumn(g_hwndLV,  col++, "0 (Plain)", 22);
        if( RType != D3DRTYPE_VOLUMETEXTURE )
            LVAddColumn(g_hwndLV,  col++, "D3DUSAGE_RENDERTARGET", 22);
//        LVAddColumn(g_hwndLV,  col++, "D3DUSAGE_DEPTHSTENCIL", 22);
        if( RType != D3DRTYPE_SURFACE )
        {
            if( RType != D3DRTYPE_VOLUMETEXTURE )
            {
                LVAddColumn(g_hwndLV, col++, "D3DUSAGE_AUTOGENMIPMAP", 22);
                if( RType != D3DRTYPE_CUBETEXTURE )
                {
                    LVAddColumn(g_hwndLV, col++, "D3DUSAGE_DMAP", 22);
                }
            }
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_LEGACYBUMPMAP", 22);
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_SRGBREAD", 18);
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_FILTER", 15);
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_SRGBWRITE", 18);
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING", 18);
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_VERTEXTEXTURE", 18);
            LVAddColumn(g_hwndLV, col++, "D3DUSAGE_QUERY_WRAPANDMIP", 18);
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
        fmt = AllFormatArray[iFmt];

        if (!g_is9Ex && ((fmt == D3DFMT_A1) || (fmt == D3DFMT_D32_LOCKABLE) || (fmt == D3DFMT_S8_LOCKABLE)))
            continue;

        // First pass: see if CDF succeeds for any usage
        bFoundSuccess = FALSE;
        for( UINT iUsage = 0; iUsage < numUsages; iUsage++ )
        {
            if( RType == D3DRTYPE_SURFACE )
            {
                if( usageArray[iUsage] != 0 &&
                    usageArray[iUsage] != D3DUSAGE_DEPTHSTENCIL &&
                    usageArray[iUsage] != D3DUSAGE_RENDERTARGET )
                {
                    continue;
                }
            }
            if( RType == D3DRTYPE_VOLUMETEXTURE )
            {
                if( usageArray[iUsage] == D3DUSAGE_DEPTHSTENCIL || 
                    usageArray[iUsage] == D3DUSAGE_RENDERTARGET ||
                    usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP ||
                    usageArray[iUsage] == D3DUSAGE_DMAP )
                {
                    continue;
                }
            }
            if( RType == D3DRTYPE_CUBETEXTURE )
            {
                if( usageArray[iUsage] == D3DUSAGE_DMAP )
                {
                    continue;
                }
            }
            if( usageArray[iUsage] == D3DUSAGE_DMAP )
            {
                if( (Caps.DevCaps2 & D3DDEVCAPS2_DMAPNPATCH) == 0 &&
                    (Caps.DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH) == 0 )
                {
                    continue;
                }
            }
            if( fmt == D3DFMT_MULTI2_ARGB8 && usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP )
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
        if( bFoundSuccess )
        {
            col = 0;
            // Add list item for this format
            if (pPrintInfo == NULL)
                LVAddText( g_hwndLV, col++, "%s", FormatName(fmt));
            else
                PrintStringLine(FormatName(fmt), pPrintInfo);

            // Show which usages it is compatible with
            for( UINT iUsage = 0; iUsage < numUsages; iUsage++ )
            {
                if( RType == D3DRTYPE_SURFACE )
                {
                    if( usageArray[iUsage] != 0 &&
                        usageArray[iUsage] != D3DUSAGE_DEPTHSTENCIL &&
                        usageArray[iUsage] != D3DUSAGE_RENDERTARGET )
                    {
                        continue;
                    }
                }
                if( RType == D3DRTYPE_VOLUMETEXTURE )
                {
                    if( usageArray[iUsage] == D3DUSAGE_DEPTHSTENCIL || 
                        usageArray[iUsage] == D3DUSAGE_RENDERTARGET ||
                        usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP ||
                        usageArray[iUsage] == D3DUSAGE_DMAP )
                    {
                        continue;
                    }
                }
                if( RType == D3DRTYPE_CUBETEXTURE )
                {
                    if( usageArray[iUsage] == D3DUSAGE_DMAP )
                    {
                        continue;
                    }
                }
                hr = S_OK;
                if( usageArray[iUsage] == D3DUSAGE_DMAP )
                {
                    if( (Caps.DevCaps2 & D3DDEVCAPS2_DMAPNPATCH) == 0 &&
                        (Caps.DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH) == 0 )
                    {
                        hr = E_FAIL;
                    }
                }
                if( fmt == D3DFMT_MULTI2_ARGB8 && usageArray[iUsage] == D3DUSAGE_AUTOGENMIPMAP )
                {
                    hr = E_FAIL;
                }
                if( SUCCEEDED( hr ) )
                {
                    hr = g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, 
                        usageArray[iUsage], RType, fmt);
                }
                if( hr == D3DOK_NOAUTOGEN )
                    pstr = TEXT("No");
                else if(SUCCEEDED(hr))
                    pstr = TEXT("Yes");
                else
                    pstr = TEXT("No");
                if (pPrintInfo == NULL)
                    LVAddText( g_hwndLV, col++, pstr);
                else
                    PrintStringLine(pstr, pPrintInfo);
            }
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
BOOL IsBBFmt(D3DFORMAT fmt)
{
    D3DFORMAT fmtBackBuffer;
    for (int iFmtBackBuffer = 0; iFmtBackBuffer < NumBBFormats; iFmtBackBuffer++)
    {
        fmtBackBuffer = BBFormatArray[iFmtBackBuffer];
        if( fmt == fmtBackBuffer )
            return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
BOOL IsDeviceTypeAvailable(UINT iAdapter, D3DDEVTYPE devType)
{
    D3DFORMAT fmtAdapter;
    for (int iFmtAdapter = 0; iFmtAdapter < NumAdapterFormats; iFmtAdapter++)
    {
        fmtAdapter = AdapterFormatArray[iFmtAdapter];
        if( IsAdapterFmtAvailable( iAdapter, devType, fmtAdapter, TRUE ) ||
            IsAdapterFmtAvailable( iAdapter, devType, fmtAdapter, FALSE ) )
        {
            return TRUE;
        }
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
BOOL IsAdapterFmtAvailable(UINT iAdapter, D3DDEVTYPE devType, D3DFORMAT fmtAdapter, BOOL bWindowed)
{
    if( fmtAdapter == D3DFMT_A2R10G10B10 && bWindowed )
        return FALSE; // D3D extended primary formats are only avaiable for fullscreen mode

    D3DFORMAT fmtBackBuffer;
    for (int iFmtBackBuffer = 0; iFmtBackBuffer < NumBBFormats; iFmtBackBuffer++)
    {
        fmtBackBuffer = BBFormatArray[iFmtBackBuffer];
        if (SUCCEEDED(g_pD3D->CheckDeviceType(iAdapter, devType, fmtAdapter, fmtBackBuffer, bWindowed)))
        {
            return TRUE;
        }
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: DXG_Init()
// Desc:
//-----------------------------------------------------------------------------
VOID DXG_Init()
{
    g_is9Ex = FALSE;

    Direct3DCreate9ExFunc Direct3DCreate9Ex = NULL;

    HMODULE d3d9 = LoadLibrary("d3d9.dll");
    if (d3d9 != NULL)
    {
        Direct3DCreate9Ex = (Direct3DCreate9ExFunc)GetProcAddress(d3d9, "Direct3DCreate9Ex");
        if (Direct3DCreate9Ex != NULL)
        {
            g_is9Ex = SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D));
        }

        FreeLibrary(d3d9);
        d3d9 = NULL;
    }

    if (!g_is9Ex)
        g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
}




//-----------------------------------------------------------------------------
// Name: DXG_FillTree()
// Desc:
//-----------------------------------------------------------------------------
VOID DXG_FillTree( HWND hwndTV )
{
    HRESULT hr;
    HTREEITEM hTree;
    HTREEITEM hTree2;
    HTREEITEM hTree3;
    HTREEITEM hTree4;
    HTREEITEM hTree5;
    HTREEITEM hTree6;
    HTREEITEM hTree7;
    HTREEITEM hTree8;
    HTREEITEM hTree9;
    HTREEITEM hTree10;
    UINT numAdapters;
    UINT iAdapter;
    D3DADAPTER_IDENTIFIER9 identifier;
    D3DDEVTYPE deviceTypeArray[] = { D3DDEVTYPE_HAL, D3DDEVTYPE_SW, D3DDEVTYPE_REF };
    D3DDEVTYPE devType;
    TCHAR *deviceNameArray[] = { "HAL", "Software", "Reference" };
    UINT numDeviceTypes = sizeof(deviceTypeArray) / sizeof(deviceTypeArray[0]);
    UINT iDevice;
    D3DCAPS9 caps;
    D3DCAPS9* pCapsCopy;

    if (g_pD3D == NULL)
        return;

    hTree = TVAddNode( TVI_ROOT, "Direct3D9 Devices", TRUE, IDI_DIRECTX,
                       NULL, 0, 0 );

    numAdapters = g_pD3D->GetAdapterCount();
    for (iAdapter = 0; iAdapter < numAdapters; iAdapter++)
    {
        if (SUCCEEDED(g_pD3D->GetAdapterIdentifier(iAdapter, 0, &identifier)))
        {
            hTree2 = TVAddNode(hTree, identifier.Description, TRUE, IDI_CAPS,
                DXGDisplayAdapterInfo, iAdapter, 0 );
            (void)TVAddNode( hTree2, "Display Modes", FALSE, IDI_CAPS,
                                DXGDisplayModes, iAdapter, 0 );
            hTree3 = TVAddNode( hTree2, "D3D Device Types", TRUE, IDI_CAPS,
                                NULL, 0, 0 );

            for (iDevice = 0; iDevice < numDeviceTypes; iDevice++)
            {
                devType = deviceTypeArray[iDevice];

                if( devType == D3DDEVTYPE_SW )
                    continue; // we don't register a SW device, so just skip it

                if (!IsDeviceTypeAvailable(iAdapter, devType))
                    continue;

                // Add caps for each device
                hr = g_pD3D->GetDeviceCaps(iAdapter, devType, &caps);
                if (FAILED(hr))
                    memset(&caps,0,sizeof(caps));
                pCapsCopy = new D3DCAPS9;
                if (pCapsCopy == NULL)
                    continue;
                *pCapsCopy = caps;
                hTree4 = TVAddNode(hTree3, deviceNameArray[iDevice], TRUE, IDI_CAPS, NULL, 0, 0);
                AddCapsToTV(hTree4, DXGCapDefs, (LPARAM)pCapsCopy);

                // List adapter formats for each device
                hTree5 = TVAddNode(hTree4, "Adapter Formats", TRUE, IDI_CAPS, NULL, 0, 0);
                D3DFORMAT fmtAdapter;
                for (int iFmtAdapter = 0; iFmtAdapter < NumAdapterFormats; iFmtAdapter++)
                {
                    fmtAdapter = AdapterFormatArray[iFmtAdapter];
                    for( BOOL bWindowed = FALSE; bWindowed < 2; bWindowed++ )
                    {
                        if (!IsAdapterFmtAvailable(iAdapter, devType, fmtAdapter, bWindowed))
                            continue;
                        
                        TCHAR sz[100];
                        sprintf_s(sz, sizeof(sz), "%s %s", FormatName(fmtAdapter), bWindowed ? "(Windowed)" : "(Fullscreen)");
                        hTree6 = TVAddNode(hTree5, sz, TRUE, IDI_CAPS, NULL, 0, 0);
                        TVAddNodeEx(hTree6, "Back Buffer Formats", FALSE, IDI_CAPS, DXGDisplayBackBuffer, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)bWindowed);
                        TVAddNodeEx(hTree6, "Render Target Formats", FALSE, IDI_CAPS, DXGDisplayRenderTarget, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)0);
                        TVAddNodeEx(hTree6, "Depth/Stencil Formats", FALSE, IDI_CAPS, DXGDisplayDepthStencil, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)0);
                        TVAddNodeEx(hTree6, "Plain Surface Formats", FALSE, IDI_CAPS, DXGDisplayPlainSurface, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)0);
                        TVAddNodeEx(hTree6, "Texture Formats", FALSE, IDI_CAPS, DXGDisplayResource, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)D3DRTYPE_TEXTURE);
                        TVAddNodeEx(hTree6, "Cube Texture Formats", FALSE, IDI_CAPS, DXGDisplayResource, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)D3DRTYPE_CUBETEXTURE);
                        TVAddNodeEx(hTree6, "Volume Texture Formats", FALSE, IDI_CAPS, DXGDisplayResource, MAKELPARAM(iAdapter, (UINT)devType), (LPARAM)fmtAdapter, (LPARAM)D3DRTYPE_VOLUMETEXTURE);
                        hTree7 = TVAddNode(hTree6, "Render Format Compatibility", TRUE, IDI_CAPS, NULL, 0, 0);
                        D3DFORMAT fmtRender;
                        for (int iFmtRender = 0; iFmtRender < NumFormats; iFmtRender++)
                        {
                            fmtRender = AllFormatArray[iFmtRender];
                            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, fmtRender)) ||
                                IsBBFmt(fmtRender) && SUCCEEDED(g_pD3D->CheckDeviceType(iAdapter, devType, fmtAdapter, fmtRender, bWindowed)))
                            {
                                hTree8 = TVAddNode(hTree7, FormatName(fmtRender), TRUE, IDI_CAPS, NULL, 0, 0);
                                for (D3DMULTISAMPLE_TYPE msType = D3DMULTISAMPLE_NONE; msType <= D3DMULTISAMPLE_16_SAMPLES; msType = (D3DMULTISAMPLE_TYPE)((UINT)msType + 1))
                                {
                                    if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, fmtRender, bWindowed, msType, NULL)))
                                    {
                                        hTree9 = TVAddNodeEx(hTree8, MultiSampleTypeName(msType), TRUE, IDI_CAPS, DXGDisplayMultiSample, MAKELPARAM(iAdapter, (UINT)devType), MAKELPARAM(bWindowed, (UINT)msType), (LPARAM)fmtRender);
                                        hTree10 = TVAddNode(hTree9, "Compatible Depth/Stencil Formats", TRUE, IDI_CAPS, NULL, 0, 0);
                                        D3DFORMAT DSFmt;
                                        for (int iFmt = 0; iFmt < NumDSFormats; iFmt++)
                                        {
                                            DSFmt = DSFormatArray[iFmt];
                                            if (SUCCEEDED(g_pD3D->CheckDeviceFormat(iAdapter, devType, fmtAdapter, D3DUSAGE_DEPTHSTENCIL,
                                                D3DRTYPE_SURFACE, DSFmt)))
                                            {
                                                if (SUCCEEDED(g_pD3D->CheckDepthStencilMatch(iAdapter, devType, fmtAdapter, fmtRender, DSFmt)))
                                                {
                                                    if (SUCCEEDED(g_pD3D->CheckDeviceMultiSampleType(iAdapter, devType, DSFmt, bWindowed, msType, NULL)))
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

    TreeView_Expand( hwndTV, hTree, TVE_EXPAND );
}




//-----------------------------------------------------------------------------
// Name: DXG_CleanUp()
// Desc:
//-----------------------------------------------------------------------------
VOID DXG_CleanUp()
{
    if( g_pD3D )
        g_pD3D->Release();
}


BOOL DXG_Is9Ex()
{
    return (BOOL)g_is9Ex;
}
