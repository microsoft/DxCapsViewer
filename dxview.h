//-----------------------------------------------------------------------------
// Name: DXView.h
//
// Desc: DirectX Device Viewer - Common header file
//
//@@BEGIN_MSINTERNAL
//
// Hist: 05.24.99 - mwetzel - I didn't write this, I'm just cleaning it up, so
//                            don't complain to me how horrid this app is.
//
//@@END_MSINTERNAL
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <tchar.h>
#include <time.h>
#include "resource.h"
#include <stdio.h>

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define DXView_WIDTH    700          // Window dimensions
#define DXView_HEIGHT   400
#define DEF_TAB_SIZE    3

#define IDC_LV          0x2000       // Child controls
#define IDC_TV          0x2003

#define IDI_FIRSTIMAGE  IDI_DIRECTX  // Imagelist first and last icons
#define IDI_LASTIMAGE   IDI_CAPSOPEN

#define TIMER_PERIOD	500




//-----------------------------------------------------------------------------
// Structs and typedefs
//-----------------------------------------------------------------------------
struct PRINTCBINFO
{
    HDC         hdcPrint;       // In:      Printer DC
    HWND        hTreeWnd;       // In:      tree window
    HTREEITEM   hCurrTree;      // In:      current tree item handle
    DWORD       dwCharWidth;    // In:      average char width
    DWORD       dwLineHeight;   // In:      max line height
    DWORD       dwCurrLine;     // In/Out:  curr line position on page
    DWORD       dwCharsPerLine; // In:      maximum chars per line (based on avg. char width)
    DWORD       dwLinesPerPage; // In:      maximum lines per page
    DWORD       dwCurrIndent;   // In:      Current tab setting
    BOOL        fStartPage;     // In/Out:  need to a start new page ?!?
};

typedef HRESULT (*DISPLAYCALLBACK)( LPARAM lParam1, LPARAM lParam2, PRINTCBINFO* pPrintInfo );
typedef HRESULT (*DISPLAYCALLBACKEX)( LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo );

struct NODEINFO
{
    DISPLAYCALLBACK fnDisplayCallback;
    BOOL            bUseLParam3;
    LPARAM          lParam1;
    LPARAM          lParam2;
    LPARAM          lParam3;
};

#define DXV_9EXCAP (1<<0)

struct CAPDEF
{
    CHAR*  strName;        // Name of cap
    DWORD  dwOffset;       // Offset to cap
    DWORD  dwFlag;         // Bit flag for cal
    DWORD  dwCapsFlags;	   // used for optional caps and such (see DXV_ values above)
};

struct CAPDEFS
{
    CHAR*           strName;        // Name of cap
    DISPLAYCALLBACK fnDisplayCallback;
    LPARAM          lParam2;
};


struct LV_INSTANCEGUIDSTRUCT
{
    GUID	guidInstance;
    DWORD	dwRefresh;
};


struct LOCALAPP
{
    LOCALAPP* pNext;
    GUID      guidApplication;
    CHAR      strAppNameA[1];
};




//-----------------------------------------------------------------------------
// DXView treeview/listview helper functions
//-----------------------------------------------------------------------------
VOID    LVAddColumn( HWND hwndLV, int i, const CHAR* strName, int width );
int     LVAddText( HWND hwndLV, int col, const CHAR* str, ... );
VOID    LVDeleteAllItems( HWND hwndLV );
HTREEITEM TVAddNode( HTREEITEM hParent, LPCSTR strText, BOOL bKids, int iImage, 
                     DISPLAYCALLBACK Callback, LPARAM lParam1, LPARAM lParam2 );
HTREEITEM TVAddNodeEx( HTREEITEM hParent, LPCSTR strText, BOOL bKids, int iImage, 
                     DISPLAYCALLBACKEX Callback, LPARAM lParam1, LPARAM lParam2, 
                     LPARAM lParam3 );
VOID    AddCapsToTV( HTREEITEM hParent, CAPDEFS *pcds, LPARAM lParam1 );
VOID    AddColsToLV();
VOID    AddCapsToLV( CAPDEF* pcd, VOID* pv );
VOID    AddMoreCapsToLV( CAPDEF* pcd, VOID* pv );
HRESULT PrintCapsToDC( CAPDEF* pcd, VOID* pv, PRINTCBINFO* pInfo );

// Printer Helper functions
HRESULT PrintStartPage( PRINTCBINFO* pci );
HRESULT PrintEndPage( PRINTCBINFO* pci );
HRESULT PrintLine( int x, int y, LPCTSTR lpszBuff, DWORD cchBuff, PRINTCBINFO* pci );
HRESULT PrintNextLine( PRINTCBINFO* pci );




//-----------------------------------------------------------------------------
// DXView external variables
//-----------------------------------------------------------------------------
extern HINSTANCE g_hInstance;
extern HWND      g_hwndMain;
extern HWND      g_hwndLV;        // List view
