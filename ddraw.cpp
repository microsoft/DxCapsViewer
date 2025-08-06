//-----------------------------------------------------------------------------
// Name: ddraw.cpp
//
// Desc: DirectX Capabilities Viewer for DirectDraw
//
// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?linkid=2136896
//-----------------------------------------------------------------------------
#include "dxview.h"

#include <ddraw.h>
#include <stdio.h>

extern HRESULT Int2Str(_Out_writes_bytes_(nDestLen) LPWSTR pszDest, UINT nDestLen, DWORD i);

namespace
{
    using LPDIRECTDRAWCREATEEX = HRESULT(WINAPI*)(GUID FAR* lpGuid, LPVOID* lplpDD, REFIID  iid, IUnknown FAR* pUnkOuter);

    LPDIRECTDRAW7 g_pDD = nullptr;
    HMODULE g_hInstDDraw = nullptr;
    GUID* g_pDDGUID;

    LPDIRECTDRAWCREATEEX g_directDrawCreateEx = nullptr;
    LPDIRECTDRAWENUMERATEEXA g_directDrawEnumerateEx = nullptr;

    HRESULT DDDisplayVidMem(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pPrintInfo);
    HRESULT DDDisplayCaps(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pInfo);
    HRESULT DDDisplayVideoModes(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pInfo);
    HRESULT DDDisplayFourCCFormat(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pInfo);

#define DDCAPDEFex(name,val,flag) {name, FIELD_OFFSET(DDCAPS,val), flag, DXV_9EXCAP}
#define DDCAPDEF(name,val,flag) {name, FIELD_OFFSET(DDCAPS,val), flag}
#define DDVALDEF(name,val)      {name, FIELD_OFFSET(DDCAPS,val), 0}
#define DDHEXDEF(name,val)      {name, FIELD_OFFSET(DDCAPS,val), 0xFFFFFFFF}
#define ROPDEF(name,dwRops,rop) DDCAPDEF(name,dwRops[((rop>>16)&0xFF)/32],static_cast<DWORD>((1<<((rop>>16)&0xFF)%32)))
#define MAKEMODE(xres,yres,bpp) (((DWORD)xres << 20) | ((DWORD)yres << 8) | bpp)


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF OtherInfoDefs[] =
    {
        DDVALDEF(L"dwVidMemTotal",                   dwVidMemTotal),
        DDVALDEF(L"dwVidMemFree",                    dwVidMemFree),
        DDHEXDEF(L"dwAlphaBltConstBitDepths",        dwAlphaBltConstBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwAlphaBltConstBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwAlphaBltConstBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwAlphaBltConstBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwAlphaBltConstBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaBltPixelBitDepths",        dwAlphaBltPixelBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwAlphaBltPixelBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwAlphaBltPixelBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwAlphaBltPixelBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwAlphaBltPixelBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaBltSurfaceBitDepths",      dwAlphaBltSurfaceBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwAlphaBltSurfaceBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwAlphaBltSurfaceBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwAlphaBltSurfaceBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwAlphaBltSurfaceBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaOverlayConstBitDepths",    dwAlphaOverlayConstBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwAlphaOverlayConstBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwAlphaOverlayConstBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwAlphaOverlayConstBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwAlphaOverlayConstBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaOverlayPixelBitDepths",    dwAlphaOverlayPixelBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwAlphaOverlayPixelBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwAlphaOverlayPixelBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwAlphaOverlayPixelBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwAlphaOverlayPixelBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaOverlaySurfaceBitDepths",  dwAlphaOverlaySurfaceBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwAlphaOverlaySurfaceBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwAlphaOverlaySurfaceBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwAlphaOverlaySurfaceBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwAlphaOverlaySurfaceBitDepths, DDBD_32),
        DDHEXDEF(L"dwZBufferBitDepths",              dwZBufferBitDepths),
        DDCAPDEF(L"  DDBD_8",                           dwZBufferBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                          dwZBufferBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                          dwZBufferBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                          dwZBufferBitDepths, DDBD_32),
        DDVALDEF(L"dwMaxVisibleOverlays",            dwMaxVisibleOverlays),
        DDVALDEF(L"dwCurrVisibleOverlays",           dwCurrVisibleOverlays),
        DDVALDEF(L"dwNumFourCCCodes",                dwNumFourCCCodes),
        DDVALDEF(L"dwAlignBoundarySrc",              dwAlignBoundarySrc),
        DDVALDEF(L"dwAlignSizeSrc",                  dwAlignSizeSrc),
        DDVALDEF(L"dwAlignBoundaryDest",             dwAlignBoundaryDest),
        DDVALDEF(L"dwAlignSizeDest",                 dwAlignSizeDest),
        DDVALDEF(L"dwAlignStrideAlign",              dwAlignStrideAlign),
        DDVALDEF(L"dwMinOverlayStretch",             dwMinOverlayStretch),
        DDVALDEF(L"dwMaxOverlayStretch",             dwMaxOverlayStretch),
        DDVALDEF(L"dwMinLiveVideoStretch",           dwMinLiveVideoStretch),
        DDVALDEF(L"dwMaxLiveVideoStretch",           dwMaxLiveVideoStretch),
        DDVALDEF(L"dwMinHwCodecStretch",             dwMinHwCodecStretch),
        DDVALDEF(L"dwMaxHwCodecStretch",             dwMaxHwCodecStretch),

        //DDHEXDEF("dwCaps",                      dwCaps),
        DDVALDEF(L"dwMaxVideoPorts",               dwMaxVideoPorts),
        DDVALDEF(L"dwCurrVideoPorts",               dwCurrVideoPorts),
        //DDVALDEF("dwSVBCaps2",                   dwSVBCaps2),
        { L"", 0, 0 }
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------


#define GEN_BLTCAPS(dwCaps)                                                     \
    DDCAPDEF(L"DDCAPS_BLT",                       dwCaps, DDCAPS_BLT),                  \
    DDCAPDEF(L"DDCAPS_BLTCOLORFILL",              dwCaps, DDCAPS_BLTCOLORFILL),         \
    DDCAPDEF(L"DDCAPS_BLTDEPTHFILL",              dwCaps, DDCAPS_BLTDEPTHFILL),         \
    DDCAPDEF(L"DDCAPS_BLTFOURCC",                 dwCaps, DDCAPS_BLTFOURCC),            \
    DDCAPDEF(L"DDCAPS_BLTSTRETCH",                dwCaps, DDCAPS_BLTSTRETCH),           \
    DDCAPDEF(L"DDCAPS_BLTQUEUE",                  dwCaps, DDCAPS_BLTQUEUE),             \
    DDCAPDEF(L"DDCAPS_COLORKEY",                  dwCaps, DDCAPS_COLORKEY),             \
    DDCAPDEF(L"DDCAPS_ALPHA",                     dwCaps, DDCAPS_ALPHA),                \
    DDCAPDEF(L"DDCAPS_CKEYHWASSIST",              dwCaps, DDCAPS_COLORKEYHWASSIST),     \
    DDCAPDEF(L"DDCAPS_CANCLIP",                   dwCaps, DDCAPS_CANCLIP),              \
    DDCAPDEF(L"DDCAPS_CANCLIPSTRETCHED",          dwCaps, DDCAPS_CANCLIPSTRETCHED),     \
    DDCAPDEF(L"DDCAPS_CANBLTSYSMEM",              dwCaps, DDCAPS_CANBLTSYSMEM),         \
    DDCAPDEF(L"DDCAPS_ZBLTS",                     dwCaps, DDCAPS_ZBLTS),


#define GEN_BLTCAPS2(dwCaps2)                                                   \
    DDCAPDEF(L"DDCAPS2_CANDROPZ16BIT",             dwCaps2,DDCAPS2_CANDROPZ16BIT),       \
    DDCAPDEF(L"DDCAPS2_NOPAGELOCKREQUIRED",        dwCaps2,DDCAPS2_NOPAGELOCKREQUIRED),  \
    DDCAPDEF(L"DDCAPS2_CANFLIPODDEVEN",            dwCaps2,DDCAPS2_CANFLIPODDEVEN),      \
    DDCAPDEF(L"DDCAPS2_CANBOBHARDWARE",            dwCaps2,DDCAPS2_CANBOBHARDWARE),      \
    DDCAPDEF(L"DDCAPS2_WIDESURFACES",              dwCaps2,DDCAPS2_WIDESURFACES),

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
    CAPDEF GenCaps[] =
    {
        // GEN_CAPS(dwCaps)
        DDCAPDEF(L"DDCAPS_3D",                        dwCaps, DDCAPS_3D),
        DDCAPDEF(L"DDCAPS_ALIGNBOUNDARYDEST",         dwCaps, DDCAPS_ALIGNBOUNDARYDEST),
        DDCAPDEF(L"DDCAPS_ALIGNSIZEDEST",             dwCaps, DDCAPS_ALIGNSIZEDEST),
        DDCAPDEF(L"DDCAPS_ALIGNBOUNDARYSRC",          dwCaps, DDCAPS_ALIGNBOUNDARYSRC),
        DDCAPDEF(L"DDCAPS_ALIGNSIZESRC",              dwCaps, DDCAPS_ALIGNSIZESRC),
        DDCAPDEF(L"DDCAPS_ALIGNSTRIDE",               dwCaps, DDCAPS_ALIGNSTRIDE),
        DDCAPDEF(L"DDCAPS_GDI",                       dwCaps, DDCAPS_GDI),
        DDCAPDEF(L"DDCAPS_OVERLAY",                   dwCaps, DDCAPS_OVERLAY),
        DDCAPDEF(L"DDCAPS_OVERLAYCANTCLIP",           dwCaps, DDCAPS_OVERLAYCANTCLIP),
        DDCAPDEF(L"DDCAPS_OVERLAYFOURCC",             dwCaps, DDCAPS_OVERLAYFOURCC),
        DDCAPDEF(L"DDCAPS_OVERLAYSTRETCH",            dwCaps, DDCAPS_OVERLAYSTRETCH),
        DDCAPDEF(L"DDCAPS_PALETTE",                   dwCaps, DDCAPS_PALETTE),
        DDCAPDEF(L"DDCAPS_PALETTEVSYNC",              dwCaps, DDCAPS_PALETTEVSYNC),
        DDCAPDEF(L"DDCAPS_READSCANLINE",              dwCaps, DDCAPS_READSCANLINE),
        DDCAPDEF(L"DDCAPS_VBI",                       dwCaps, DDCAPS_VBI),
        DDCAPDEF(L"DDCAPS_ZOVERLAYS",                 dwCaps, DDCAPS_ZOVERLAYS),
        DDCAPDEF(L"DDCAPS_NOHARDWARE",                dwCaps, DDCAPS_NOHARDWARE),
        DDCAPDEF(L"DDCAPS_BANKSWITCHED",              dwCaps, DDCAPS_BANKSWITCHED),

        // GEN_CAPS2(dwCaps2)
        DDCAPDEF(L"DDCAPS2_CERTIFIED",                 dwCaps2,DDCAPS2_CERTIFIED),
        DDCAPDEF(L"DDCAPS2_NO2DDURING3DSCENE",         dwCaps2,DDCAPS2_NO2DDURING3DSCENE),
        DDCAPDEF(L"DDCAPS2_VIDEOPORT",                 dwCaps2,DDCAPS2_VIDEOPORT),
        DDCAPDEF(L"DDCAPS2_AUTOFLIPOVERLAY",           dwCaps2,DDCAPS2_AUTOFLIPOVERLAY),
        DDCAPDEF(L"DDCAPS2_CANBOBINTERLEAVED",         dwCaps2,DDCAPS2_CANBOBINTERLEAVED),
        DDCAPDEF(L"DDCAPS2_CANBOBNONINTERLEAVED",      dwCaps2,DDCAPS2_CANBOBNONINTERLEAVED),
        DDCAPDEF(L"DDCAPS2_COLORCONTROLOVERLAY",       dwCaps2,DDCAPS2_COLORCONTROLOVERLAY),
        DDCAPDEF(L"DDCAPS2_COLORCONTROLPRIMARY",       dwCaps2,DDCAPS2_COLORCONTROLPRIMARY),
        DDCAPDEF(L"DDCAPS2_NONLOCALVIDMEM",            dwCaps2,DDCAPS2_NONLOCALVIDMEM),
        DDCAPDEF(L"DDCAPS2_NONLOCALVIDMEMCAPS",        dwCaps2,DDCAPS2_NONLOCALVIDMEMCAPS),
        DDCAPDEF(L"DDCAPS2_WIDESURFACES",              dwCaps2,DDCAPS2_WIDESURFACES),
        DDCAPDEF(L"DDCAPS2_NOPAGELOCKREQUIRED",        dwCaps2,DDCAPS2_NOPAGELOCKREQUIRED),
        DDCAPDEF(L"DDCAPS2_CANRENDERWINDOWED",         dwCaps2,DDCAPS2_CANRENDERWINDOWED),
        DDCAPDEF(L"DDCAPS2_COPYFOURCC",                dwCaps2,DDCAPS2_COPYFOURCC),
        DDCAPDEF(L"DDCAPS2_PRIMARYGAMMA",              dwCaps2,DDCAPS2_PRIMARYGAMMA),
        DDCAPDEF(L"DDCAPS2_CANCALIBRATEGAMMA",         dwCaps2,DDCAPS2_CANCALIBRATEGAMMA),
        DDCAPDEF(L"DDCAPS2_FLIPINTERVAL",              dwCaps2,DDCAPS2_FLIPINTERVAL),
        DDCAPDEF(L"DDCAPS2_FLIPNOVSYNC",               dwCaps2,DDCAPS2_FLIPNOVSYNC),
        DDCAPDEF(L"DDCAPS2_CANMANAGETEXTURE",          dwCaps2,DDCAPS2_CANMANAGETEXTURE),
        DDCAPDEF(L"DDCAPS2_TEXMANINNONLOCALVIDMEM",    dwCaps2,DDCAPS2_TEXMANINNONLOCALVIDMEM),
        DDCAPDEF(L"DDCAPS2_STEREO",                    dwCaps2,DDCAPS2_STEREO),
        DDCAPDEFex(L"DDCAPS2_CANSHARERESOURCE",        dwCaps2,DDCAPS2_CANSHARERESOURCE),

        { L"", 0, 0 }
    };


    //
    // NOTE GenCaps and CapsDefs are the same
    // because some are blt caps that apply to VRAM->VRAM blts and
    // some are "general" caps...
    //
    CAPDEF CapsDefs[] =
    {
        GEN_BLTCAPS(dwCaps)
        GEN_BLTCAPS2(dwCaps2)
        { L"", 0, 0 }
    };
    CAPDEF SVBCapsDefs[] =
    {
        GEN_BLTCAPS(dwSVBCaps)
        GEN_BLTCAPS2(dwSVBCaps2)
        { L"", 0, 0 }
    };
    CAPDEF VSBCapsDefs[] =
    {
        GEN_BLTCAPS(dwVSBCaps)
        //  GEN_BLTCAPS2(dwVSBCaps2)
        { L"", 0, 0 }
    };
    CAPDEF SSBCapsDefs[] =
    {
        GEN_BLTCAPS(dwSSBCaps)
        //  GEN_BLTCAPS2(dwSSBCaps2)
        { L"", 0, 0 }
    };
    CAPDEF NLVBCapsDefs[] =
    {
        GEN_BLTCAPS(dwNLVBCaps)
        GEN_BLTCAPS2(dwNLVBCaps2)
        { L"", 0, 0 }
    };

    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
#define GEN_CKCAPS(dwCKeyCaps)                                                              \
    DDCAPDEF(L"DDCKEYCAPS_DESTBLT",                 dwCKeyCaps, DDCKEYCAPS_DESTBLT),                  \
    DDCAPDEF(L"DDCKEYCAPS_DESTBLTCLRSPACE",         dwCKeyCaps, DDCKEYCAPS_DESTBLTCLRSPACE),          \
    DDCAPDEF(L"DDCKEYCAPS_DESTBLTCLRSPACEYUV",      dwCKeyCaps, DDCKEYCAPS_DESTBLTCLRSPACEYUV),       \
    DDCAPDEF(L"DDCKEYCAPS_DESTBLTYUV",              dwCKeyCaps, DDCKEYCAPS_DESTBLTYUV),               \
    DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAY",             dwCKeyCaps, DDCKEYCAPS_DESTOVERLAY),              \
    DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYCLRSPACE",     dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACE),      \
    DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV",  dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV),   \
    DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYONEACTIVE",    dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYONEACTIVE),     \
    DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYYUV",          dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYYUV),           \
    DDCAPDEF(L"DDCKEYCAPS_NOCOSTOVERLAY",           dwCKeyCaps, DDCKEYCAPS_NOCOSTOVERLAY),            \
    DDCAPDEF(L"DDCKEYCAPS_SRCBLT",                  dwCKeyCaps, DDCKEYCAPS_SRCBLT),                   \
    DDCAPDEF(L"DDCKEYCAPS_SRCBLTCLRSPACE",          dwCKeyCaps, DDCKEYCAPS_SRCBLTCLRSPACE),           \
    DDCAPDEF(L"DDCKEYCAPS_SRCBLTCLRSPACEYUV",       dwCKeyCaps, DDCKEYCAPS_SRCBLTCLRSPACEYUV),        \
    DDCAPDEF(L"DDCKEYCAPS_SRCBLTYUV",               dwCKeyCaps, DDCKEYCAPS_SRCBLTYUV),                \
    DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAY",              dwCKeyCaps, DDCKEYCAPS_SRCOVERLAY),               \
    DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYCLRSPACE",      dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACE),       \
    DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV",   dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV),    \
    DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYONEACTIVE",     dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYONEACTIVE),      \
    DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYYUV",           dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYYUV),

    CAPDEF CKeyCapsDefs[] =
    {
        GEN_CKCAPS(dwCKeyCaps)
        { L"", 0, 0 }
    };
    CAPDEF SVBCKeyCapsDefs[] =
    {
        GEN_CKCAPS(dwSVBCKeyCaps)
        { L"", 0, 0 }
    };
    CAPDEF SSBCKeyCapsDefs[] =
    {
        GEN_CKCAPS(dwSSBCKeyCaps)
        { L"", 0, 0 }
    };
    CAPDEF NLVBCKeyCapsDefs[] =
    {
        GEN_CKCAPS(dwNLVBCKeyCaps)
        { L"", 0, 0 }
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
#define GEN_FXCAPS(dwFXCaps)                                                            \
    DDCAPDEF(L"DDFXCAPS_BLTALPHA",                  dwFXCaps, DDFXCAPS_BLTALPHA),                 \
    DDCAPDEF(L"DDFXCAPS_BLTARITHSTRETCHY",          dwFXCaps, DDFXCAPS_BLTARITHSTRETCHY),         \
    DDCAPDEF(L"DDFXCAPS_BLTARITHSTRETCHYN",         dwFXCaps, DDFXCAPS_BLTARITHSTRETCHYN),        \
    DDCAPDEF(L"DDFXCAPS_BLTFILTER",                 dwFXCaps, DDFXCAPS_BLTFILTER),                \
    DDCAPDEF(L"DDFXCAPS_BLTMIRRORLEFTRIGHT",        dwFXCaps, DDFXCAPS_BLTMIRRORLEFTRIGHT),       \
    DDCAPDEF(L"DDFXCAPS_BLTMIRRORUPDOWN",           dwFXCaps, DDFXCAPS_BLTMIRRORUPDOWN),          \
    DDCAPDEF(L"DDFXCAPS_BLTROTATION",               dwFXCaps, DDFXCAPS_BLTROTATION),              \
    DDCAPDEF(L"DDFXCAPS_BLTROTATION90",             dwFXCaps, DDFXCAPS_BLTROTATION90),            \
    DDCAPDEF(L"DDFXCAPS_BLTSHRINKX",                dwFXCaps, DDFXCAPS_BLTSHRINKX),               \
    DDCAPDEF(L"DDFXCAPS_BLTSHRINKXN",               dwFXCaps, DDFXCAPS_BLTSHRINKXN),              \
    DDCAPDEF(L"DDFXCAPS_BLTSHRINKY",                dwFXCaps, DDFXCAPS_BLTSHRINKY),               \
    DDCAPDEF(L"DDFXCAPS_BLTSHRINKYN",               dwFXCaps, DDFXCAPS_BLTSHRINKYN),              \
    DDCAPDEF(L"DDFXCAPS_BLTSTRETCHX",               dwFXCaps, DDFXCAPS_BLTSTRETCHX),              \
    DDCAPDEF(L"DDFXCAPS_BLTSTRETCHXN",              dwFXCaps, DDFXCAPS_BLTSTRETCHXN),             \
    DDCAPDEF(L"DDFXCAPS_BLTSTRETCHY",               dwFXCaps, DDFXCAPS_BLTSTRETCHY),              \
    DDCAPDEF(L"DDFXCAPS_BLTSTRETCHYN",              dwFXCaps, DDFXCAPS_BLTSTRETCHYN),             \
    DDCAPDEF(L"DDFXCAPS_OVERLAYALPHA",              dwFXCaps, DDFXCAPS_OVERLAYALPHA),             \
    DDCAPDEF(L"DDFXCAPS_OVERLAYARITHSTRETCHY",      dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHY),     \
    DDCAPDEF(L"DDFXCAPS_OVERLAYARITHSTRETCHYN",     dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHYN),    \
    DDCAPDEF(L"DDFXCAPS_OVERLAYFILTER",             dwFXCaps, DDFXCAPS_OVERLAYFILTER),            \
    DDCAPDEF(L"DDFXCAPS_OVERLAYMIRRORLEFTRIGHT",    dwFXCaps, DDFXCAPS_OVERLAYMIRRORLEFTRIGHT),   \
    DDCAPDEF(L"DDFXCAPS_OVERLAYMIRRORUPDOWN",       dwFXCaps, DDFXCAPS_OVERLAYMIRRORUPDOWN),      \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKX",            dwFXCaps, DDFXCAPS_OVERLAYSHRINKX),           \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKXN",           dwFXCaps, DDFXCAPS_OVERLAYSHRINKXN),          \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKY",            dwFXCaps, DDFXCAPS_OVERLAYSHRINKY),           \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKYN",           dwFXCaps, DDFXCAPS_OVERLAYSHRINKYN),          \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHX",           dwFXCaps, DDFXCAPS_OVERLAYSTRETCHX),          \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHXN",          dwFXCaps, DDFXCAPS_OVERLAYSTRETCHXN),         \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHY",           dwFXCaps, DDFXCAPS_OVERLAYSTRETCHY),          \
    DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHYN",          dwFXCaps, DDFXCAPS_OVERLAYSTRETCHYN),

    CAPDEF FXCapsDefs[] =
    {
        GEN_FXCAPS(dwFXCaps)
        { L"", 0, 0 }
    };
    CAPDEF SVBFXCapsDefs[] =
    {
        GEN_FXCAPS(dwSVBFXCaps)
        { L"", 0, 0 }
    };
    CAPDEF SSBFXCapsDefs[] =
    {
        GEN_FXCAPS(dwSSBFXCaps)
        { L"", 0, 0 }
    };
    CAPDEF NLVBFXCapsDefs[] =
    {
        GEN_FXCAPS(dwNLVBFXCaps)
        { L"", 0, 0 }
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
#define GEN_FXALPHACAPS(dwFXAlphaCaps)                                                          \
    DDCAPDEF(L"DDFXALPHACAPS_BLTALPHAEDGEBLEND",           dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHAEDGEBLEND),    \
    DDCAPDEF(L"DDFXALPHACAPS_BLTALPHAPIXELS",              dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHAPIXELS),       \
    DDCAPDEF(L"DDFXALPHACAPS_BLTALPHAPIXELSNEG",           dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHAPIXELSNEG),    \
    DDCAPDEF(L"DDFXALPHACAPS_BLTALPHASURFACES",            dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHASURFACES),     \
    DDCAPDEF(L"DDFXALPHACAPS_BLTALPHASURFACESNEG",         dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHASURFACESNEG),  \
    DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND",       dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND),\
    DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHAPIXELS",          dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELS),   \
    DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG",       dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG),\
    DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHASURFACES",        dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACES), \
    DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHASURFACESNEG",     dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACESNEG),

    CAPDEF FXAlphaCapsDefs[] =
    {
        GEN_FXALPHACAPS(dwFXAlphaCaps)
        { L"", 0, 0 }
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF PalCapsDefs[] =
    {
        DDCAPDEF(L"DDPCAPS_1BIT",              dwPalCaps, DDPCAPS_1BIT),
        DDCAPDEF(L"DDPCAPS_2BIT",              dwPalCaps, DDPCAPS_2BIT),
        DDCAPDEF(L"DDPCAPS_4BIT",              dwPalCaps, DDPCAPS_4BIT),
        DDCAPDEF(L"DDPCAPS_8BITENTRIES",       dwPalCaps, DDPCAPS_8BITENTRIES),
        DDCAPDEF(L"DDPCAPS_8BIT",              dwPalCaps, DDPCAPS_8BIT),
        DDCAPDEF(L"DDPCAPS_ALLOW256",          dwPalCaps, DDPCAPS_ALLOW256),
        DDCAPDEF(L"DDPCAPS_ALPHA",             dwPalCaps, DDPCAPS_ALPHA),
        DDCAPDEF(L"DDPCAPS_PRIMARYSURFACE",    dwPalCaps, DDPCAPS_PRIMARYSURFACE),
        DDCAPDEF(L"DDPCAPS_PRIMARYSURFACELEFT",dwPalCaps, DDPCAPS_PRIMARYSURFACELEFT),
        DDCAPDEF(L"DDPCAPS_VSYNC",             dwPalCaps, DDPCAPS_VSYNC),
        { L"", 0, 0}
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF SurfCapsDefs[] =
    {
        DDCAPDEF(L"DDSCAPS_3DDEVICE",             ddsCaps.dwCaps,  DDSCAPS_3DDEVICE),
        DDCAPDEF(L"DDSCAPS_ALPHA",                ddsCaps.dwCaps,  DDSCAPS_ALPHA),
        DDCAPDEF(L"DDSCAPS_BACKBUFFER",           ddsCaps.dwCaps,  DDSCAPS_BACKBUFFER),
        DDCAPDEF(L"DDSCAPS_COMPLEX",              ddsCaps.dwCaps,  DDSCAPS_COMPLEX),
        DDCAPDEF(L"DDSCAPS_FLIP",                 ddsCaps.dwCaps,  DDSCAPS_FLIP),
        DDCAPDEF(L"DDSCAPS_FRONTBUFFER",          ddsCaps.dwCaps,  DDSCAPS_FRONTBUFFER),
        DDCAPDEF(L"DDSCAPS_HWCODEC",              ddsCaps.dwCaps,  DDSCAPS_HWCODEC),
        DDCAPDEF(L"DDSCAPS_LIVEVIDEO",            ddsCaps.dwCaps,  DDSCAPS_LIVEVIDEO),
        DDCAPDEF(L"DDSCAPS_MIPMAP",               ddsCaps.dwCaps,  DDSCAPS_MIPMAP),
        DDCAPDEF(L"DDSCAPS_MODEX",                ddsCaps.dwCaps,  DDSCAPS_MODEX),
        DDCAPDEF(L"DDSCAPS_OFFSCREENPLAIN",       ddsCaps.dwCaps,  DDSCAPS_OFFSCREENPLAIN),
        DDCAPDEF(L"DDSCAPS_OVERLAY",              ddsCaps.dwCaps,  DDSCAPS_OVERLAY),
        DDCAPDEF(L"DDSCAPS_OWNDC",                ddsCaps.dwCaps,  DDSCAPS_OWNDC),
        DDCAPDEF(L"DDSCAPS_PALETTE",              ddsCaps.dwCaps,  DDSCAPS_PALETTE),
        DDCAPDEF(L"DDSCAPS_PRIMARYSURFACE",       ddsCaps.dwCaps,  DDSCAPS_PRIMARYSURFACE),
        DDCAPDEF(L"DDSCAPS_SYSTEMMEMORY",         ddsCaps.dwCaps,  DDSCAPS_SYSTEMMEMORY),
        DDCAPDEF(L"DDSCAPS_TEXTURE",              ddsCaps.dwCaps,  DDSCAPS_TEXTURE),
        DDCAPDEF(L"DDSCAPS_VIDEOMEMORY",          ddsCaps.dwCaps,  DDSCAPS_VIDEOMEMORY),
        DDCAPDEF(L"DDSCAPS_VIDEOPORT",            ddsCaps.dwCaps,  DDSCAPS_VIDEOPORT),
        DDCAPDEF(L"DDSCAPS_VISIBLE",              ddsCaps.dwCaps,  DDSCAPS_VISIBLE),
        DDCAPDEF(L"DDSCAPS_WRITEONLY",            ddsCaps.dwCaps,  DDSCAPS_WRITEONLY),
        DDCAPDEF(L"DDSCAPS_ZBUFFER",              ddsCaps.dwCaps,  DDSCAPS_ZBUFFER),
        DDCAPDEF(L"DDSCAPS_ALLOCONLOAD",          ddsCaps.dwCaps,  DDSCAPS_ALLOCONLOAD),
        DDCAPDEF(L"DDSCAPS_LOCALVIDMEM",          ddsCaps.dwCaps,  DDSCAPS_LOCALVIDMEM),
        DDCAPDEF(L"DDSCAPS_NONLOCALVIDMEM",       ddsCaps.dwCaps,  DDSCAPS_NONLOCALVIDMEM),
        DDCAPDEF(L"DDSCAPS_STANDARDVGAMODE",      ddsCaps.dwCaps,  DDSCAPS_STANDARDVGAMODE),
        DDCAPDEF(L"DDSCAPS_OPTIMIZED",            ddsCaps.dwCaps,  DDSCAPS_OPTIMIZED),
        DDCAPDEF(L"DDSCAPS2_HARDWAREDEINTERLACE",  ddsCaps.dwCaps2, DDSCAPS2_HARDWAREDEINTERLACE),
        DDCAPDEF(L"DDSCAPS2_HINTDYNAMIC",          ddsCaps.dwCaps2, DDSCAPS2_HINTDYNAMIC),
        DDCAPDEF(L"DDSCAPS2_HINTSTATIC",           ddsCaps.dwCaps2, DDSCAPS2_HINTSTATIC),
        DDCAPDEF(L"DDSCAPS2_TEXTUREMANAGE",        ddsCaps.dwCaps2, DDSCAPS2_TEXTUREMANAGE),
        DDCAPDEF(L"DDSCAPS2_OPAQUE",               ddsCaps.dwCaps2, DDSCAPS2_OPAQUE),
        DDCAPDEF(L"DDSCAPS2_HINTANTIALIASING",     ddsCaps.dwCaps2, DDSCAPS2_HINTANTIALIASING),
        DDCAPDEF(L"DDSCAPS2_CUBEMAP",              ddsCaps.dwCaps2, DDSCAPS2_CUBEMAP),
        DDCAPDEF(L"DDSCAPS2_MIPMAPSUBLEVEL",       ddsCaps.dwCaps2, DDSCAPS2_MIPMAPSUBLEVEL),
        DDCAPDEF(L"DDSCAPS2_D3DTEXTUREMANAGE",     ddsCaps.dwCaps2, DDSCAPS2_D3DTEXTUREMANAGE),
        DDCAPDEF(L"DDSCAPS2_DONOTPERSIST",         ddsCaps.dwCaps2, DDSCAPS2_DONOTPERSIST),
        DDCAPDEF(L"DDSCAPS2_STEREOSURFACELEFT",    ddsCaps.dwCaps2, DDSCAPS2_STEREOSURFACELEFT),
        { L"", 0, 0}
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF SVisionCapsDefs[] =
    {
        DDCAPDEF(L"DDSVCAPS_STEREOSEQUENTIAL",  dwSVCaps, DDSVCAPS_STEREOSEQUENTIAL),
        { L"", 0, 0}
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
#define GEN_ROPS(dwRops)                        \
    ROPDEF(L"SRCCOPY",    dwRops, SRCCOPY),      \
    ROPDEF(L"SRCPAINT",   dwRops, SRCPAINT),     \
    ROPDEF(L"SRCAND",     dwRops, SRCAND),       \
    ROPDEF(L"SRCINVERT",  dwRops, SRCINVERT),    \
    ROPDEF(L"SRCERASE",   dwRops, SRCERASE),     \
    ROPDEF(L"NOTSRCCOPY", dwRops, NOTSRCCOPY),   \
    ROPDEF(L"NOTSRCERASE",dwRops, NOTSRCERASE),  \
    ROPDEF(L"MERGECOPY",  dwRops, MERGECOPY),    \
    ROPDEF(L"MERGEPAINT", dwRops, MERGEPAINT),   \
    ROPDEF(L"PATCOPY",    dwRops, PATCOPY),      \
    ROPDEF(L"PATPAINT",   dwRops, PATPAINT),     \
    ROPDEF(L"PATINVERT",  dwRops, PATINVERT),    \
    ROPDEF(L"DSTINVERT",  dwRops, DSTINVERT),    \
    ROPDEF(L"BLACKNESS",  dwRops, BLACKNESS),    \
    ROPDEF(L"WHITENESS",  dwRops, WHITENESS),

    CAPDEF ROPCapsDefs[] =
    {
        GEN_ROPS(dwRops)
        { L"", 0, 0 }
    };
    CAPDEF VSBROPCapsDefs[] =
    {
        GEN_ROPS(dwVSBRops)
        { L"", 0, 0 }
    };
    CAPDEF SVBROPCapsDefs[] =
    {
        GEN_ROPS(dwSVBRops)
        { L"", 0, 0 }
    };
    CAPDEF SSBROPCapsDefs[] =
    {
        GEN_ROPS(dwSSBRops)
        { L"", 0, 0 }
    };
    CAPDEF NLVBROPCapsDefs[] =
    {
        GEN_ROPS(dwNLVBRops)
        { L"", 0, 0 }
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF OverlayCapsDefs[] =
    {
        DDCAPDEF(L"DDCAPS_OVERLAY",                         dwCaps, DDCAPS_OVERLAY),

        DDCAPDEF(L"DDCAPS_ALIGNBOUNDARYDEST",               dwCaps, DDCAPS_ALIGNBOUNDARYDEST),
        DDVALDEF(L"  dwAlignBoundaryDest",             dwAlignBoundaryDest),
        DDCAPDEF(L"DDCAPS_ALIGNSIZEDEST",                   dwCaps, DDCAPS_ALIGNSIZEDEST),
        DDVALDEF(L"  dwAlignSizeDest",                 dwAlignSizeDest),
        DDCAPDEF(L"DDCAPS_ALIGNBOUNDARYSRC",                dwCaps, DDCAPS_ALIGNBOUNDARYSRC),
        DDVALDEF(L"  dwAlignBoundarySrc",              dwAlignBoundarySrc),
        DDCAPDEF(L"DDCAPS_ALIGNSIZESRC",                    dwCaps, DDCAPS_ALIGNSIZESRC),
        DDVALDEF(L"  dwAlignSizeSrc",                  dwAlignSizeSrc),
        DDCAPDEF(L"DDCAPS_ALIGNSTRIDE",                     dwCaps, DDCAPS_ALIGNSTRIDE),
        DDVALDEF(L"  dwAlignStrideAlign",              dwAlignStrideAlign),

        DDCAPDEF(L"DDCAPS_OVERLAYCANTCLIP",                 dwCaps, DDCAPS_OVERLAYCANTCLIP),
        DDCAPDEF(L"DDCAPS_OVERLAYFOURCC",                   dwCaps, DDCAPS_OVERLAYFOURCC),
        DDCAPDEF(L"DDCAPS_OVERLAYSTRETCH",                  dwCaps, DDCAPS_OVERLAYSTRETCH),
        DDCAPDEF(L"DDCAPS_ZOVERLAYS",                       dwCaps, DDCAPS_ZOVERLAYS),
        DDCAPDEF(L"DDCAPS2_AUTOFLIPOVERLAY",                 dwCaps2,DDCAPS2_AUTOFLIPOVERLAY),
        DDCAPDEF(L"DDCAPS2_CANBOBINTERLEAVED",               dwCaps2,DDCAPS2_CANBOBINTERLEAVED),
        DDCAPDEF(L"DDCAPS2_CANBOBNONINTERLEAVED",            dwCaps2,DDCAPS2_CANBOBNONINTERLEAVED),
        DDCAPDEF(L"DDCAPS2_COLORCONTROLOVERLAY",             dwCaps2,DDCAPS2_COLORCONTROLOVERLAY),
        DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAY",                     dwCKeyCaps, DDCKEYCAPS_DESTOVERLAY),
        DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYCLRSPACE",             dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACE),
        DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV",          dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV),
        DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYONEACTIVE",            dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYONEACTIVE),
        DDCAPDEF(L"DDCKEYCAPS_DESTOVERLAYYUV",                  dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYYUV),
        DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAY",                      dwCKeyCaps, DDCKEYCAPS_SRCOVERLAY),
        DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYCLRSPACE",              dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACE),
        DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV",           dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV),
        DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYONEACTIVE",             dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYONEACTIVE),
        DDCAPDEF(L"DDCKEYCAPS_SRCOVERLAYYUV",                   dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYYUV),
        DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND",           dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND),
        DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHAPIXELS",              dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELS),
        DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG",           dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG),
        DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHASURFACES",            dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACES),
        DDCAPDEF(L"DDFXALPHACAPS_OVERLAYALPHASURFACESNEG",         dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACESNEG),
        DDCAPDEF(L"DDFXCAPS_OVERLAYALPHA",                    dwFXCaps, DDFXCAPS_OVERLAYALPHA),
        DDCAPDEF(L"DDFXCAPS_OVERLAYARITHSTRETCHY",            dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHY),
        DDCAPDEF(L"DDFXCAPS_OVERLAYARITHSTRETCHYN",           dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHYN),
        DDCAPDEF(L"DDFXCAPS_OVERLAYFILTER",                   dwFXCaps, DDFXCAPS_OVERLAYFILTER),
        DDCAPDEF(L"DDFXCAPS_OVERLAYMIRRORLEFTRIGHT",          dwFXCaps, DDFXCAPS_OVERLAYMIRRORLEFTRIGHT),
        DDCAPDEF(L"DDFXCAPS_OVERLAYMIRRORUPDOWN",             dwFXCaps, DDFXCAPS_OVERLAYMIRRORUPDOWN),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKX",                  dwFXCaps, DDFXCAPS_OVERLAYSHRINKX),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKXN",                 dwFXCaps, DDFXCAPS_OVERLAYSHRINKXN),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKY",                  dwFXCaps, DDFXCAPS_OVERLAYSHRINKY),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSHRINKYN",                 dwFXCaps, DDFXCAPS_OVERLAYSHRINKYN),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHX",                 dwFXCaps, DDFXCAPS_OVERLAYSTRETCHX),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHXN",                dwFXCaps, DDFXCAPS_OVERLAYSTRETCHXN),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHY",                 dwFXCaps, DDFXCAPS_OVERLAYSTRETCHY),
        DDCAPDEF(L"DDFXCAPS_OVERLAYSTRETCHYN",                dwFXCaps, DDFXCAPS_OVERLAYSTRETCHYN),
        DDHEXDEF(L"dwAlphaOverlayConstBitDepths",      dwAlphaOverlayConstBitDepths),
        DDCAPDEF(L"  DDBD_8",                             dwAlphaOverlayConstBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                            dwAlphaOverlayConstBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                            dwAlphaOverlayConstBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                            dwAlphaOverlayConstBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaOverlayPixelBitDepths",      dwAlphaOverlayPixelBitDepths),
        DDCAPDEF(L"  DDBD_8",                             dwAlphaOverlayPixelBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                            dwAlphaOverlayPixelBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                            dwAlphaOverlayPixelBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                            dwAlphaOverlayPixelBitDepths, DDBD_32),
        DDHEXDEF(L"dwAlphaOverlaySurfaceBitDepths",    dwAlphaOverlaySurfaceBitDepths),
        DDCAPDEF(L"  DDBD_8",                             dwAlphaOverlaySurfaceBitDepths, DDBD_8),
        DDCAPDEF(L"  DDBD_16",                            dwAlphaOverlaySurfaceBitDepths, DDBD_16),
        DDCAPDEF(L"  DDBD_24",                            dwAlphaOverlaySurfaceBitDepths, DDBD_24),
        DDCAPDEF(L"  DDBD_32",                            dwAlphaOverlaySurfaceBitDepths, DDBD_32),
        DDVALDEF(L"dwMaxVisibleOverlays",              dwMaxVisibleOverlays),
        DDVALDEF(L"dwCurrVisibleOverlays",             dwCurrVisibleOverlays),
        DDVALDEF(L"dwMinOverlayStretch",               dwMinOverlayStretch),
        DDVALDEF(L"dwMaxOverlayStretch",               dwMaxOverlayStretch),
        { L"", 0, 0}
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEF VideoPortCapsDefs[] =
    {
        DDCAPDEF(L"DDCAPS2_VIDEOPORT",               dwCaps2,DDCAPS2_VIDEOPORT),
        DDCAPDEF(L"DDSCAPS_VIDEOPORT",               ddsCaps.dwCaps, DDSCAPS_VIDEOPORT),
        DDCAPDEF(L"DDSCAPS_LIVEVIDEO",                       ddsCaps.dwCaps, DDSCAPS_LIVEVIDEO),
        DDCAPDEF(L"DDSCAPS2_HARDWAREDEINTERLACE",             dwCaps2,DDSCAPS2_HARDWAREDEINTERLACE),
        DDCAPDEF(L"DDSCAPS2_AUTOFLIPOVERLAY",                 dwCaps2,DDCAPS2_AUTOFLIPOVERLAY),
        DDVALDEF(L"dwMinLiveVideoStretch",             dwMinLiveVideoStretch),
        DDVALDEF(L"dwMaxLiveVideoStretch",             dwMaxLiveVideoStretch),
        DDVALDEF(L"dwMaxVideoPorts",                   dwMaxVideoPorts),
        DDVALDEF(L"dwCurrVideoPorts",                  dwCurrVideoPorts),
        { L"", 0, 0}
    };


    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    CAPDEFS DDCapDefs[] =
    {
        {L"",                    nullptr,            (LPARAM)0,                  },
        {L"Memory",              DDDisplayVidMem,    (LPARAM)0,                  },

        {L"+Caps",               nullptr,            (LPARAM)0,                  },
        {L"General",             DDDisplayCaps,      (LPARAM)GenCaps,            },
        {L"FX Alpha Caps",       DDDisplayCaps,      (LPARAM)FXAlphaCapsDefs,    },
        {L"Palette Caps",        DDDisplayCaps,      (LPARAM)PalCapsDefs,        },
        {L"Overlay Caps",        DDDisplayCaps,      (LPARAM)OverlayCapsDefs,    },
        {L"Surface Caps",        DDDisplayCaps,      (LPARAM)SurfCapsDefs,       },
        {L"Stereo Vision Caps",  DDDisplayCaps,      (LPARAM)SVisionCapsDefs,    },
        {L"Video Port Caps",     DDDisplayCaps,      (LPARAM)VideoPortCapsDefs,  },

        {L"+BLT Caps",           nullptr,            (LPARAM)0,                  },

        {L"+Video - Video",      nullptr,            (LPARAM)0,                  },
        {L"General",             DDDisplayCaps,      (LPARAM)CapsDefs,           },
        {L"Color Key",           DDDisplayCaps,      (LPARAM)CKeyCapsDefs,       },
        {L"FX",                  DDDisplayCaps,      (LPARAM)FXCapsDefs,         },
        {L"ROPS",                DDDisplayCaps,      (LPARAM)ROPCapsDefs,        },
        {L"-",                   nullptr,            (LPARAM)0,                  },

        {L"+System - Video",     nullptr,            (LPARAM)0,                  },
        {L"General",             DDDisplayCaps,      (LPARAM)SVBCapsDefs,        },
        {L"Color Key",           DDDisplayCaps,      (LPARAM)SVBCKeyCapsDefs,    },
        {L"FX",                  DDDisplayCaps,      (LPARAM)SVBFXCapsDefs,      },
        {L"ROPS",                DDDisplayCaps,      (LPARAM)SVBROPCapsDefs,     },
        {L"-",                   nullptr,            (LPARAM)0,                  },

        {L"+Video - System",     nullptr,            (LPARAM)0,                  },
        {L"General",             DDDisplayCaps,      (LPARAM)VSBCapsDefs,        },
        {L"Color Key",           DDDisplayCaps,      (LPARAM)SSBCKeyCapsDefs,    },
        {L"FX",                  DDDisplayCaps,      (LPARAM)SSBFXCapsDefs,      },
        {L"ROPS",                DDDisplayCaps,      (LPARAM)VSBROPCapsDefs,     },
        {L"-",                   nullptr,            (LPARAM)0,                  },

        {L"+System - System",    nullptr,            (LPARAM)0,                  },
        {L"General",             DDDisplayCaps,      (LPARAM)SSBCapsDefs,        },
        {L"Color Key",           DDDisplayCaps,      (LPARAM)SSBCKeyCapsDefs,    },
        {L"FX",                  DDDisplayCaps,      (LPARAM)SSBFXCapsDefs,      },
        {L"ROPS",                DDDisplayCaps,      (LPARAM)SSBROPCapsDefs,     },
        {L"-",                   nullptr,            (LPARAM)0,                  },

        {L"+NonLocal - Video",   nullptr,            (LPARAM)0,                  },
        {L"General",             DDDisplayCaps,      (LPARAM)NLVBCapsDefs,       },
        {L"Color Key",           DDDisplayCaps,      (LPARAM)NLVBCKeyCapsDefs,   },
        {L"FX",                  DDDisplayCaps,      (LPARAM)NLVBFXCapsDefs,     },
        {L"ROPS",                DDDisplayCaps,      (LPARAM)NLVBROPCapsDefs,    },
        {L"-",                   nullptr,            (LPARAM)0,                  },

        {L"-",                   nullptr,            (LPARAM)0,                  },
        {L"-",                   nullptr,            (LPARAM)0,                  },

        {L"Video Modes",         DDDisplayVideoModes,(LPARAM)0,                  },
        {L"FourCC Formats",      DDDisplayFourCCFormat,(LPARAM)0,                },
        {L"Other",               DDDisplayCaps,      (LPARAM)OtherInfoDefs,      },

        { nullptr, 0, 0 }
    };


    //-----------------------------------------------------------------------------
    HRESULT DDCreate(GUID* pGUID)
    {
        if (pGUID == (GUID*)-2)
            return E_FAIL;

        if (g_pDD && pGUID == g_pDDGUID)
            return S_OK;

        SAFE_RELEASE(g_pDD)

        // There is no need to create DirectDraw emulation-only just to get
        // the HEL caps.  In fact, this will fail if there is another DirectDraw
        // app running and using the hardware.
        if (pGUID == (GUID*)DDCREATE_EMULATIONONLY)
            pGUID = nullptr;

        if (!g_directDrawCreateEx)
            return E_FAIL;

        if (SUCCEEDED(g_directDrawCreateEx(pGUID, (VOID**)&g_pDD, IID_IDirectDraw7, nullptr)))
        {
            g_pDDGUID = pGUID;
            return S_OK;
        }

        return E_FAIL;
    }


    //-----------------------------------------------------------------------------
    HRESULT DDDisplayVidMem(LPARAM lParam1, LPARAM /*lParam2*/, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        if (SUCCEEDED(DDCreate((GUID*)lParam1)))
        {
            DWORD dwTotalVidMem = 0, dwFreeVidMem = 0;
            DWORD dwTotalLocMem = 0, dwFreeLocMem = 0;
            DWORD dwTotalAGPMem = 0, dwFreeAGPMem = 0;
            DWORD dwTotalTexMem = 0, dwFreeTexMem = 0;

            DDSCAPS2 ddsCaps2 = {};

            ddsCaps2.dwCaps = DDSCAPS_VIDEOMEMORY;
            HRESULT hr = g_pDD->GetAvailableVidMem(&ddsCaps2, &dwTotalVidMem, &dwFreeVidMem);
            if (FAILED(hr))
            {
                dwTotalVidMem = 0;
                dwFreeVidMem = 0;
            }

            ddsCaps2.dwCaps = DDSCAPS_LOCALVIDMEM;
            hr = g_pDD->GetAvailableVidMem(&ddsCaps2, &dwTotalLocMem, &dwFreeLocMem);
            if (FAILED(hr))
            {
                dwTotalLocMem = 0;
                dwFreeLocMem = 0;
            }

            ddsCaps2.dwCaps = DDSCAPS_NONLOCALVIDMEM;
            hr = g_pDD->GetAvailableVidMem(&ddsCaps2, &dwTotalAGPMem, &dwFreeAGPMem);
            if (FAILED(hr))
            {
                dwTotalAGPMem = 0;
                dwFreeAGPMem = 0;
            }

            ddsCaps2.dwCaps = DDSCAPS_TEXTURE;
            hr = g_pDD->GetAvailableVidMem(&ddsCaps2, &dwTotalTexMem, &dwFreeTexMem);
            if (FAILED(hr))
            {
                dwTotalTexMem = 0;
                dwFreeTexMem = 0;
            }

            if (pPrintInfo)
            {
                PrintValueLine(L"dwTotalVidMem", dwTotalVidMem, pPrintInfo);
                PrintValueLine(L"dwFreeVidMem", dwFreeVidMem, pPrintInfo);
                PrintValueLine(L"dwTotalLocMem", dwTotalLocMem, pPrintInfo);
                PrintValueLine(L"dwFreeLocMem", dwFreeLocMem, pPrintInfo);
                PrintValueLine(L"dwTotalAGPMem", dwTotalAGPMem, pPrintInfo);
                PrintValueLine(L"dwFreeAGPMem", dwFreeAGPMem, pPrintInfo);
                PrintValueLine(L"dwTotalTexMem", dwTotalTexMem, pPrintInfo);
                PrintValueLine(L"dwFreeTexMem", dwFreeTexMem, pPrintInfo);
            }
            else
            {
                WCHAR strBuff[64];

                LVAddColumn(g_hwndLV, 0, L"Type", 24);
                LVAddColumn(g_hwndLV, 1, L"Total", 10);
                LVAddColumn(g_hwndLV, 2, L"Free", 10);

                LVAddText(g_hwndLV, 0, L"Video");
                Int2Str(strBuff, 64, dwTotalVidMem);
                LVAddText(g_hwndLV, 1, L"%s", strBuff);
                Int2Str(strBuff, 64, dwFreeVidMem);
                LVAddText(g_hwndLV, 2, L"%s", strBuff);

                LVAddText(g_hwndLV, 0, L"Video (local)");
                Int2Str(strBuff, 64, dwTotalLocMem);
                LVAddText(g_hwndLV, 1, L"%s", strBuff);
                Int2Str(strBuff, 64, dwFreeLocMem);
                LVAddText(g_hwndLV, 2, L"%s", strBuff);

                LVAddText(g_hwndLV, 0, L"Video (non-local)");
                Int2Str(strBuff, 64, dwTotalAGPMem);
                LVAddText(g_hwndLV, 1, L"%s", strBuff);
                Int2Str(strBuff, 64, dwFreeAGPMem);
                LVAddText(g_hwndLV, 2, L"%s", strBuff);

                LVAddText(g_hwndLV, 0, L"Texture");
                Int2Str(strBuff, 64, dwTotalTexMem);
                LVAddText(g_hwndLV, 1, L"%s", strBuff);
                Int2Str(strBuff, 64, dwFreeTexMem);
                LVAddText(g_hwndLV, 2, L"%s", strBuff);
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    HRESULT DDDisplayCaps(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        // lParam1 is the GUID for the driver we should open
        // lParam2 is the CAPDEF table we should use
        if (SUCCEEDED(DDCreate((GUID*)lParam1)))
        {
            DDCAPS ddcaps = {};
            ddcaps.dwSize = sizeof(ddcaps);

            HRESULT hr;
            if (lParam1 == DDCREATE_EMULATIONONLY)
                hr = g_pDD->GetCaps(nullptr, &ddcaps);
            else
                hr = g_pDD->GetCaps(&ddcaps, nullptr);
            if (FAILED(hr))
            {
                ddcaps = {};
            }

            if (pPrintInfo)
                return PrintCapsToDC((CAPDEF*)lParam2, (VOID*)&ddcaps, pPrintInfo);
            else
                AddCapsToLV((CAPDEF*)lParam2, (LPVOID)&ddcaps);
        }

        // Keep printing, even if an error occurred
        return S_OK;
    }


    //-----------------------------------------------------------------------------
    HRESULT DDDisplayFourCCFormat(LPARAM /*lParam1*/, LPARAM /*lParam2*/,
        _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        if (!g_pDD)
            return S_OK;

        DWORD dwNumOfCodes;
        HRESULT hr = g_pDD->GetFourCCCodes(&dwNumOfCodes, nullptr);
        if (FAILED(hr))
            return E_FAIL;

        auto FourCC = static_cast<DWORD*>(GlobalAlloc(GPTR, (sizeof(DWORD) * dwNumOfCodes)));
        if (!FourCC)
            return E_OUTOFMEMORY;

        hr = g_pDD->GetFourCCCodes(&dwNumOfCodes, FourCC);
        if (FAILED(hr))
            return E_FAIL;

        // Add columns
        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Codes", 24);
            LVAddColumn(g_hwndLV, 1, L"", 24);
        }

        // Assume all FourCC values are ascii strings
        for (DWORD dwCount = 0; dwCount < dwNumOfCodes; dwCount++)
        {
            CHAR strText[5] = {};
            memcpy(strText, &FourCC[dwCount], 4);
            WCHAR szText[5] = {};
            szText[0] = strText[0];
            szText[1] = strText[1];
            szText[2] = strText[2];
            szText[3] = strText[3];

            if (!pPrintInfo)
            {
                LVAddText(g_hwndLV, 0, L"%s", szText);
            }
            else
            {
                int xCode = (pPrintInfo->dwCurrIndent * DEF_TAB_SIZE * pPrintInfo->dwCharWidth);
                int yLine = (pPrintInfo->dwCurrLine * pPrintInfo->dwLineHeight);

                // Print Code
                if (FAILED(PrintLine(xCode, yLine, szText, 4, pPrintInfo)))
                    return E_FAIL;

                if (FAILED(PrintNextLine(pPrintInfo)))
                    return E_FAIL;
            }
        }

        GlobalFree(FourCC);

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    HRESULT CALLBACK EnumDisplayModesCallback(DDSURFACEDESC2* pddsd, VOID*)
    {
        if (pddsd->ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE)
        {
            LVAddText(g_hwndLV, 0, L"%dx%dx%d (StandardVGA)",
                pddsd->dwWidth, pddsd->dwHeight,
                pddsd->ddpfPixelFormat.dwRGBBitCount);
        }
        else if (pddsd->ddsCaps.dwCaps & DDSCAPS_MODEX)
        {
            LVAddText(g_hwndLV, 0, L"%dx%dx%d (ModeX)",
                pddsd->dwWidth, pddsd->dwHeight,
                pddsd->ddpfPixelFormat.dwRGBBitCount);
        }
        else
        {
            LVAddText(g_hwndLV, 0, L"%dx%dx%d ",
                pddsd->dwWidth, pddsd->dwHeight,
                pddsd->ddpfPixelFormat.dwRGBBitCount);
        }

        return DDENUMRET_OK;
    }


    //-----------------------------------------------------------------------------
    HRESULT CALLBACK EnumDisplayModesCallbackPrint(DDSURFACEDESC2* pddsd, VOID* Context)
    {
        WCHAR szBuff[80];
        DWORD cchLen;
        auto lpInfo = reinterpret_cast<PRINTCBINFO*>(Context);
        int xMode, yLine;

        if (!lpInfo)
            return E_FAIL;

        xMode = (lpInfo->dwCurrIndent * DEF_TAB_SIZE * lpInfo->dwCharWidth);
        yLine = (lpInfo->dwCurrLine * lpInfo->dwLineHeight);

        if (pddsd->ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE)
        {
            swprintf_s(szBuff, 80, L"%ux%ux%u (StandardVGA)", pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
        }
        else if (pddsd->ddsCaps.dwCaps & DDSCAPS_MODEX)
        {
            swprintf_s(szBuff, 80, L"%ux%ux%u (ModeX)", pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
        }
        else
        {
            swprintf_s(szBuff, 80, L"%ux%ux%u ", pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
        }
        // Print Mode Info
        cchLen = static_cast<DWORD>(wcslen(szBuff));
        if (FAILED(PrintLine(xMode, yLine, szBuff, cchLen, lpInfo)))
            return DDENUMRET_CANCEL;
        // Advance to next line
        if (FAILED(PrintNextLine(lpInfo)))
            return DDENUMRET_CANCEL;

        return DDENUMRET_OK;
    }


    //-----------------------------------------------------------------------------
    HRESULT DDDisplayVideoModes(LPARAM lParam1, LPARAM /*lParam2*/,
        _In_opt_ PRINTCBINFO* pPrintInfo)
    {
        DWORD          mode;
        DDSURFACEDESC2 ddsd;

        if (!pPrintInfo)
        {
            LVAddColumn(g_hwndLV, 0, L"Mode", 24);
            LVAddColumn(g_hwndLV, 1, L"", 24);
        }

        // lParam1 is the GUID for the driver we should open
        // lParam2 is not used

        if (SUCCEEDED(DDCreate((GUID*)lParam1)))
        {
            // Get the current mode mode for this driver
            ddsd.dwSize = sizeof(DDSURFACEDESC2);
            HRESULT hr = g_pDD->GetDisplayMode(&ddsd);
            if (SUCCEEDED(hr))
            {
                mode = MAKEMODE(ddsd.dwWidth, ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount);

                // Get Mode with ModeX
                g_pDD->SetCooperativeLevel(g_hwndMain, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE |
                    DDSCL_ALLOWMODEX | DDSCL_NOWINDOWCHANGES);

                if (pPrintInfo)
                    g_pDD->EnumDisplayModes(DDEDM_STANDARDVGAMODES, nullptr, (VOID*)pPrintInfo,
                        EnumDisplayModesCallbackPrint);
                else
                    g_pDD->EnumDisplayModes(DDEDM_STANDARDVGAMODES, nullptr, UIntToPtr(mode),
                        EnumDisplayModesCallback);

                g_pDD->SetCooperativeLevel(g_hwndMain, DDSCL_NORMAL);
            }
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    BOOL CALLBACK DDEnumCallBack(_In_ GUID* pid, _In_z_ LPSTR lpDriverDesc,
        _In_opt_ LPSTR lpDriverName, _In_opt_ VOID* lpContext, _In_opt_ HMONITOR)
    {
        HTREEITEM hParent = (HTREEITEM)lpContext;
        char szText[256];

        if (pid != (GUID*)-2)
            if (HIWORD(pid) != 0)
            {
                GUID temp = *pid;
                pid = (GUID*)LocalAlloc(LPTR, sizeof(GUID));
                if (pid)
                    *pid = temp;
            }

        // Add subnode to treeview
        if (lpDriverName && *lpDriverName)
            sprintf_s(szText, 256, "%s (%s)", lpDriverDesc, lpDriverName);
        else
            strcpy_s(szText, 256, lpDriverDesc);
        szText[255] = '\0';

        WideString strText(szText);
        DDCapDefs[0].strName = strText.c_str();
        AddCapsToTV(hParent, DDCapDefs, (LPARAM)pid);

        return(DDENUMRET_OK);
    }
}

//-----------------------------------------------------------------------------
// Name: DD_Init()
//-----------------------------------------------------------------------------
VOID DD_Init()
{
    g_hInstDDraw = LoadLibraryEx(L"ddraw.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_hInstDDraw)
    {
        g_directDrawCreateEx = reinterpret_cast<LPDIRECTDRAWCREATEEX>(GetProcAddress(g_hInstDDraw, "DirectDrawCreateEx"));
        g_directDrawEnumerateEx = reinterpret_cast<LPDIRECTDRAWENUMERATEEXA>(GetProcAddress(g_hInstDDraw, "DirectDrawEnumerateExA"));
    }
}


//-----------------------------------------------------------------------------
// Name: DD_FillTree()
//-----------------------------------------------------------------------------
VOID DD_FillTree(HWND hwndTV)
{
    if (!g_directDrawEnumerateEx)
        return;

    HTREEITEM hTree;

    // Add DirectDraw devices
    hTree = TVAddNode(TVI_ROOT, L"DirectDraw Devices", TRUE, IDI_DIRECTX,
        nullptr, 0, 0);

    // Add Display Driver node(s) and capability nodes to treeview
    g_directDrawEnumerateEx(DDEnumCallBack, hTree,
        DDENUM_ATTACHEDSECONDARYDEVICES |
        DDENUM_DETACHEDSECONDARYDEVICES |
        DDENUM_NONDISPLAYDEVICES);

    // Hardware Emulation Layer (HEL) not supported on Windows 8,
    // so we no longer show it

    TreeView_Expand(hwndTV, hTree, TVE_EXPAND);
}


//-----------------------------------------------------------------------------
// Name: DD_CleanUp()
//-----------------------------------------------------------------------------
VOID DD_CleanUp()
{
    SAFE_RELEASE(g_pDD);

    if (g_hInstDDraw)
    {
        g_directDrawCreateEx = nullptr;
        g_directDrawEnumerateEx = nullptr;

        FreeLibrary(g_hInstDDraw);
        g_hInstDDraw = nullptr;
    }
}
