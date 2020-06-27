//-----------------------------------------------------------------------------
// Name: DSound.cpp
//
// Desc: DSound stuff for DirectX caps viewer
//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#include "dxview.h"
#include <dsound.h>
#include <d3dtypes.h>
#include <stdio.h>

VOID DisplayErrorMessage( const CHAR* strError );

LPDIRECTSOUND        g_pDSound    = NULL; // DirectSound object
LPDIRECTSOUNDCAPTURE g_pDSCapture = NULL; // DirectSoundCapture object
GUID* g_pDSGUID;
GUID* g_pDSCGUID;


HRESULT DSDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo );
HRESULT DSCDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo );




#define DSCAPDEF(name,val,flag) {name, FIELD_OFFSET(DSCAPS,val), flag}
#define DSVALDEF(name,val)      {name, FIELD_OFFSET(DSCAPS,val), 0}

#define DSCCAPDEF(name,val,flag) {name, FIELD_OFFSET(DSCCAPS,val), flag}
#define DSCVALDEF(name,val)      {name, FIELD_OFFSET(DSCCAPS,val), 0}




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF DSInfo[] =
{
    DSVALDEF("dwMinSecondarySampleRate",          dwMinSecondarySampleRate),
    DSVALDEF("dwMaxSecondarySampleRate",          dwMaxSecondarySampleRate),
    DSVALDEF("dwPrimaryBuffers",                  dwPrimaryBuffers),
    DSVALDEF("dwMaxHwMixingAllBuffers",           dwMaxHwMixingAllBuffers),
    DSVALDEF("dwMaxHwMixingStaticBuffers",        dwMaxHwMixingStaticBuffers),
    DSVALDEF("dwMaxHwMixingStreamingBuffers",     dwMaxHwMixingStreamingBuffers),
    DSVALDEF("dwFreeHwMixingAllBuffers",          dwFreeHwMixingAllBuffers),
    DSVALDEF("dwFreeHwMixingStaticBuffers",       dwFreeHwMixingStaticBuffers),
    DSVALDEF("dwFreeHwMixingStreamingBuffers",    dwFreeHwMixingStreamingBuffers),
    DSVALDEF("dwMaxHw3DAllBuffers",               dwMaxHw3DAllBuffers),
    DSVALDEF("dwMaxHw3DStaticBuffers",            dwMaxHw3DStaticBuffers),
    DSVALDEF("dwMaxHw3DStreamingBuffers",         dwMaxHw3DStreamingBuffers),
    DSVALDEF("dwFreeHw3DAllBuffers",              dwFreeHw3DAllBuffers),
    DSVALDEF("dwFreeHw3DStaticBuffers",           dwFreeHw3DStaticBuffers),
    DSVALDEF("dwFreeHw3DStreamingBuffers",        dwFreeHw3DStreamingBuffers),
    DSVALDEF("dwTotalHwMemBytes",                 dwTotalHwMemBytes),
    DSVALDEF("dwFreeHwMemBytes",                  dwFreeHwMemBytes),
    DSVALDEF("dwMaxContigFreeHwMemBytes",         dwMaxContigFreeHwMemBytes),
    DSVALDEF("dwUnlockTransferRateHwBuffers",     dwUnlockTransferRateHwBuffers),
    DSVALDEF("dwPlayCpuOverheadSwBuffers",        dwPlayCpuOverheadSwBuffers),
    {"", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF DSGeneralCaps[] =
{
    DSCAPDEF("DSCAPS_PRIMARYMONO",       dwFlags,    DSCAPS_PRIMARYMONO),
    DSCAPDEF("DSCAPS_PRIMARYSTEREO",     dwFlags,    DSCAPS_PRIMARYSTEREO),
    DSCAPDEF("DSCAPS_PRIMARY8BIT",       dwFlags,    DSCAPS_PRIMARY8BIT),
    DSCAPDEF("DSCAPS_PRIMARY16BIT",      dwFlags,    DSCAPS_PRIMARY16BIT),
    DSCAPDEF("DSCAPS_CONTINUOUSRATE",    dwFlags,    DSCAPS_CONTINUOUSRATE),
    DSCAPDEF("DSCAPS_EMULDRIVER",        dwFlags,    DSCAPS_EMULDRIVER),
    DSCAPDEF("DSCAPS_SECONDARYMONO",     dwFlags,    DSCAPS_SECONDARYMONO),
    DSCAPDEF("DSCAPS_SECONDARYSTEREO",   dwFlags,    DSCAPS_SECONDARYSTEREO),
    DSCAPDEF("DSCAPS_SECONDARY8BIT",     dwFlags,    DSCAPS_SECONDARY8BIT),
    DSCAPDEF("DSCAPS_SECONDARY16BIT",    dwFlags,    DSCAPS_SECONDARY16BIT),
    {"", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEFS DSCapDefs[] =
{
    {"",                    DSDisplayCaps,          (LPARAM)DSInfo,        },
    {"General",             DSDisplayCaps,          (LPARAM)DSInfo,        },
    {"General Caps",        DSDisplayCaps,          (LPARAM)DSGeneralCaps, },
    {NULL, 0, 0 }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF DSCInfo[] =
{
    DSCVALDEF("dwChannels",          dwChannels),
    {"", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEF DSCGeneralCaps[] =
{
    DSCCAPDEF("DSCCAPS_EMULDRIVER",dwFlags,    DSCCAPS_EMULDRIVER),
    DSCCAPDEF("WAVE_FORMAT_1M08",  dwFormats,  WAVE_FORMAT_1M08),
    DSCCAPDEF("WAVE_FORMAT_1S08",  dwFormats,  WAVE_FORMAT_1S08),
    DSCCAPDEF("WAVE_FORMAT_1M16",  dwFormats,  WAVE_FORMAT_1M16),
    DSCCAPDEF("WAVE_FORMAT_1S16",  dwFormats,  WAVE_FORMAT_1S16),
    DSCCAPDEF("WAVE_FORMAT_2M08",  dwFormats,  WAVE_FORMAT_2M08),
    DSCCAPDEF("WAVE_FORMAT_2S08",  dwFormats,  WAVE_FORMAT_2S08),
    DSCCAPDEF("WAVE_FORMAT_2M16",  dwFormats,  WAVE_FORMAT_2M16),
    DSCCAPDEF("WAVE_FORMAT_2S16",  dwFormats,  WAVE_FORMAT_2S16),
    DSCCAPDEF("WAVE_FORMAT_4M08",  dwFormats,  WAVE_FORMAT_4M08),
    DSCCAPDEF("WAVE_FORMAT_4S08",  dwFormats,  WAVE_FORMAT_4S08),
    DSCCAPDEF("WAVE_FORMAT_4M16",  dwFormats,  WAVE_FORMAT_4M16),
    DSCCAPDEF("WAVE_FORMAT_4S16",  dwFormats,  WAVE_FORMAT_4S16),
    {"", 0, 0}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEFS DSCCapDefs[] =
{
    {"",                    DSCDisplayCaps,          (LPARAM)DSCInfo,        },
    {"General",             DSCDisplayCaps,          (LPARAM)DSCInfo,        },
    {"General Caps",        DSCDisplayCaps,          (LPARAM)DSCGeneralCaps, },
    {NULL, 0, 0 }
};




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
LPDIRECTSOUND DSCreate( GUID* pGUID )
{
    if( g_pDSound && pGUID == g_pDSGUID )
        return g_pDSound;

    if( g_pDSound )
        g_pDSound->Release();
    g_pDSound = NULL;

    // Call DirectSoundCreate.
    if( SUCCEEDED( DirectSoundCreate( pGUID, &g_pDSound, NULL ) ) )
    {
        g_pDSGUID = pGUID;
        return g_pDSound;
    }

    DisplayErrorMessage( "DirectSoundCreate failed." );

	return NULL;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
LPDIRECTSOUNDCAPTURE DSCCreate( GUID* pGUID )
{
    if( g_pDSCapture && pGUID == g_pDSCGUID )
        return g_pDSCapture;

    if( g_pDSCapture )
        g_pDSCapture->Release();
    g_pDSCapture = NULL;

    if( SUCCEEDED( DirectSoundCaptureCreate( pGUID, &g_pDSCapture, NULL ) ) )
    {
        g_pDSCGUID = pGUID;
        return g_pDSCapture;
    }

    DisplayErrorMessage( "DirectSoundCaptureCreate failed." );

    return NULL;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DSDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo )
{
    // lParam1 is the GUID for the driver we should open
    // lParam2 is the CAPDEF table we should use
    if( DSCreate( (GUID*)lParam1 ) )
    {
        DSCAPS dscaps;
        dscaps.dwSize = sizeof(dscaps);

        g_pDSound->GetCaps( &dscaps);

		if( pPrintInfo )
	        return PrintCapsToDC( (CAPDEF*)lParam2, (VOID*)&dscaps, pPrintInfo );
		else
	        AddCapsToLV( (CAPDEF*)lParam2, (VOID*)&dscaps );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT DSCDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo )
{
    // lParam1 is the GUID for the driver we should open
    // lParam2 is the CAPDEF table we should use
    if( DSCCreate( (GUID*)lParam1 ) )
    {
        DSCCAPS dsccaps;
        dsccaps.dwSize = sizeof(dsccaps);

        g_pDSCapture->GetCaps( &dsccaps );

		if( pPrintInfo )
	        return PrintCapsToDC( (CAPDEF*)lParam2, (VOID*)&dsccaps, pPrintInfo );
		else
	        AddCapsToLV( (CAPDEF*)lParam2, (VOID*)&dsccaps );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL CALLBACK DSEnumCallBack( GUID* lpGUID, const CHAR* lpDriverDesc,
							  const CHAR* lpDriverName, VOID* lpContext)
{
    HTREEITEM hParent = (HTREEITEM)lpContext;
    TCHAR     szText[256];
    LPGUID    lpTemp = NULL;

    if( lpGUID != NULL )
        {
        if(( lpTemp = (GUID*)LocalAlloc( LPTR, sizeof(GUID))) == NULL )
        return( TRUE );

        memcpy( lpTemp, lpGUID, sizeof(GUID));
    }

    // Add subnode to treeview
    if (lpDriverName && *lpDriverName)
        sprintf_s(szText, sizeof(szText), "%s (%s)", lpDriverDesc, lpDriverName);
    else
        strcpy_s(szText, sizeof(szText), lpDriverDesc);
    szText[255] = TEXT('\0');

    DSCapDefs[0].strName = szText;
    AddCapsToTV(hParent, DSCapDefs, (LPARAM)lpTemp);

    return(DDENUMRET_OK);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL CALLBACK DSCEnumCallBack( GUID* lpGUID, const CHAR* lpDriverDesc, 
							   const CHAR* lpDriverName, VOID* lpContext)
{
    HTREEITEM hParent = (HTREEITEM)lpContext;
    TCHAR     szText[256];
    LPGUID    lpTemp = NULL;

    if( lpGUID != NULL )
    {
        if(( lpTemp = (GUID*)LocalAlloc( LPTR, sizeof(GUID))) == NULL )
        return( TRUE );

        memcpy( lpTemp, lpGUID, sizeof(GUID));
    }

    // Add subnode to treeview
    if (lpDriverName && *lpDriverName)
        sprintf_s(szText, sizeof(szText), "%s (%s)", lpDriverDesc, lpDriverName);
    else
        sprintf_s(szText, sizeof(szText), lpDriverDesc);
    szText[255] = TEXT('\0');

    DSCCapDefs[0].strName = szText;
    AddCapsToTV(hParent, DSCCapDefs, (LPARAM)lpTemp);

    return(DDENUMRET_OK);
}




//-----------------------------------------------------------------------------
// Name: DS_Init()
// Desc: 
//-----------------------------------------------------------------------------
VOID DS_Init()
{
}



//-----------------------------------------------------------------------------
// Name: DS_FillTree()
// Desc: 
//-----------------------------------------------------------------------------
VOID DS_FillTree( HWND hwndTV )
{
    HTREEITEM hTree;

    // Add DirectSound devices
    hTree = TVAddNode( TVI_ROOT, "DirectSound Devices", TRUE, IDI_DIRECTX,
		               NULL, 0, 0 );
    DirectSoundEnumerate( DSEnumCallBack, hTree );

    TreeView_Expand( hwndTV, hTree, TVE_EXPAND );

    // Add DirectSound capture devices
    hTree = TVAddNode( TVI_ROOT, "DirectSoundCapture Devices", TRUE,
		               IDI_DIRECTX, NULL, 0, 0 );
    DirectSoundCaptureEnumerate( DSCEnumCallBack, hTree );
    TreeView_Expand( hwndTV, hTree, TVE_EXPAND );
}




//-----------------------------------------------------------------------------
// Name: DS_CleanUp()
// Desc: 
//-----------------------------------------------------------------------------
VOID DS_CleanUp()
{
    if( g_pDSound )
        g_pDSound->Release();

    if( g_pDSCapture )
        g_pDSCapture->Release();
}





