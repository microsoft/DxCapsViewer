//-----------------------------------------------------------------------------
// Name: DInput.cpp
//
// Desc: DirectX caps viewer - DirectInput module
//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#include "dxview.h"

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include <stdio.h>

#define DICAPDEF(name,val,flag) {name, FIELD_OFFSET(DIDEVCAPS,val), flag}
#define DIVALDEF(name,val)      {name, FIELD_OFFSET(DIDEVCAPS,val), 0}




//-----------------------------------------------------------------------------
// Name: struct DI3Info
// Desc: Structure that describes the caps maintained by DIDEVCAPS
//       that existed in DirectX 3.
//-----------------------------------------------------------------------------
CAPDEF DI3Info[] =
{
    DIVALDEF("dwAxes",                   dwAxes),
    DIVALDEF("dwButtons",                dwButtons),
    DIVALDEF("dwPOVs",                   dwPOVs),

    DICAPDEF("DIDC_ATTACHED",            dwFlags,        DIDC_ATTACHED),
    DICAPDEF("DIDC_POLLEDDEVICE",        dwFlags,        DIDC_POLLEDDEVICE),
    DICAPDEF("DIDC_EMULATED",            dwFlags,        DIDC_EMULATED),

    {"", 0, 0}
};




//-----------------------------------------------------------------------------
// Name: struct DI5Info
// Desc: Structure that describes the caps maintained by DIDEVCAPS
//       that exist in DirectX 5 and were't in DirectX 3.
//-----------------------------------------------------------------------------
CAPDEF DI5Info[] =
{
    DICAPDEF("DIDC_FORCEFEEDBACK",       dwFlags,        DIDC_FORCEFEEDBACK),
    DICAPDEF("DIDC_FFATTACK",            dwFlags,        DIDC_FFATTACK),
    DICAPDEF("DIDC_FFFADE",              dwFlags,        DIDC_FFFADE),
    DICAPDEF("DIDC_SATURATION",          dwFlags,        DIDC_SATURATION),
    DICAPDEF("DIDC_POSNEGCOEFFICIENTS",  dwFlags,        DIDC_POSNEGCOEFFICIENTS),
    DICAPDEF("DIDC_POSNEGSATURATION",    dwFlags,        DIDC_POSNEGSATURATION),

    DIVALDEF("dwFFSamplePeriod",         dwFFSamplePeriod),
    DIVALDEF("dwFFMinTimeResolution",    dwFFMinTimeResolution),
    {"", 0, 0}
};




//-----------------------------------------------------------------------------
// Name: struct DISubTypeList
// Desc: List of various DirectInput device subtypes and their names.
//-----------------------------------------------------------------------------
struct SUBTYPEINFO
{
    DWORD   dwDevType;
    LPCTSTR strName;
};

#define SUBTYPE(type, subtype, name)    \
{   MAKEWORD(DIDEVTYPE_##type, \
    DIDEVTYPE##type##_##subtype), \
TEXT("%d - ") TEXT(name) }

SUBTYPEINFO DISubTypes[] =
{
    SUBTYPE(MOUSE,          UNKNOWN,            "Unknown"),
    SUBTYPE(MOUSE,          TRADITIONAL,        "Traditional"),
    SUBTYPE(MOUSE,          FINGERSTICK,        "Fingerstick"),
    SUBTYPE(MOUSE,          TOUCHPAD,           "Touchpad"),
    SUBTYPE(MOUSE,          TRACKBALL,          "Trackball"),

    SUBTYPE(KEYBOARD,       UNKNOWN,            "Unknown"),
    SUBTYPE(KEYBOARD,       PCXT,               "XT"),
    SUBTYPE(KEYBOARD,       OLIVETTI,           "Olivetti"),
    SUBTYPE(KEYBOARD,       PCAT,               "AT"),
    SUBTYPE(KEYBOARD,       PCENH,              "Enhanced"),
    SUBTYPE(KEYBOARD,       NOKIA1050,          "Nokia 1050"),
    SUBTYPE(KEYBOARD,       NOKIA9140,          "Nokia 9140"),
    SUBTYPE(KEYBOARD,       NEC98,              "NEC98"),
    SUBTYPE(KEYBOARD,       NEC98LAPTOP,        "NEC98 Laptop"),
    SUBTYPE(KEYBOARD,       NEC98106,           "NEC98 106"),
    SUBTYPE(KEYBOARD,       JAPAN106,           "Japan 106"),
    SUBTYPE(KEYBOARD,       JAPANAX,            "Japan AX"),
    SUBTYPE(KEYBOARD,       J3100,              "J3100"),

    SUBTYPE(JOYSTICK,       UNKNOWN,            "Unknown"),
    SUBTYPE(JOYSTICK,       TRADITIONAL,        "Traditional"),
    SUBTYPE(JOYSTICK,       FLIGHTSTICK,        "Flightstick"),
    SUBTYPE(JOYSTICK,       GAMEPAD,            "Gamepad"),
    SUBTYPE(JOYSTICK,       RUDDER,             "Rudder"),
    SUBTYPE(JOYSTICK,       WHEEL,              "Wheel"),
    SUBTYPE(JOYSTICK,       HEADTRACKER,        "Head tracker"),
    { 0, 0 },
};




//-----------------------------------------------------------------------------
// Name: DI_CreateDI()
// Desc: Create a DirectInput pointer.
//-----------------------------------------------------------------------------
LPDIRECTINPUT DI_CreateDI()
{
    LPDIRECTINPUT pDI;
    
    if( FAILED( DirectInputCreate( g_hInstance, DIRECTINPUT_VERSION,
                                   &pDI, NULL ) ) )
    {
        return NULL;
    }
    
    return pDI;
}




//-----------------------------------------------------------------------------
// Name: DI_CreateDevice()
// Desc: Create a device with the specified instance GUID.
//-----------------------------------------------------------------------------
LPDIRECTINPUTDEVICE DI_CreateDevice( const GUID* pGUID )
{
    LPDIRECTINPUT       pDI;
    LPDIRECTINPUTDEVICE pdidDevice;
    
    pDI = DI_CreateDI();
    if( NULL == pDI )
        return NULL;

    if( FAILED( pDI->CreateDevice( *pGUID, &pdidDevice, NULL ) ) )
    {
        pDI->Release();
        return NULL;
    }

    pDI->Release();
    return pdidDevice;
}




//-----------------------------------------------------------------------------
// Name: DIAddRow()
// Desc: Add a row to the growing two-column listview or printer.
//       - pInfo = print context or NULL if adding to listview
//       - strName = name of cap
//       - strFormat = wsprintf-style format string
//       - ... = inserts for wsprintf
//-----------------------------------------------------------------------------
HRESULT DIAddRow( PRINTCBINFO* pInfo, const TCHAR* strName,
                  const TCHAR* strFormat, ... )
{
    TCHAR   strBuf[1024];
    int     cch;
    va_list ap;
    
    va_start( ap, strFormat );
    cch = vsprintf_s( strBuf, sizeof(strBuf), strFormat, ap );
    va_end( ap );
    strBuf[1023] = TEXT('\0');
    
    if( pInfo == NULL )
    {
        LVAddText( g_hwndLV, 0, strName, 0 );
        LVAddText( g_hwndLV, 1, TEXT("%s"), strBuf );
    }
    else
    {
        int xName, xVal, yLine;
        
        // Calculate Name and Value column x offsets
        xName = (pInfo->dwCurrIndent * DEF_TAB_SIZE * pInfo->dwCharWidth);
        xVal  = xName + (32 * pInfo->dwCharWidth);
        yLine = pInfo->dwCurrLine * pInfo->dwLineHeight;
        
        // Print name
        if( FAILED( PrintLine( xName, yLine, strName, strlen(strName), pInfo ) ) )
            return E_FAIL;
        
        // Print value
        if( FAILED( PrintLine( xVal, yLine, strBuf, cch, pInfo ) ) )
            return E_FAIL;
        
        // Advance to next line on page
        if( FAILED( PrintNextLine( pInfo ) ) )
            return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DIAddTypes()
// Desc: Emit the device type information to the output device.
//       - lpInfo = print context or NULL if adding to listview
//       - dwDevType = device type to decode
//-----------------------------------------------------------------------------
HRESULT DIAddTypes( PRINTCBINFO* pInfo, DWORD dwDevType )
{
    HRESULT      hr;
    DWORD        dwType;
    LPCTSTR      strValue;
    SUBTYPEINFO* psti;
    
    // Add the type code.
    dwType = GET_DIDEVICE_TYPE(dwDevType);
    switch( dwType )
    {
        case DIDEVTYPE_MOUSE:    strValue = TEXT("%d - Mouse");    break;
        case DIDEVTYPE_KEYBOARD: strValue = TEXT("%d - Keyboard"); break;
        case DIDEVTYPE_JOYSTICK: strValue = TEXT("%d - Joystick"); break;
        default:                 strValue = TEXT("%d");            break;
    }
    
    hr = DIAddRow( pInfo, "Type", strValue, dwType );
    if( FAILED(hr) )
        return E_FAIL;
    
    // Add the sub type code.
    strValue = TEXT("%d");
    for( psti = DISubTypes; psti->dwDevType; psti++ )
    {
        if( psti->dwDevType == (dwDevType & 0xFFFF) )
        {
            strValue = psti->strName;
            break;
        }
    }
    
    hr = DIAddRow( pInfo, TEXT("Subtype"), strValue,
                   GET_DIDEVICE_SUBTYPE(dwDevType) );
    if( FAILED(hr) )
        return E_FAIL;
    
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DIAddCapsToTarget()
// Desc: Add the caps either to the listview or to the printer.
//       - lpInfo = print context or NULL if adding to listview
//       - pcd = pointer to CAPDEF array describing the caps
//       - pv = pointer to structure to be parsed
//-----------------------------------------------------------------------------
HRESULT DIAddCapsToTarget( PRINTCBINFO* pInfo, CAPDEF* pcd, VOID* pv )
{
    if( pInfo )
        return PrintCapsToDC( pcd, pv, pInfo );

    AddMoreCapsToLV( pcd, pv );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DIDisplayCaps()
// Desc: lpInfo = print context or NULL if adding to listview
//       lParam1 = state info recorded by DIEnumDevCallback (LPGUID)
//       lParam2 = refdata (not used)
//-----------------------------------------------------------------------------
HRESULT DIDisplayCaps( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo )
{
    LPDIRECTINPUTDEVICE pdidDevice;
    GUID*     pGUID = (GUID*)lParam1;
    DIDEVCAPS caps;
    HRESULT   hr;
    
    if( pInfo == NULL )
        AddColsToLV();
    
    pdidDevice = DI_CreateDevice( pGUID );
    
    if( NULL == pdidDevice )
        return E_FAIL;

        
    // First use the DX3 caps.
    caps.dwSize = sizeof(DIDEVCAPS_DX3);
    
    hr = pdidDevice->GetCapabilities( &caps );
    if( SUCCEEDED(hr) )
    {
        hr = DIAddTypes( pInfo, caps.dwDevType );
        if( SUCCEEDED(hr) )
        {
            hr = DIAddCapsToTarget( pInfo, DI3Info, &caps );
    
            if( SUCCEEDED(hr) )
            {
                // Now get the DX5 caps if we haven't cancelled printing yet.
                caps.dwSize = sizeof(DIDEVCAPS);
        
                hr = pdidDevice->GetCapabilities( &caps );
                if( SUCCEEDED(hr) )
                    hr = DIAddCapsToTarget( pInfo, DI5Info, &caps );
            }
        }
    }
    
    pdidDevice->Release();

    return hr;
}




//-----------------------------------------------------------------------------
// Name: DIEnumEffCallback()
// Desc: Add the enumerated DirectInput effect to the listview.
//-----------------------------------------------------------------------------
BOOL CALLBACK DIEnumEffCallback( const DIEFFECTINFO* pei, VOID* pInfo )
{
    const GUID* pGUID = &pei->guid;
    HRESULT     hr;
    
    hr = DIAddRow( (PRINTCBINFO*)pInfo, pei->tszName,
                   TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
                   pGUID->Data1, pGUID->Data2, pGUID->Data3,
                   pGUID->Data4[0], pGUID->Data4[1],
                   pGUID->Data4[2], pGUID->Data4[3],
                   pGUID->Data4[4], pGUID->Data4[5],
                   pGUID->Data4[6], pGUID->Data4[7] );
   
    if( FAILED(hr) )
        return DIENUM_STOP;

    return DIENUM_CONTINUE;
}




//-----------------------------------------------------------------------------
// Name: DIDisplayEffects()
// Desc: lpInfo = print context or NULL if adding to listview
//       lParam1 = state info recorded by DIEnumDevCallback (LPGUID)
//       lParam2 = refdata (not used)
//-----------------------------------------------------------------------------
HRESULT DIDisplayEffects( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pInfo )
{
    GUID* pGUID    = (GUID*)lParam1;
    LPDIRECTINPUTDEVICE pdidDevice;

    // The effects are not a simple name/value thing. But DXView doesn't like
    // multiple nesting levels, so we mash it into the name/value paradigm
    // because I'm lazy. 

    if( pInfo == NULL )
        AddColsToLV();

    pdidDevice = DI_CreateDevice( pGUID );

    if( pdidDevice )
    {
        LPDIRECTINPUTDEVICE2 pdidDevice2;
        HRESULT hr;
        
        hr = pdidDevice->QueryInterface( IID_IDirectInputDevice2, (VOID**)&pdidDevice2 );
        
        if( SUCCEEDED(hr) )
        {
            // Enumerate the effects and add them to the listview.
            pdidDevice2->EnumEffects( DIEnumEffCallback, pInfo, DIEFT_ALL );
            
            pdidDevice2->Release();
        }
        
        pdidDevice->Release();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAPDEFS DICapDefs[] =
{
    { "",                    DIDisplayCaps,     0,      },
    { "Effects",             DIDisplayEffects,  0,      },
    { NULL, 0, 0 }
};




//-----------------------------------------------------------------------------
// Name: DIEnumDevCallback()
// Desc: Add the enumerated DirectInput device to the treeview.
//-----------------------------------------------------------------------------
BOOL CALLBACK DIEnumDevCallback( const DIDEVICEINSTANCE* pinst, VOID* pv )
{
    HTREEITEM hParent = (HTREEITEM)pv;
    TCHAR     strText[MAX_PATH + 2 + MAX_PATH + 2];
    GUID*     pGUID;

    pGUID = (GUID*)LocalAlloc( LPTR, sizeof(GUID) );
    if( pGUID == NULL )
        return DIENUM_STOP;

    *pGUID = pinst->guidInstance;

    sprintf_s( strText, sizeof(strText), "%s (%s)", pinst->tszInstanceName,
                                  pinst->tszProductName );

    DICapDefs[0].strName = strText;
    AddCapsToTV( hParent, DICapDefs, (LPARAM)pGUID );

    return DIENUM_CONTINUE;
}




//-----------------------------------------------------------------------------
// Name: DI_Init()
// Desc: 
//-----------------------------------------------------------------------------
VOID DI_Init()
{
}




//-----------------------------------------------------------------------------
// Name: DI_FillTree()
// Desc: Add the DirectInput nodes to the treeview.
//-----------------------------------------------------------------------------
VOID DI_FillTree( HWND hwndTV )
{
    // Add direct input devices if DInput is found
    
    LPDIRECTINPUT pDI = DI_CreateDI();
    if( pDI )
    {       
        HTREEITEM hTree = TVAddNode( TVI_ROOT, "DirectInput Devices", TRUE,
                                     IDI_DIRECTX, NULL, 0, 0 );
        
        pDI->EnumDevices( 0, DIEnumDevCallback, hTree, DIEDFL_ALLDEVICES );
        
        TreeView_Expand( hwndTV, hTree, TVE_EXPAND );
        
        pDI->Release();
    }
}

