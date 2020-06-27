//-----------------------------------------------------------------------------
// Name: DDraw.cpp
//
// Desc: DDraw stuff for DirectX caps viewer
//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#include "dxview.h"
#include <ddraw.h>
#include <stdio.h>

extern VOID DisplayErrorMessage( const CHAR* strError );
extern HRESULT Int2Str( __out_bcount(nDestLen) LPTSTR pszDest, __in UINT nDestLen, __in DWORD i );
extern HRESULT PrintValueLine( const char * szText, DWORD dwValue, PRINTCBINFO *lpInfo );


LPDIRECTDRAW7 g_pDD  = NULL;               // DirectDraw object
GUID* g_pDDGUID;


HRESULT DDDisplayVidMem( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo );
HRESULT DDDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo );
HRESULT DDDisplayVideoModes( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo );
HRESULT DDDisplayFourCCFormat( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo );


#define DDCAPDEFex(name,val,flag) {name, FIELD_OFFSET(DDCAPS,val), flag, DXV_9EXCAP}
#define DDCAPDEF(name,val,flag) {name, FIELD_OFFSET(DDCAPS,val), flag}
#define DDVALDEF(name,val)      {name, FIELD_OFFSET(DDCAPS,val), 0}
#define DDHEXDEF(name,val)      {name, FIELD_OFFSET(DDCAPS,val), 0xFFFFFFFF}
#define ROPDEF(name,dwRops,rop) DDCAPDEF(name,dwRops[((rop>>16)&0xFF)/32],(1<<((rop>>16)&0xFF)%32))
#define MAKEMODE(xres,yres,bpp) (((DWORD)xres << 20) | ((DWORD)yres << 8) | bpp)


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF OtherInfoDefs[] =
{
    DDVALDEF("dwVidMemTotal",                   dwVidMemTotal),
    DDVALDEF("dwVidMemFree",                    dwVidMemFree),
    DDHEXDEF("dwAlphaBltConstBitDepths",        dwAlphaBltConstBitDepths),
    DDCAPDEF("  DDBD_8",                           dwAlphaBltConstBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwAlphaBltConstBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwAlphaBltConstBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwAlphaBltConstBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaBltPixelBitDepths",        dwAlphaBltPixelBitDepths),
    DDCAPDEF("  DDBD_8",                           dwAlphaBltPixelBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwAlphaBltPixelBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwAlphaBltPixelBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwAlphaBltPixelBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaBltSurfaceBitDepths",      dwAlphaBltSurfaceBitDepths),
    DDCAPDEF("  DDBD_8",                           dwAlphaBltSurfaceBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwAlphaBltSurfaceBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwAlphaBltSurfaceBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwAlphaBltSurfaceBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaOverlayConstBitDepths",    dwAlphaOverlayConstBitDepths),
    DDCAPDEF("  DDBD_8",                           dwAlphaOverlayConstBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwAlphaOverlayConstBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwAlphaOverlayConstBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwAlphaOverlayConstBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaOverlayPixelBitDepths",    dwAlphaOverlayPixelBitDepths),
    DDCAPDEF("  DDBD_8",                           dwAlphaOverlayPixelBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwAlphaOverlayPixelBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwAlphaOverlayPixelBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwAlphaOverlayPixelBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaOverlaySurfaceBitDepths",  dwAlphaOverlaySurfaceBitDepths),
    DDCAPDEF("  DDBD_8",                           dwAlphaOverlaySurfaceBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwAlphaOverlaySurfaceBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwAlphaOverlaySurfaceBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwAlphaOverlaySurfaceBitDepths, DDBD_32),
    DDHEXDEF("dwZBufferBitDepths",              dwZBufferBitDepths),
    DDCAPDEF("  DDBD_8",                           dwZBufferBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                          dwZBufferBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                          dwZBufferBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                          dwZBufferBitDepths, DDBD_32),
    DDVALDEF("dwMaxVisibleOverlays",            dwMaxVisibleOverlays),
    DDVALDEF("dwCurrVisibleOverlays",           dwCurrVisibleOverlays),
    DDVALDEF("dwNumFourCCCodes",                dwNumFourCCCodes),
    DDVALDEF("dwAlignBoundarySrc",              dwAlignBoundarySrc),
    DDVALDEF("dwAlignSizeSrc",                  dwAlignSizeSrc),
    DDVALDEF("dwAlignBoundaryDest",             dwAlignBoundaryDest),
    DDVALDEF("dwAlignSizeDest",                 dwAlignSizeDest),
    DDVALDEF("dwAlignStrideAlign",              dwAlignStrideAlign),
    DDVALDEF("dwMinOverlayStretch",             dwMinOverlayStretch),
    DDVALDEF("dwMaxOverlayStretch",             dwMaxOverlayStretch),
    DDVALDEF("dwMinLiveVideoStretch",           dwMinLiveVideoStretch),
    DDVALDEF("dwMaxLiveVideoStretch",           dwMaxLiveVideoStretch),
    DDVALDEF("dwMinHwCodecStretch",             dwMinHwCodecStretch),
    DDVALDEF("dwMaxHwCodecStretch",             dwMaxHwCodecStretch),

    //DDHEXDEF("dwCaps",                      dwCaps),
    DDVALDEF("dwMaxVideoPorts",               dwMaxVideoPorts),
    DDVALDEF("dwCurrVideoPorts",               dwCurrVideoPorts),
    //DDVALDEF("dwSVBCaps2",                   dwSVBCaps2),
    { "", 0, 0 }
};




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


#define GEN_BLTCAPS(dwCaps)                                                     \
    DDCAPDEF("DDCAPS_BLT",                       dwCaps, DDCAPS_BLT),                  \
    DDCAPDEF("DDCAPS_BLTCOLORFILL",              dwCaps, DDCAPS_BLTCOLORFILL),         \
    DDCAPDEF("DDCAPS_BLTDEPTHFILL",              dwCaps, DDCAPS_BLTDEPTHFILL),         \
    DDCAPDEF("DDCAPS_BLTFOURCC",                 dwCaps, DDCAPS_BLTFOURCC),            \
    DDCAPDEF("DDCAPS_BLTSTRETCH",                dwCaps, DDCAPS_BLTSTRETCH),           \
    DDCAPDEF("DDCAPS_BLTQUEUE",                  dwCaps, DDCAPS_BLTQUEUE),             \
    DDCAPDEF("DDCAPS_COLORKEY",                  dwCaps, DDCAPS_COLORKEY),             \
    DDCAPDEF("DDCAPS_ALPHA",                     dwCaps, DDCAPS_ALPHA),                \
    DDCAPDEF("DDCAPS_CKEYHWASSIST",              dwCaps, DDCAPS_COLORKEYHWASSIST),     \
    DDCAPDEF("DDCAPS_CANCLIP",                   dwCaps, DDCAPS_CANCLIP),              \
    DDCAPDEF("DDCAPS_CANCLIPSTRETCHED",          dwCaps, DDCAPS_CANCLIPSTRETCHED),     \
    DDCAPDEF("DDCAPS_CANBLTSYSMEM",              dwCaps, DDCAPS_CANBLTSYSMEM),         \
    DDCAPDEF("DDCAPS_ZBLTS",                     dwCaps, DDCAPS_ZBLTS),                


#define GEN_BLTCAPS2(dwCaps2)                                                   \
    DDCAPDEF("DDCAPS2_CANDROPZ16BIT",             dwCaps2,DDCAPS2_CANDROPZ16BIT),       \
    DDCAPDEF("DDCAPS2_NOPAGELOCKREQUIRED",        dwCaps2,DDCAPS2_NOPAGELOCKREQUIRED),  \
    DDCAPDEF("DDCAPS2_CANFLIPODDEVEN",            dwCaps2,DDCAPS2_CANFLIPODDEVEN),      \
    DDCAPDEF("DDCAPS2_CANBOBHARDWARE",            dwCaps2,DDCAPS2_CANBOBHARDWARE),      \
    DDCAPDEF("DDCAPS2_WIDESURFACES",              dwCaps2,DDCAPS2_WIDESURFACES),

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF GenCaps[] =
{
    // GEN_CAPS(dwCaps)
    DDCAPDEF("DDCAPS_3D",                        dwCaps, DDCAPS_3D),                   
    DDCAPDEF("DDCAPS_ALIGNBOUNDARYDEST",         dwCaps, DDCAPS_ALIGNBOUNDARYDEST),    
    DDCAPDEF("DDCAPS_ALIGNSIZEDEST",             dwCaps, DDCAPS_ALIGNSIZEDEST),        
    DDCAPDEF("DDCAPS_ALIGNBOUNDARYSRC",          dwCaps, DDCAPS_ALIGNBOUNDARYSRC),     
    DDCAPDEF("DDCAPS_ALIGNSIZESRC",              dwCaps, DDCAPS_ALIGNSIZESRC),         
    DDCAPDEF("DDCAPS_ALIGNSTRIDE",               dwCaps, DDCAPS_ALIGNSTRIDE),          
    DDCAPDEF("DDCAPS_GDI",                       dwCaps, DDCAPS_GDI),                  
    DDCAPDEF("DDCAPS_OVERLAY",                   dwCaps, DDCAPS_OVERLAY),              
    DDCAPDEF("DDCAPS_OVERLAYCANTCLIP",           dwCaps, DDCAPS_OVERLAYCANTCLIP),      
    DDCAPDEF("DDCAPS_OVERLAYFOURCC",             dwCaps, DDCAPS_OVERLAYFOURCC),        
    DDCAPDEF("DDCAPS_OVERLAYSTRETCH",            dwCaps, DDCAPS_OVERLAYSTRETCH),       
    DDCAPDEF("DDCAPS_PALETTE",                   dwCaps, DDCAPS_PALETTE),              
    DDCAPDEF("DDCAPS_PALETTEVSYNC",              dwCaps, DDCAPS_PALETTEVSYNC),         
    DDCAPDEF("DDCAPS_READSCANLINE",              dwCaps, DDCAPS_READSCANLINE),         
    DDCAPDEF("DDCAPS_VBI",                       dwCaps, DDCAPS_VBI),                  
    DDCAPDEF("DDCAPS_ZOVERLAYS",                 dwCaps, DDCAPS_ZOVERLAYS),            
    DDCAPDEF("DDCAPS_NOHARDWARE",                dwCaps, DDCAPS_NOHARDWARE),           
    DDCAPDEF("DDCAPS_BANKSWITCHED",              dwCaps, DDCAPS_BANKSWITCHED),

    // GEN_CAPS2(dwCaps2)
    DDCAPDEF("DDCAPS2_CERTIFIED",                 dwCaps2,DDCAPS2_CERTIFIED),           
    DDCAPDEF("DDCAPS2_NO2DDURING3DSCENE",         dwCaps2,DDCAPS2_NO2DDURING3DSCENE),   
    DDCAPDEF("DDCAPS2_VIDEOPORT",                 dwCaps2,DDCAPS2_VIDEOPORT),           
    DDCAPDEF("DDCAPS2_AUTOFLIPOVERLAY",           dwCaps2,DDCAPS2_AUTOFLIPOVERLAY),     
    DDCAPDEF("DDCAPS2_CANBOBINTERLEAVED",         dwCaps2,DDCAPS2_CANBOBINTERLEAVED),   
    DDCAPDEF("DDCAPS2_CANBOBNONINTERLEAVED",      dwCaps2,DDCAPS2_CANBOBNONINTERLEAVED),
    DDCAPDEF("DDCAPS2_COLORCONTROLOVERLAY",       dwCaps2,DDCAPS2_COLORCONTROLOVERLAY), 
    DDCAPDEF("DDCAPS2_COLORCONTROLPRIMARY",       dwCaps2,DDCAPS2_COLORCONTROLPRIMARY), 
    DDCAPDEF("DDCAPS2_NONLOCALVIDMEM",            dwCaps2,DDCAPS2_NONLOCALVIDMEM),      
    DDCAPDEF("DDCAPS2_NONLOCALVIDMEMCAPS",        dwCaps2,DDCAPS2_NONLOCALVIDMEMCAPS),  
    DDCAPDEF("DDCAPS2_WIDESURFACES",              dwCaps2,DDCAPS2_WIDESURFACES),        
    DDCAPDEF("DDCAPS2_NOPAGELOCKREQUIRED",        dwCaps2,DDCAPS2_NOPAGELOCKREQUIRED),  
    DDCAPDEF("DDCAPS2_CANRENDERWINDOWED",         dwCaps2,DDCAPS2_CANRENDERWINDOWED),   
    DDCAPDEF("DDCAPS2_COPYFOURCC",                dwCaps2,DDCAPS2_COPYFOURCC),          
    DDCAPDEF("DDCAPS2_PRIMARYGAMMA",              dwCaps2,DDCAPS2_PRIMARYGAMMA),        
    DDCAPDEF("DDCAPS2_CANCALIBRATEGAMMA",         dwCaps2,DDCAPS2_CANCALIBRATEGAMMA),
    DDCAPDEF("DDCAPS2_FLIPINTERVAL",              dwCaps2,DDCAPS2_FLIPINTERVAL),        
    DDCAPDEF("DDCAPS2_FLIPNOVSYNC",               dwCaps2,DDCAPS2_FLIPNOVSYNC),         
    DDCAPDEF("DDCAPS2_CANMANAGETEXTURE",          dwCaps2,DDCAPS2_CANMANAGETEXTURE),    
    DDCAPDEF("DDCAPS2_TEXMANINNONLOCALVIDMEM",    dwCaps2,DDCAPS2_TEXMANINNONLOCALVIDMEM), 
    DDCAPDEF("DDCAPS2_STEREO",                    dwCaps2,DDCAPS2_STEREO),        
    DDCAPDEFex("DDCAPS2_CANSHARERESOURCE",        dwCaps2,DDCAPS2_CANSHARERESOURCE),

    { "", 0, 0 }
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
    { "", 0, 0 }
};
CAPDEF SVBCapsDefs[] =
{
    GEN_BLTCAPS(dwSVBCaps)
    GEN_BLTCAPS2(dwSVBCaps2)
    { "", 0, 0 }
};
CAPDEF VSBCapsDefs[] =
{
    GEN_BLTCAPS(dwVSBCaps)
//  GEN_BLTCAPS2(dwVSBCaps2)
    { "", 0, 0 }
};
CAPDEF SSBCapsDefs[] =
{
    GEN_BLTCAPS(dwSSBCaps)
//  GEN_BLTCAPS2(dwSSBCaps2)
    { "", 0, 0 }
};
CAPDEF NLVBCapsDefs[] =
{
    GEN_BLTCAPS(dwNLVBCaps)
    GEN_BLTCAPS2(dwNLVBCaps2)
    { "", 0, 0 }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define GEN_CKCAPS(dwCKeyCaps)                                                              \
    DDCAPDEF("DDCKEYCAPS_DESTBLT",                 dwCKeyCaps, DDCKEYCAPS_DESTBLT),                  \
    DDCAPDEF("DDCKEYCAPS_DESTBLTCLRSPACE",         dwCKeyCaps, DDCKEYCAPS_DESTBLTCLRSPACE),          \
    DDCAPDEF("DDCKEYCAPS_DESTBLTCLRSPACEYUV",      dwCKeyCaps, DDCKEYCAPS_DESTBLTCLRSPACEYUV),       \
    DDCAPDEF("DDCKEYCAPS_DESTBLTYUV",              dwCKeyCaps, DDCKEYCAPS_DESTBLTYUV),               \
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAY",             dwCKeyCaps, DDCKEYCAPS_DESTOVERLAY),              \
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYCLRSPACE",     dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACE),      \
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV",  dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV),   \
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYONEACTIVE",    dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYONEACTIVE),     \
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYYUV",          dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYYUV),           \
    DDCAPDEF("DDCKEYCAPS_NOCOSTOVERLAY",           dwCKeyCaps, DDCKEYCAPS_NOCOSTOVERLAY),            \
    DDCAPDEF("DDCKEYCAPS_SRCBLT",                  dwCKeyCaps, DDCKEYCAPS_SRCBLT),                   \
    DDCAPDEF("DDCKEYCAPS_SRCBLTCLRSPACE",          dwCKeyCaps, DDCKEYCAPS_SRCBLTCLRSPACE),           \
    DDCAPDEF("DDCKEYCAPS_SRCBLTCLRSPACEYUV",       dwCKeyCaps, DDCKEYCAPS_SRCBLTCLRSPACEYUV),        \
    DDCAPDEF("DDCKEYCAPS_SRCBLTYUV",               dwCKeyCaps, DDCKEYCAPS_SRCBLTYUV),                \
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAY",              dwCKeyCaps, DDCKEYCAPS_SRCOVERLAY),               \
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYCLRSPACE",      dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACE),       \
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV",   dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV),    \
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYONEACTIVE",     dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYONEACTIVE),      \
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYYUV",           dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYYUV),

CAPDEF CKeyCapsDefs[] =
{
    GEN_CKCAPS(dwCKeyCaps)
    { "", 0, 0}
};
CAPDEF VSBCKeyCapsDefs[] =
{
    GEN_CKCAPS(dwVSBCKeyCaps)
    { "", 0, 0}
};
CAPDEF SVBCKeyCapsDefs[] =
{
    GEN_CKCAPS(dwSVBCKeyCaps)
    { "", 0, 0}
};
CAPDEF SSBCKeyCapsDefs[] =
{
    GEN_CKCAPS(dwSSBCKeyCaps)
    { "", 0, 0}
};
CAPDEF NLVBCKeyCapsDefs[] =
{
    GEN_CKCAPS(dwNLVBCKeyCaps)
    { "", 0, 0}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define GEN_FXCAPS(dwFXCaps)                                                            \
    DDCAPDEF("DDFXCAPS_BLTALPHA",                  dwFXCaps, DDFXCAPS_BLTALPHA),                 \
    DDCAPDEF("DDFXCAPS_BLTARITHSTRETCHY",          dwFXCaps, DDFXCAPS_BLTARITHSTRETCHY),         \
    DDCAPDEF("DDFXCAPS_BLTARITHSTRETCHYN",         dwFXCaps, DDFXCAPS_BLTARITHSTRETCHYN),        \
    DDCAPDEF("DDFXCAPS_BLTFILTER",                 dwFXCaps, DDFXCAPS_BLTFILTER),                \
    DDCAPDEF("DDFXCAPS_BLTMIRRORLEFTRIGHT",        dwFXCaps, DDFXCAPS_BLTMIRRORLEFTRIGHT),       \
    DDCAPDEF("DDFXCAPS_BLTMIRRORUPDOWN",           dwFXCaps, DDFXCAPS_BLTMIRRORUPDOWN),          \
    DDCAPDEF("DDFXCAPS_BLTROTATION",               dwFXCaps, DDFXCAPS_BLTROTATION),              \
    DDCAPDEF("DDFXCAPS_BLTROTATION90",             dwFXCaps, DDFXCAPS_BLTROTATION90),            \
    DDCAPDEF("DDFXCAPS_BLTSHRINKX",                dwFXCaps, DDFXCAPS_BLTSHRINKX),               \
    DDCAPDEF("DDFXCAPS_BLTSHRINKXN",               dwFXCaps, DDFXCAPS_BLTSHRINKXN),              \
    DDCAPDEF("DDFXCAPS_BLTSHRINKY",                dwFXCaps, DDFXCAPS_BLTSHRINKY),               \
    DDCAPDEF("DDFXCAPS_BLTSHRINKYN",               dwFXCaps, DDFXCAPS_BLTSHRINKYN),              \
    DDCAPDEF("DDFXCAPS_BLTSTRETCHX",               dwFXCaps, DDFXCAPS_BLTSTRETCHX),              \
    DDCAPDEF("DDFXCAPS_BLTSTRETCHXN",              dwFXCaps, DDFXCAPS_BLTSTRETCHXN),             \
    DDCAPDEF("DDFXCAPS_BLTSTRETCHY",               dwFXCaps, DDFXCAPS_BLTSTRETCHY),              \
    DDCAPDEF("DDFXCAPS_BLTSTRETCHYN",              dwFXCaps, DDFXCAPS_BLTSTRETCHYN),             \
    DDCAPDEF("DDFXCAPS_OVERLAYALPHA",              dwFXCaps, DDFXCAPS_OVERLAYALPHA),             \
    DDCAPDEF("DDFXCAPS_OVERLAYARITHSTRETCHY",      dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHY),     \
    DDCAPDEF("DDFXCAPS_OVERLAYARITHSTRETCHYN",     dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHYN),    \
    DDCAPDEF("DDFXCAPS_OVERLAYFILTER",             dwFXCaps, DDFXCAPS_OVERLAYFILTER),            \
    DDCAPDEF("DDFXCAPS_OVERLAYMIRRORLEFTRIGHT",    dwFXCaps, DDFXCAPS_OVERLAYMIRRORLEFTRIGHT),   \
    DDCAPDEF("DDFXCAPS_OVERLAYMIRRORUPDOWN",       dwFXCaps, DDFXCAPS_OVERLAYMIRRORUPDOWN),      \
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKX",            dwFXCaps, DDFXCAPS_OVERLAYSHRINKX),           \
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKXN",           dwFXCaps, DDFXCAPS_OVERLAYSHRINKXN),          \
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKY",            dwFXCaps, DDFXCAPS_OVERLAYSHRINKY),           \
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKYN",           dwFXCaps, DDFXCAPS_OVERLAYSHRINKYN),          \
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHX",           dwFXCaps, DDFXCAPS_OVERLAYSTRETCHX),          \
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHXN",          dwFXCaps, DDFXCAPS_OVERLAYSTRETCHXN),         \
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHY",           dwFXCaps, DDFXCAPS_OVERLAYSTRETCHY),          \
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHYN",          dwFXCaps, DDFXCAPS_OVERLAYSTRETCHYN),

CAPDEF FXCapsDefs[] =
{
    GEN_FXCAPS(dwFXCaps)
    { "", 0, 0}
};
CAPDEF VSBFXCapsDefs[] =
{
    GEN_FXCAPS(dwVSBFXCaps)
    { "", 0, 0}
};
CAPDEF SVBFXCapsDefs[] =
{
    GEN_FXCAPS(dwSVBFXCaps)
    { "", 0, 0}
};
CAPDEF SSBFXCapsDefs[] =
{
    GEN_FXCAPS(dwSSBFXCaps)
    { "", 0, 0}
};
CAPDEF NLVBFXCapsDefs[] =
{
    GEN_FXCAPS(dwNLVBFXCaps)
    { "", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define GEN_FXALPHACAPS(dwFXAlphaCaps)                                                          \
    DDCAPDEF("DDFXALPHACAPS_BLTALPHAEDGEBLEND",           dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHAEDGEBLEND),    \
    DDCAPDEF("DDFXALPHACAPS_BLTALPHAPIXELS",              dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHAPIXELS),       \
    DDCAPDEF("DDFXALPHACAPS_BLTALPHAPIXELSNEG",           dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHAPIXELSNEG),    \
    DDCAPDEF("DDFXALPHACAPS_BLTALPHASURFACES",            dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHASURFACES),     \
    DDCAPDEF("DDFXALPHACAPS_BLTALPHASURFACESNEG",         dwFXAlphaCaps, DDFXALPHACAPS_BLTALPHASURFACESNEG),  \
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND",       dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND),\
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHAPIXELS",          dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELS),   \
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG",       dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG),\
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHASURFACES",        dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACES), \
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHASURFACESNEG",     dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACESNEG),

CAPDEF FXAlphaCapsDefs[] =
{
    GEN_FXALPHACAPS(dwFXAlphaCaps)
    { "", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF PalCapsDefs[] =
{
    DDCAPDEF("DDPCAPS_1BIT",              dwPalCaps, DDPCAPS_1BIT),
    DDCAPDEF("DDPCAPS_2BIT",              dwPalCaps, DDPCAPS_2BIT),
    DDCAPDEF("DDPCAPS_4BIT",              dwPalCaps, DDPCAPS_4BIT),
    DDCAPDEF("DDPCAPS_8BITENTRIES",       dwPalCaps, DDPCAPS_8BITENTRIES),
    DDCAPDEF("DDPCAPS_8BIT",              dwPalCaps, DDPCAPS_8BIT),
    DDCAPDEF("DDPCAPS_ALLOW256",          dwPalCaps, DDPCAPS_ALLOW256),
    DDCAPDEF("DDPCAPS_ALPHA",             dwPalCaps, DDPCAPS_ALPHA),
    DDCAPDEF("DDPCAPS_PRIMARYSURFACE",    dwPalCaps, DDPCAPS_PRIMARYSURFACE),
    DDCAPDEF("DDPCAPS_PRIMARYSURFACELEFT",dwPalCaps, DDPCAPS_PRIMARYSURFACELEFT),
    DDCAPDEF("DDPCAPS_VSYNC",             dwPalCaps, DDPCAPS_VSYNC),
    { "", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF SurfCapsDefs[] =
{
    DDCAPDEF("DDSCAPS_3DDEVICE",             ddsCaps.dwCaps,  DDSCAPS_3DDEVICE),
    DDCAPDEF("DDSCAPS_ALPHA",                ddsCaps.dwCaps,  DDSCAPS_ALPHA),
    DDCAPDEF("DDSCAPS_BACKBUFFER",           ddsCaps.dwCaps,  DDSCAPS_BACKBUFFER),
    DDCAPDEF("DDSCAPS_COMPLEX",              ddsCaps.dwCaps,  DDSCAPS_COMPLEX),
    DDCAPDEF("DDSCAPS_FLIP",                 ddsCaps.dwCaps,  DDSCAPS_FLIP),
    DDCAPDEF("DDSCAPS_FRONTBUFFER",          ddsCaps.dwCaps,  DDSCAPS_FRONTBUFFER),
    DDCAPDEF("DDSCAPS_HWCODEC",              ddsCaps.dwCaps,  DDSCAPS_HWCODEC),
    DDCAPDEF("DDSCAPS_LIVEVIDEO",            ddsCaps.dwCaps,  DDSCAPS_LIVEVIDEO),
    DDCAPDEF("DDSCAPS_MIPMAP",               ddsCaps.dwCaps,  DDSCAPS_MIPMAP),
    DDCAPDEF("DDSCAPS_MODEX",                ddsCaps.dwCaps,  DDSCAPS_MODEX),
    DDCAPDEF("DDSCAPS_OFFSCREENPLAIN",       ddsCaps.dwCaps,  DDSCAPS_OFFSCREENPLAIN),
    DDCAPDEF("DDSCAPS_OVERLAY",              ddsCaps.dwCaps,  DDSCAPS_OVERLAY),
    DDCAPDEF("DDSCAPS_OWNDC",                ddsCaps.dwCaps,  DDSCAPS_OWNDC),
    DDCAPDEF("DDSCAPS_PALETTE",              ddsCaps.dwCaps,  DDSCAPS_PALETTE),
    DDCAPDEF("DDSCAPS_PRIMARYSURFACE",       ddsCaps.dwCaps,  DDSCAPS_PRIMARYSURFACE),
    DDCAPDEF("DDSCAPS_SYSTEMMEMORY",         ddsCaps.dwCaps,  DDSCAPS_SYSTEMMEMORY),
    DDCAPDEF("DDSCAPS_TEXTURE",              ddsCaps.dwCaps,  DDSCAPS_TEXTURE),
    DDCAPDEF("DDSCAPS_VIDEOMEMORY",          ddsCaps.dwCaps,  DDSCAPS_VIDEOMEMORY),
    DDCAPDEF("DDSCAPS_VIDEOPORT",            ddsCaps.dwCaps,  DDSCAPS_VIDEOPORT),
    DDCAPDEF("DDSCAPS_VISIBLE",              ddsCaps.dwCaps,  DDSCAPS_VISIBLE),
    DDCAPDEF("DDSCAPS_WRITEONLY",            ddsCaps.dwCaps,  DDSCAPS_WRITEONLY),
    DDCAPDEF("DDSCAPS_ZBUFFER",              ddsCaps.dwCaps,  DDSCAPS_ZBUFFER),
    DDCAPDEF("DDSCAPS_ALLOCONLOAD",          ddsCaps.dwCaps,  DDSCAPS_ALLOCONLOAD),
    DDCAPDEF("DDSCAPS_LOCALVIDMEM",          ddsCaps.dwCaps,  DDSCAPS_LOCALVIDMEM),
    DDCAPDEF("DDSCAPS_NONLOCALVIDMEM",       ddsCaps.dwCaps,  DDSCAPS_NONLOCALVIDMEM),
    DDCAPDEF("DDSCAPS_STANDARDVGAMODE",      ddsCaps.dwCaps,  DDSCAPS_STANDARDVGAMODE),
    DDCAPDEF("DDSCAPS_OPTIMIZED",            ddsCaps.dwCaps,  DDSCAPS_OPTIMIZED),
    DDCAPDEF("DDSCAPS2_HARDWAREDEINTERLACE",  ddsCaps.dwCaps2, DDSCAPS2_HARDWAREDEINTERLACE),
    DDCAPDEF("DDSCAPS2_HINTDYNAMIC",          ddsCaps.dwCaps2, DDSCAPS2_HINTDYNAMIC),
    DDCAPDEF("DDSCAPS2_HINTSTATIC",           ddsCaps.dwCaps2, DDSCAPS2_HINTSTATIC),
    DDCAPDEF("DDSCAPS2_TEXTUREMANAGE",        ddsCaps.dwCaps2, DDSCAPS2_TEXTUREMANAGE),
    DDCAPDEF("DDSCAPS2_OPAQUE",               ddsCaps.dwCaps2, DDSCAPS2_OPAQUE),
    DDCAPDEF("DDSCAPS2_HINTANTIALIASING",     ddsCaps.dwCaps2, DDSCAPS2_HINTANTIALIASING),
    DDCAPDEF("DDSCAPS2_CUBEMAP",              ddsCaps.dwCaps2, DDSCAPS2_CUBEMAP),
    DDCAPDEF("DDSCAPS2_MIPMAPSUBLEVEL",       ddsCaps.dwCaps2, DDSCAPS2_MIPMAPSUBLEVEL),
    DDCAPDEF("DDSCAPS2_D3DTEXTUREMANAGE",     ddsCaps.dwCaps2, DDSCAPS2_D3DTEXTUREMANAGE),
    DDCAPDEF("DDSCAPS2_DONOTPERSIST",         ddsCaps.dwCaps2, DDSCAPS2_DONOTPERSIST),
    DDCAPDEF("DDSCAPS2_STEREOSURFACELEFT",    ddsCaps.dwCaps2, DDSCAPS2_STEREOSURFACELEFT),
    { "", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF SVisionCapsDefs[] =
{
    DDCAPDEF("DDSVCAPS_STEREOSEQUENTIAL",  dwSVCaps, DDSVCAPS_STEREOSEQUENTIAL),
    { "", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define GEN_ROPS(dwRops)                        \
    ROPDEF("SRCCOPY",    dwRops, SRCCOPY),      \
    ROPDEF("SRCPAINT",   dwRops, SRCPAINT),     \
    ROPDEF("SRCAND",     dwRops, SRCAND),       \
    ROPDEF("SRCINVERT",  dwRops, SRCINVERT),    \
    ROPDEF("SRCERASE",   dwRops, SRCERASE),     \
    ROPDEF("NOTSRCCOPY", dwRops, NOTSRCCOPY),   \
    ROPDEF("NOTSRCERASE",dwRops, NOTSRCERASE),  \
    ROPDEF("MERGECOPY",  dwRops, MERGECOPY),    \
    ROPDEF("MERGEPAINT", dwRops, MERGEPAINT),   \
    ROPDEF("PATCOPY",    dwRops, PATCOPY),      \
    ROPDEF("PATPAINT",   dwRops, PATPAINT),     \
    ROPDEF("PATINVERT",  dwRops, PATINVERT),    \
    ROPDEF("DSTINVERT",  dwRops, DSTINVERT),    \
    ROPDEF("BLACKNESS",  dwRops, BLACKNESS),    \
    ROPDEF("WHITENESS",  dwRops, WHITENESS),

CAPDEF ROPCapsDefs[] =
{
    GEN_ROPS(dwRops)
    {"", 0, 0}
};
CAPDEF VSBROPCapsDefs[] =
{
    GEN_ROPS(dwVSBRops)
    {"", 0, 0}
};
CAPDEF SVBROPCapsDefs[] =
{
    GEN_ROPS(dwSVBRops)
    {"", 0, 0}
};
CAPDEF SSBROPCapsDefs[] =
{
    GEN_ROPS(dwSSBRops)
    {"", 0, 0}
};
CAPDEF NLVBROPCapsDefs[] =
{
    GEN_ROPS(dwNLVBRops)
    {"", 0, 0}
};




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF OverlayCapsDefs[] =
{
    DDCAPDEF("DDCAPS_OVERLAY",                         dwCaps, DDCAPS_OVERLAY),

    DDCAPDEF("DDCAPS_ALIGNBOUNDARYDEST",               dwCaps, DDCAPS_ALIGNBOUNDARYDEST),
    DDVALDEF("  dwAlignBoundaryDest",             dwAlignBoundaryDest),
    DDCAPDEF("DDCAPS_ALIGNSIZEDEST",                   dwCaps, DDCAPS_ALIGNSIZEDEST),
    DDVALDEF("  dwAlignSizeDest",                 dwAlignSizeDest),
    DDCAPDEF("DDCAPS_ALIGNBOUNDARYSRC",                dwCaps, DDCAPS_ALIGNBOUNDARYSRC),
    DDVALDEF("  dwAlignBoundarySrc",              dwAlignBoundarySrc),
    DDCAPDEF("DDCAPS_ALIGNSIZESRC",                    dwCaps, DDCAPS_ALIGNSIZESRC),
    DDVALDEF("  dwAlignSizeSrc",                  dwAlignSizeSrc),
    DDCAPDEF("DDCAPS_ALIGNSTRIDE",                     dwCaps, DDCAPS_ALIGNSTRIDE),
    DDVALDEF("  dwAlignStrideAlign",              dwAlignStrideAlign),

    DDCAPDEF("DDCAPS_OVERLAYCANTCLIP",                 dwCaps, DDCAPS_OVERLAYCANTCLIP),
    DDCAPDEF("DDCAPS_OVERLAYFOURCC",                   dwCaps, DDCAPS_OVERLAYFOURCC),
    DDCAPDEF("DDCAPS_OVERLAYSTRETCH",                  dwCaps, DDCAPS_OVERLAYSTRETCH),
    DDCAPDEF("DDCAPS_ZOVERLAYS",                       dwCaps, DDCAPS_ZOVERLAYS),
    DDCAPDEF("DDCAPS2_AUTOFLIPOVERLAY",                 dwCaps2,DDCAPS2_AUTOFLIPOVERLAY),
    DDCAPDEF("DDCAPS2_CANBOBINTERLEAVED",               dwCaps2,DDCAPS2_CANBOBINTERLEAVED),
    DDCAPDEF("DDCAPS2_CANBOBNONINTERLEAVED",            dwCaps2,DDCAPS2_CANBOBNONINTERLEAVED),
    DDCAPDEF("DDCAPS2_COLORCONTROLOVERLAY",             dwCaps2,DDCAPS2_COLORCONTROLOVERLAY),
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAY",                     dwCKeyCaps, DDCKEYCAPS_DESTOVERLAY),
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYCLRSPACE",             dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACE),
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV",          dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV),
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYONEACTIVE",            dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYONEACTIVE),
    DDCAPDEF("DDCKEYCAPS_DESTOVERLAYYUV",                  dwCKeyCaps, DDCKEYCAPS_DESTOVERLAYYUV),
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAY",                      dwCKeyCaps, DDCKEYCAPS_SRCOVERLAY),
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYCLRSPACE",              dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACE),
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV",           dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV),
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYONEACTIVE",             dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYONEACTIVE),
    DDCAPDEF("DDCKEYCAPS_SRCOVERLAYYUV",                   dwCKeyCaps, DDCKEYCAPS_SRCOVERLAYYUV),
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND",           dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND),
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHAPIXELS",              dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELS),
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG",           dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG),
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHASURFACES",            dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACES),
    DDCAPDEF("DDFXALPHACAPS_OVERLAYALPHASURFACESNEG",         dwFXAlphaCaps, DDFXALPHACAPS_OVERLAYALPHASURFACESNEG),
    DDCAPDEF("DDFXCAPS_OVERLAYALPHA",                    dwFXCaps, DDFXCAPS_OVERLAYALPHA),
    DDCAPDEF("DDFXCAPS_OVERLAYARITHSTRETCHY",            dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHY),
    DDCAPDEF("DDFXCAPS_OVERLAYARITHSTRETCHYN",           dwFXCaps, DDFXCAPS_OVERLAYARITHSTRETCHYN),
    DDCAPDEF("DDFXCAPS_OVERLAYFILTER",                   dwFXCaps, DDFXCAPS_OVERLAYFILTER),
    DDCAPDEF("DDFXCAPS_OVERLAYMIRRORLEFTRIGHT",          dwFXCaps, DDFXCAPS_OVERLAYMIRRORLEFTRIGHT),
    DDCAPDEF("DDFXCAPS_OVERLAYMIRRORUPDOWN",             dwFXCaps, DDFXCAPS_OVERLAYMIRRORUPDOWN),
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKX",                  dwFXCaps, DDFXCAPS_OVERLAYSHRINKX),
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKXN",                 dwFXCaps, DDFXCAPS_OVERLAYSHRINKXN),
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKY",                  dwFXCaps, DDFXCAPS_OVERLAYSHRINKY),
    DDCAPDEF("DDFXCAPS_OVERLAYSHRINKYN",                 dwFXCaps, DDFXCAPS_OVERLAYSHRINKYN),
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHX",                 dwFXCaps, DDFXCAPS_OVERLAYSTRETCHX),
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHXN",                dwFXCaps, DDFXCAPS_OVERLAYSTRETCHXN),
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHY",                 dwFXCaps, DDFXCAPS_OVERLAYSTRETCHY),
    DDCAPDEF("DDFXCAPS_OVERLAYSTRETCHYN",                dwFXCaps, DDFXCAPS_OVERLAYSTRETCHYN),
    DDHEXDEF("dwAlphaOverlayConstBitDepths",      dwAlphaOverlayConstBitDepths),
    DDCAPDEF("  DDBD_8",                             dwAlphaOverlayConstBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                            dwAlphaOverlayConstBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                            dwAlphaOverlayConstBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                            dwAlphaOverlayConstBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaOverlayPixelBitDepths",      dwAlphaOverlayPixelBitDepths),
    DDCAPDEF("  DDBD_8",                             dwAlphaOverlayPixelBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                            dwAlphaOverlayPixelBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                            dwAlphaOverlayPixelBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                            dwAlphaOverlayPixelBitDepths, DDBD_32),
    DDHEXDEF("dwAlphaOverlaySurfaceBitDepths",    dwAlphaOverlaySurfaceBitDepths),
    DDCAPDEF("  DDBD_8",                             dwAlphaOverlaySurfaceBitDepths, DDBD_8),
    DDCAPDEF("  DDBD_16",                            dwAlphaOverlaySurfaceBitDepths, DDBD_16),
    DDCAPDEF("  DDBD_24",                            dwAlphaOverlaySurfaceBitDepths, DDBD_24),
    DDCAPDEF("  DDBD_32",                            dwAlphaOverlaySurfaceBitDepths, DDBD_32),
    DDVALDEF("dwMaxVisibleOverlays",              dwMaxVisibleOverlays),
    DDVALDEF("dwCurrVisibleOverlays",             dwCurrVisibleOverlays),
    DDVALDEF("dwMinOverlayStretch",               dwMinOverlayStretch),
    DDVALDEF("dwMaxOverlayStretch",               dwMaxOverlayStretch),
    { "", 0, 0}
};



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF VideoPortCapsDefs[] =
{
    DDCAPDEF("DDCAPS2_VIDEOPORT",               dwCaps2,DDCAPS2_VIDEOPORT),
    DDCAPDEF("DDSCAPS_VIDEOPORT",               ddsCaps.dwCaps, DDSCAPS_VIDEOPORT),
    DDCAPDEF("DDSCAPS_LIVEVIDEO",                       ddsCaps.dwCaps, DDSCAPS_LIVEVIDEO),
    DDCAPDEF("DDSCAPS2_HARDWAREDEINTERLACE",             dwCaps2,DDSCAPS2_HARDWAREDEINTERLACE),
    DDCAPDEF("DDSCAPS2_AUTOFLIPOVERLAY",                 dwCaps2,DDCAPS2_AUTOFLIPOVERLAY),
    DDVALDEF("dwMinLiveVideoStretch",             dwMinLiveVideoStretch),
    DDVALDEF("dwMaxLiveVideoStretch",             dwMaxLiveVideoStretch),
    DDVALDEF("dwMaxVideoPorts",                   dwMaxVideoPorts),
    DDVALDEF("dwCurrVideoPorts",                  dwCurrVideoPorts),
    { "", 0, 0}
};



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEFS DDCapDefs[] =
{
    {"",                    NULL,               (LPARAM)0,                  },
    {"Memory",              DDDisplayVidMem,    (LPARAM)0,                  },

    {"+Caps",               NULL,               (LPARAM)0,                  },
    {"General",             DDDisplayCaps,      (LPARAM)GenCaps,            },
    {"FX Alpha Caps",       DDDisplayCaps,      (LPARAM)FXAlphaCapsDefs,    },
    {"Palette Caps",        DDDisplayCaps,      (LPARAM)PalCapsDefs,        },
    {"Overlay Caps",        DDDisplayCaps,      (LPARAM)OverlayCapsDefs,    },
    {"Surface Caps",        DDDisplayCaps,      (LPARAM)SurfCapsDefs,       },
    {"Stereo Vision Caps",  DDDisplayCaps,      (LPARAM)SVisionCapsDefs,    },
    {"Video Port Caps",     DDDisplayCaps,      (LPARAM)VideoPortCapsDefs,  },

    {"+BLT Caps",           NULL,               (LPARAM)0,                  },

    {"+Video - Video",      NULL,               (LPARAM)0,                  },
    {"General",             DDDisplayCaps,      (LPARAM)CapsDefs,           },
    {"Color Key",           DDDisplayCaps,      (LPARAM)CKeyCapsDefs,       },
    {"FX",                  DDDisplayCaps,      (LPARAM)FXCapsDefs,         },
    {"ROPS",                DDDisplayCaps,      (LPARAM)ROPCapsDefs,        },
    {"-",                   NULL,               (LPARAM)0,                  },

    {"+System - Video",     NULL,               (LPARAM)0,                  },
    {"General",             DDDisplayCaps,      (LPARAM)SVBCapsDefs,        },
    {"Color Key",           DDDisplayCaps,      (LPARAM)SVBCKeyCapsDefs,    },
    {"FX",                  DDDisplayCaps,      (LPARAM)SVBFXCapsDefs,      },
    {"ROPS",                DDDisplayCaps,      (LPARAM)SVBROPCapsDefs,     },
    {"-",                   NULL,               (LPARAM)0,                  },

    {"+Video - System",     NULL,               (LPARAM)0,                  },
    {"General",             DDDisplayCaps,      (LPARAM)VSBCapsDefs,        },
    {"Color Key",           DDDisplayCaps,      (LPARAM)SSBCKeyCapsDefs,    },
    {"FX",                  DDDisplayCaps,      (LPARAM)SSBFXCapsDefs,      },
    {"ROPS",                DDDisplayCaps,      (LPARAM)VSBROPCapsDefs,     },
    {"-",                   NULL,               (LPARAM)0,                  },

    {"+System - System",    NULL,               (LPARAM)0,                  },
    {"General",             DDDisplayCaps,      (LPARAM)SSBCapsDefs,        },
    {"Color Key",           DDDisplayCaps,      (LPARAM)SSBCKeyCapsDefs,    },
    {"FX",                  DDDisplayCaps,      (LPARAM)SSBFXCapsDefs,      },
    {"ROPS",                DDDisplayCaps,      (LPARAM)SSBROPCapsDefs,     },
    {"-",                   NULL,               (LPARAM)0,                  },

    {"+NonLocal - Video",   NULL,               (LPARAM)0,                  },
    {"General",             DDDisplayCaps,      (LPARAM)NLVBCapsDefs,       },
    {"Color Key",           DDDisplayCaps,      (LPARAM)NLVBCKeyCapsDefs,   },
    {"FX",                  DDDisplayCaps,      (LPARAM)NLVBFXCapsDefs,     },
    {"ROPS",                DDDisplayCaps,      (LPARAM)NLVBROPCapsDefs,    },
    {"-",                   NULL,               (LPARAM)0,                  },

    {"-",                   NULL,               (LPARAM)0,                  },
    {"-",                   NULL,               (LPARAM)0,                  },

    {"Video Modes",         DDDisplayVideoModes,(LPARAM)0,                  },
    {"FourCC Formats",      DDDisplayFourCCFormat,(LPARAM)0,                },
    {"Other",               DDDisplayCaps,      (LPARAM)OtherInfoDefs,      },

    { NULL, 0, 0 }
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DDCreate( GUID* pGUID )
{
    if( pGUID == (GUID *)-2 )
        return E_FAIL;
        
    if( g_pDD && pGUID == g_pDDGUID )
        return S_OK;

    if( g_pDD )
        g_pDD->Release();
    g_pDD = NULL;

    // There is no need to create DirectDraw emulation-only just to get
    // the HEL caps.  In fact, this will fail if there is another DirectDraw
    // app running and using the hardware.
    if( pGUID == (GUID *)DDCREATE_EMULATIONONLY )
        pGUID = NULL;

    if( SUCCEEDED( DirectDrawCreateEx( pGUID, (VOID**)&g_pDD, IID_IDirectDraw7, NULL ) ) )
    {
        g_pDDGUID = pGUID;
        return S_OK;
    }

    DisplayErrorMessage( "DirectDrawCreate failed." );
    
    return E_FAIL;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DDDisplayVidMem( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo )
{
    if( SUCCEEDED( DDCreate( (GUID*)lParam1 ) ) )
    {
        DWORD dwTotalVidMem = 0, dwFreeVidMem = 0;
        DWORD dwTotalLocMem = 0, dwFreeLocMem = 0;
        DWORD dwTotalAGPMem = 0, dwFreeAGPMem = 0;
        DWORD dwTotalTexMem = 0, dwFreeTexMem = 0;

        DDSCAPS2 ddsCaps2;
        ZeroMemory( &ddsCaps2, sizeof(ddsCaps2) );

        ddsCaps2.dwCaps = DDSCAPS_VIDEOMEMORY;
        HRESULT hr = g_pDD->GetAvailableVidMem( &ddsCaps2, &dwTotalVidMem, &dwFreeVidMem );
        if (FAILED(hr))
        {
            dwTotalVidMem = 0;
            dwFreeVidMem = 0;
        }

        ddsCaps2.dwCaps = DDSCAPS_LOCALVIDMEM;
        hr = g_pDD->GetAvailableVidMem( &ddsCaps2, &dwTotalLocMem, &dwFreeLocMem );
        if (FAILED(hr))
        {
            dwTotalLocMem = 0;
            dwFreeLocMem = 0;
        }

        ddsCaps2.dwCaps = DDSCAPS_NONLOCALVIDMEM;
        hr = g_pDD->GetAvailableVidMem( &ddsCaps2, &dwTotalAGPMem, &dwFreeAGPMem );
        if (FAILED(hr))
        {
            dwTotalAGPMem = 0;
            dwFreeAGPMem = 0;
        }

        ddsCaps2.dwCaps = DDSCAPS_TEXTURE;
        hr = g_pDD->GetAvailableVidMem( &ddsCaps2, &dwTotalTexMem, &dwFreeTexMem );
        if (FAILED(hr))
        {
            dwTotalTexMem = 0;
            dwFreeTexMem = 0;
        }
        
        if( pPrintInfo )
        {
            PrintValueLine( "dwTotalVidMem", dwTotalVidMem, pPrintInfo );
            PrintValueLine( "dwFreeVidMem",  dwFreeVidMem,  pPrintInfo );
            PrintValueLine( "dwTotalLocMem", dwTotalLocMem, pPrintInfo );
            PrintValueLine( "dwFreeLocMem",  dwFreeLocMem,  pPrintInfo );
            PrintValueLine( "dwTotalAGPMem", dwTotalAGPMem, pPrintInfo );
            PrintValueLine( "dwFreeAGPMem",  dwFreeAGPMem,  pPrintInfo );
            PrintValueLine( "dwTotalTexMem", dwTotalTexMem, pPrintInfo );
            PrintValueLine( "dwFreeTexMem",  dwFreeTexMem,  pPrintInfo );
        }
        else
        {
            CHAR strBuff[64];

            LVAddColumn( g_hwndLV, 0, "Type",  24 );
            LVAddColumn( g_hwndLV, 1, "Total", 10 );
            LVAddColumn( g_hwndLV, 2, "Free",  10 );

            LVAddText( g_hwndLV, 0, "Video" );
            Int2Str( strBuff, 64, dwTotalVidMem );
            LVAddText( g_hwndLV, 1, "%s", strBuff );
            Int2Str( strBuff, 64, dwFreeVidMem );
            LVAddText( g_hwndLV, 2, "%s", strBuff );

            LVAddText( g_hwndLV, 0, "Video (local)" );
            Int2Str( strBuff, 64, dwTotalLocMem );
            LVAddText( g_hwndLV, 1, "%s", strBuff );
            Int2Str( strBuff, 64, dwFreeLocMem );
            LVAddText( g_hwndLV, 2, "%s", strBuff );

            LVAddText( g_hwndLV, 0, "Video (non-local)" );
            Int2Str( strBuff, 64, dwTotalAGPMem );
            LVAddText( g_hwndLV, 1, "%s", strBuff );
            Int2Str( strBuff, 64, dwFreeAGPMem );
            LVAddText( g_hwndLV, 2, "%s", strBuff );

            LVAddText( g_hwndLV, 0, "Texture" );
            Int2Str( strBuff, 64, dwTotalTexMem );
            LVAddText( g_hwndLV, 1, "%s", strBuff );
            Int2Str( strBuff, 64, dwFreeTexMem );
            LVAddText( g_hwndLV, 2, "%s", strBuff );
        }
    }

    return S_OK; 
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DDDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo )
{
    // lParam1 is the GUID for the driver we should open
    // lParam2 is the CAPDEF table we should use
    if( SUCCEEDED( DDCreate( (GUID*)lParam1 ) ) )
    {
        DDCAPS ddcaps;
        ZeroMemory( &ddcaps, sizeof(ddcaps) );
        ddcaps.dwSize = sizeof(ddcaps);

        HRESULT hr;
        if( lParam1 == DDCREATE_EMULATIONONLY )
            hr = g_pDD->GetCaps( NULL, &ddcaps);
        else
            hr = g_pDD->GetCaps( &ddcaps, NULL);
        if ( FAILED(hr) )
            ZeroMemory( &ddcaps, sizeof(ddcaps) );

        if( pPrintInfo )
            return PrintCapsToDC( (CAPDEF*)lParam2, (VOID*)&ddcaps, pPrintInfo );
        else
            AddCapsToLV((CAPDEF *)lParam2, (LPVOID)&ddcaps);
    }

    // Keep printing, even if an error occurred
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DDDisplayFourCCFormat( LPARAM lParam1, LPARAM lParam2,
                               PRINTCBINFO* pPrintInfo )
{
    HRESULT hr;
    DWORD   dwNumOfCodes;
    DWORD*  FourCC;

    if( NULL == g_pDD )
        return S_OK;

    if( FAILED( hr = g_pDD->GetFourCCCodes( &dwNumOfCodes, NULL ) ) )
        return E_FAIL;

    if( NULL == ( FourCC = (DWORD*)GlobalAlloc(GPTR,(sizeof(DWORD)*dwNumOfCodes) ) ) )
        return E_FAIL;

    if( FAILED( hr = g_pDD->GetFourCCCodes( &dwNumOfCodes, FourCC ) ) )
        return E_FAIL;

    // Add columns
    if( NULL == pPrintInfo )
    {
        LVAddColumn(g_hwndLV, 0, "Codes", 24);
        LVAddColumn(g_hwndLV, 1, "", 24);
    }

    // Assume all FourCC values are ascii strings
    for( DWORD dwCount = 0; dwCount < dwNumOfCodes; dwCount++ )
    {
        CHAR strText[5]={0,0,0,0,0};
        memcpy( strText, &FourCC[dwCount], 4 );

        if( NULL == pPrintInfo )
        {
            LVAddText( g_hwndLV, 0, "%s", strText );
        }
        else
        {
            int xCode = ( pPrintInfo->dwCurrIndent * DEF_TAB_SIZE * pPrintInfo->dwCharWidth );
            int yLine = ( pPrintInfo->dwCurrLine * pPrintInfo->dwLineHeight );

            // Print Code
            if( FAILED( PrintLine( xCode, yLine, strText, 4, pPrintInfo ) ) )
                return E_FAIL;

            if( FAILED( PrintNextLine(pPrintInfo) ) )
                return E_FAIL;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DisplayEnumModes()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CALLBACK EnumDisplayModesCallback( DDSURFACEDESC2* pddsd, VOID* Context )
{
    if( pddsd->ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE )
    {
        LVAddText( g_hwndLV, 0, "%dx%dx%d (StandardVGA)",
                   pddsd->dwWidth, pddsd->dwHeight,
                   pddsd->ddpfPixelFormat.dwRGBBitCount );
    }
    else if( pddsd->ddsCaps.dwCaps & DDSCAPS_MODEX )
    {
        LVAddText( g_hwndLV, 0, "%dx%dx%d (ModeX)",
                   pddsd->dwWidth, pddsd->dwHeight,
                   pddsd->ddpfPixelFormat.dwRGBBitCount );
    }
    else
    {
        LVAddText( g_hwndLV, 0, "%dx%dx%d ",
                   pddsd->dwWidth, pddsd->dwHeight,
                   pddsd->ddpfPixelFormat.dwRGBBitCount );
    }
    
    return DDENUMRET_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CALLBACK EnumDisplayModesCallbackPrint( DDSURFACEDESC2* pddsd, VOID* Context )
{
    TCHAR szBuff[80];
    DWORD cchLen;
    PRINTCBINFO* lpInfo = (PRINTCBINFO*)Context;
    int xMode, yLine;

    if (! lpInfo)
        return FALSE;

    xMode = (lpInfo->dwCurrIndent * DEF_TAB_SIZE * lpInfo->dwCharWidth);
    yLine = (lpInfo->dwCurrLine * lpInfo->dwLineHeight);

    if(pddsd->ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE)
    {
        sprintf_s(szBuff, sizeof(szBuff), TEXT("%dx%dx%d (StandardVGA)"), pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
    }else if(pddsd->ddsCaps.dwCaps & DDSCAPS_MODEX)
    {
        sprintf_s(szBuff, sizeof(szBuff), TEXT("%dx%dx%d (ModeX)"), pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
    }else
    {
        sprintf_s(szBuff, sizeof(szBuff), TEXT("%dx%dx%d "), pddsd->dwWidth, pddsd->dwHeight, pddsd->ddpfPixelFormat.dwRGBBitCount);
    }
    // Print Mode Info
    cchLen = _tcslen (szBuff);
    if( FAILED( PrintLine (xMode, yLine, szBuff, cchLen, lpInfo) ) )
        return DDENUMRET_CANCEL;
    // Advance to next line
    if( FAILED( PrintNextLine(lpInfo) ) )
        return DDENUMRET_CANCEL;

    return DDENUMRET_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DDDisplayVideoModes( LPARAM lParam1, LPARAM lParam2,
                             PRINTCBINFO* pPrintInfo )
{
    DWORD          mode;
    DDSURFACEDESC2 ddsd;

    if( pPrintInfo == NULL )
    {
        LVAddColumn(g_hwndLV, 0, "Mode", 24);
        LVAddColumn(g_hwndLV, 1, "", 24);
    }

    // lParam1 is the GUID for the driver we should open
    // lParam2 is not used

    if( SUCCEEDED( DDCreate((GUID*)lParam1)) )
    {
        // Get the current mode mode for this driver
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        HRESULT hr = g_pDD->GetDisplayMode( &ddsd );
        if ( SUCCEEDED( hr ) )
        {
            mode = MAKEMODE(ddsd.dwWidth, ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount);

            // Get Mode with ModeX
            g_pDD->SetCooperativeLevel( g_hwndMain, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE |
                                        DDSCL_ALLOWMODEX | DDSCL_NOWINDOWCHANGES );

            if( pPrintInfo )
                g_pDD->EnumDisplayModes( DDEDM_STANDARDVGAMODES, NULL, (VOID*)pPrintInfo,
                                         EnumDisplayModesCallbackPrint );
            else
                g_pDD->EnumDisplayModes( DDEDM_STANDARDVGAMODES, NULL, UIntToPtr(mode),
                                          EnumDisplayModesCallback );

            g_pDD->SetCooperativeLevel( g_hwndMain, DDSCL_NORMAL);
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL CALLBACK DDEnumCallBack( __in GUID* pid, __in_z LPSTR lpDriverDesc,
                              __in_z_opt LPSTR lpDriverName, __in_opt VOID* lpContext, __in_opt HMONITOR )
{
    HTREEITEM hParent = (HTREEITEM)lpContext;
    TCHAR szText[256];

    if (pid != (GUID *)-2)
        if (HIWORD(pid) != 0)
        {
            GUID temp = *pid;
            pid = (GUID *)LocalAlloc(LPTR, sizeof(GUID));
            if( pid != NULL )
                *pid = temp;
        }

    // Add subnode to treeview
    if (lpDriverName && *lpDriverName)
        sprintf_s(szText, sizeof(szText), "%s (%s)", lpDriverDesc, lpDriverName);
    else
        strcpy_s(szText, sizeof(szText), lpDriverDesc);
    szText[255] = TEXT('\0');

    DDCapDefs[0].strName = szText;
    AddCapsToTV(hParent, DDCapDefs, (LPARAM)pid);

    return(DDENUMRET_OK);
}




//-----------------------------------------------------------------------------
// Name: DD_Init()
// Desc: 
//-----------------------------------------------------------------------------
VOID DD_Init()
{
}




//-----------------------------------------------------------------------------
// Name: DD_FillTree()
// Desc: 
//-----------------------------------------------------------------------------
VOID DD_FillTree( HWND hwndTV )
{
    HTREEITEM hTree;

    // Add DirectDraw devices
    hTree = TVAddNode( TVI_ROOT, "DirectDraw Devices", TRUE, IDI_DIRECTX,
                       NULL, 0, 0 );

    // Add Display Driver node(s) and capability nodes to treeview
    DirectDrawEnumerateEx( DDEnumCallBack, hTree,
                           DDENUM_ATTACHEDSECONDARYDEVICES |
                           DDENUM_DETACHEDSECONDARYDEVICES |
                           DDENUM_NONDISPLAYDEVICES );

    // Hardware Emulation Layer (HEL) not supported on Windows 8,
    // so we no longer show it

    TreeView_Expand( hwndTV, hTree, TVE_EXPAND );
}




//-----------------------------------------------------------------------------
// Name: DD_CleanUp()
// Desc: 
//-----------------------------------------------------------------------------
VOID DD_CleanUp()
{
    if( g_pDD )
        g_pDD->Release();
    g_pDD = NULL;
}









