//-----------------------------------------------------------------------------
// Name: DXView.h
//
// Desc: DirectX Capabilities Viewer Common Header
//
// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?linkid=2136896
//-----------------------------------------------------------------------------
#include <Windows.h>

#include <mmsystem.h>
#include <commctrl.h>
#include <tchar.h>

#include <ctime>
#include <cstdio>
#include <iterator>
#include <new>

#include "resource.h"

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define DXView_WIDTH    800          // Window dimensions
#define DXView_HEIGHT   640
#define DEF_TAB_SIZE    3

constexpr int c_DefNameLength = 30;

#define IDC_LV          0x2000       // Child controls
#define IDC_TV          0x2003

#define IDI_FIRSTIMAGE  IDI_DIRECTX  // Imagelist first and last icons
#define IDI_LASTIMAGE   IDI_CAPSOPEN

#define TIMER_PERIOD	500

#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=nullptr; } }


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

using DISPLAYCALLBACK = HRESULT(*)(LPARAM lParam1, LPARAM lParam2, _In_opt_ PRINTCBINFO* pPrintInfo);
using DISPLAYCALLBACKEX = HRESULT(*)(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, _In_opt_ PRINTCBINFO* pPrintInfo);

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
    const CHAR*  strName;        // Name of cap
    LONG         dwOffset;       // Offset to cap
    DWORD        dwFlag;         // Bit flag for cal
    DWORD        dwCapsFlags;	   // used for optional caps and such (see DXV_ values above)
};

struct CAPDEFS
{
    const CHAR*           strName;        // Name of cap
    DISPLAYCALLBACK       fnDisplayCallback;
    LPARAM                lParam2;
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
HRESULT PrintCapsToDC( CAPDEF* pcd, VOID* pv, _In_ PRINTCBINFO* pInfo );

// Printer Helper functions
HRESULT PrintLine(int x, int y, _In_count_(cchBuff) LPCTSTR lpszBuff, size_t cchBuff, _In_ PRINTCBINFO* pci);
HRESULT PrintNextLine(_In_ PRINTCBINFO* pci );
HRESULT PrintValueLine(_In_z_ const char* szText, DWORD dwValue, _In_ PRINTCBINFO* lpInfo);
HRESULT PrintHexValueLine(_In_z_ const CHAR* szText, DWORD dwValue, _In_ PRINTCBINFO* lpInfo);
HRESULT PrintStringValueLine(_In_z_ const CHAR* szText, const CHAR* szText2, _In_ PRINTCBINFO* lpInfo);
HRESULT PrintStringLine(_In_z_ const CHAR* szText, _In_ PRINTCBINFO* lpInfo);


//-----------------------------------------------------------------------------
// DXView external variables
//-----------------------------------------------------------------------------
extern HINSTANCE g_hInstance;
extern HWND      g_hwndMain;
extern HWND      g_hwndLV;        // List view
