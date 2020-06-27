//-----------------------------------------------------------------------------
// Name: DXView.cpp
//
// Desc: DirectX caps viewer
//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#include "dxview.h"
#include <objbase.h>
#include <stdio.h>
#include <d3d8.h> // for D3DSHADER_VERSION_MAJOR and D3DSHADER_VERSION_MINOR
#include <strsafe.h>
#include <shlwapi.h>
#include <htmlhelp.h>

HINSTANCE   g_hInstance;
CHAR        g_strAppName[]  = "DXView";
CHAR        g_strClassName[] = "DXView";
CHAR        g_strTitle[]     = "DirectX Caps Viewer";
HWND        g_hwndMain;

HWND        g_hwndLV;        // List view
HWND        g_hwndTV;        // Tree view
HIMAGELIST  g_hImageList;
HFONT       g_hFont;
int         g_xPaneSplit;
int         g_xHalfSplitWidth;
BOOL        g_bSplitMove;
DWORD       g_dwViewState;
DWORD		g_dwView9Ex;
DWORD       g_tmAveCharWidth;
extern BOOL g_PrintToFile;
extern TCHAR  g_PrintToFilePath[MAX_PATH];
CHAR        g_szClip[200];   // Text to possibly copy to clipboard
TCHAR       g_helpPath[MAX_PATH] = {0};

//-----------------------------------------------------------------------------
// Local function prototypes
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    DXView_OnCreate( HWND hwnd );
VOID    DXView_OnCommand( HWND hwnd, WPARAM wParam );
VOID    DXView_OnSize( HWND hwnd );
VOID    DXView_OnTreeSelect( HWND hwndTV, NM_TREEVIEW* ptv );
VOID    DXView_OnListViewDblClick( HWND hwndLV, NM_LISTVIEW* plv );
VOID    DXView_Cleanup();
BOOL    DXView_InitImageList();
BOOL    DXView_OnPrint( HWND hWindow, HWND hTreeView, BOOL bPrintAll );
BOOL    DXView_OnFile( HWND hWindow, HWND hTreeWnd,BOOL bPrintAll );
VOID    CreateCopyMenu( VOID );

VOID    FindHelp();
VOID    InvokeHelp();



//-----------------------------------------------------------------------------
// External function prototypes
//-----------------------------------------------------------------------------

VOID DXGI_FillTree( HWND hwndTV );
VOID DXG_FillTree( HWND hwndTV );
VOID DD_FillTree( HWND hwndTV );

VOID DXGI_Init();
VOID DXG_Init();
VOID DD_Init();

VOID DXGI_CleanUp();
VOID DXG_CleanUp();
VOID DD_CleanUp();

BOOL DXG_Is9Ex();



//-----------------------------------------------------------------------------
// Name: Int2Str()
// Desc: Get number as a string
//-----------------------------------------------------------------------------
HRESULT Int2Str( _Out_z_cap_(nDestLen) LPTSTR strDest, UINT nDestLen, DWORD i )
{
    char  strIn[32];
    char  strOut[32];
    char  strDec[2];
    char* pstrDec;

    *strDest = 0;

    GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, strDec, 2 );
    sprintf_s( strIn, sizeof(strIn), "%u",i );
    if( 0 == GetNumberFormat( LOCALE_USER_DEFAULT, 0, strIn, NULL, strOut, 32 ) )
    {
        strcpy_s( strDest, nDestLen, strIn );
        return E_FAIL;
    }
    pstrDec = strrchr( strOut, strDec[0] );
    if( pstrDec != NULL )
        *pstrDec = '\0';
    
    if( strcpy_s( strDest, nDestLen, strOut ) != 0)
    {
        *strDest = 0;
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT PrintStringValueLine(const char * szText, const char * szText2, PRINTCBINFO *lpInfo)
{
    char  szBuff[80];
    DWORD cchLen;
    int   xName, xVal, yLine;

    // Calculate Name and Value column x offsets
    xName   = (lpInfo->dwCurrIndent * DEF_TAB_SIZE * lpInfo->dwCharWidth);
    xVal    = xName + (32 * lpInfo->dwCharWidth);
    yLine   = (lpInfo->dwCurrLine * lpInfo->dwLineHeight);

    // Print name
    strcpy_s(szBuff, sizeof(szBuff), szText);
    szBuff[79] = '\0';
    cchLen = _tcslen (szText);
    if( FAILED( PrintLine (xName, yLine, szBuff, cchLen, lpInfo) ) )
        return E_FAIL;

    // Print value
    strcpy_s(szBuff, sizeof(szBuff), szText2);
    szBuff[79] = '\0';
    cchLen = _tcslen (szText2);
    if( FAILED( PrintLine (xVal, yLine, szBuff, cchLen, lpInfo) ) )
        return E_FAIL;

    // Advance to next line on page
    if( FAILED( PrintNextLine(lpInfo) ) )
        return E_FAIL;

   return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT PrintValueLine(const char * szText, DWORD dwValue, PRINTCBINFO *lpInfo)
{
    char  szBuff[80];
    Int2Str(szBuff, 80, dwValue);
    return PrintStringValueLine( szText, szBuff, lpInfo );
}


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT PrintHexValueLine(const char * szText, DWORD dwValue, PRINTCBINFO *lpInfo)
{
    char  szBuff[80];
    sprintf_s( szBuff, sizeof(szBuff), "0x%08x", dwValue );
    return PrintStringValueLine( szText, szBuff, lpInfo );
}


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT PrintStringLine(const char * szText, PRINTCBINFO *lpInfo)
{
    char  szBuff[80];
    DWORD cchLen;
    int   xName, yLine;

    // Calculate Name and Value column x offsets
    xName   = (lpInfo->dwCurrIndent * DEF_TAB_SIZE * lpInfo->dwCharWidth);
    yLine   = (lpInfo->dwCurrLine * lpInfo->dwLineHeight);

    // Print name
    strcpy_s(szBuff, sizeof(szBuff), szText);
    szBuff[79] = '\0';
    cchLen = _tcslen (szText);
    if( FAILED( PrintLine (xName, yLine, szBuff, cchLen, lpInfo) ) )
        return E_FAIL;

    // Advance to next line on page
    if( FAILED( PrintNextLine(lpInfo) ) )
        return E_FAIL;

   return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPSTR strCmdLine, _In_ int nCmdShow )
{
    g_hInstance = hInstance; // Store instance handle in our global variable
    g_PrintToFilePath[0] = TEXT('\0');

    // Initialize COM
    CoInitialize( NULL );

    FindHelp();

    // Init various DX components
    DXGI_Init();
    DXG_Init();
    DD_Init();

    // Register window class
    WNDCLASS  wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW; // Class style(s).
    wc.lpfnWndProc   = (WNDPROC)WndProc;        // Window Procedure
    wc.cbClsExtra    = 0;                       // No per-class extra data.
    wc.cbWndExtra    = 0;                       // No per-window extra data.
    wc.hInstance     = hInstance;               // Owner of this class
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTX)); // Icon name from .RC
    wc.hCursor       = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SPLIT));// Cursor
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1); // Default color
    wc.lpszMenuName  = "Menu";                   // Menu name from .RC
    wc.lpszClassName = g_strClassName;            // Name to register as
    RegisterClass( &wc );

    // Create a main window for this application instance.
    g_hwndMain = CreateWindowEx( 0, g_strClassName, g_strTitle, WS_OVERLAPPEDWINDOW, 
                                 CW_USEDEFAULT, CW_USEDEFAULT, DXView_WIDTH, DXView_HEIGHT, 
                                 NULL, NULL, hInstance, NULL );

    // If window could not be created, return "failure"
    if( NULL==g_hwndMain )
    {
        CoUninitialize();
        return -1;
    }

    TCHAR* pszCmdLine = GetCommandLine();
    // Skip past program name (first token in command line).
    if (*pszCmdLine == TEXT('"'))  // Check for and handle quoted program name
    {
        pszCmdLine++;
        // Scan, and skip over, subsequent characters until  another
        // double-quote or a null is encountered
        while (*pszCmdLine && (*pszCmdLine != TEXT('"')))
            pszCmdLine++;
        // If we stopped on a double-quote (usual case), skip over it.
        if (*pszCmdLine == TEXT('"'))            
            pszCmdLine++;    
    }
    else    // First token wasn't a quote
    {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    // Skip past any white space preceeding the next token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
        pszCmdLine++;

    // Treat the rest of the command line as a filename to save the whole tree to
    TCHAR* pstrSave = g_PrintToFilePath;
    if (*pszCmdLine == TEXT('"'))  // Check for and handle quoted program name
    {
        pszCmdLine++;
        // Scan, and copy, subsequent characters until  another
        // double-quote or a null is encountered
        while (*pszCmdLine && (*pszCmdLine != TEXT('"')))
            *pstrSave++ = *pszCmdLine++;
        // If we stopped on a double-quote (usual case), skip over it.
        if (*pszCmdLine == TEXT('"'))            
            pszCmdLine++;    
    }
    else    // First token wasn't a quote
    {
        while (*pszCmdLine > TEXT(' '))
            *pstrSave++ = *pszCmdLine++;
    }
    *pstrSave = TEXT('\0');

    if( strlen( g_PrintToFilePath ) > 0 )
    {
        PostMessage( g_hwndMain, WM_COMMAND, IDM_PRINTWHOLETREETOFILE, 0 );
        PostMessage( g_hwndMain, WM_CLOSE, 0, 0 );
    }
    else
    {
        // Make the window visible; update its client area; and return "success"
        ShowWindow( g_hwndMain, SW_MAXIMIZE /*nCmdShow*/ );
    }

    // Message pump
    MSG msg;
    while( GetMessage(&msg, NULL, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    CoUninitialize();

    return (int)msg.wParam;
}




//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Message handler
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_CREATE:
            return DXView_OnCreate(hWnd);

        case WM_SIZE:
            DXView_OnSize(hWnd);
            break;

        case WM_LBUTTONDOWN:
            g_bSplitMove = TRUE;
            SetCapture(hWnd);
            break;

        case WM_LBUTTONUP:
            g_bSplitMove = FALSE;
            ReleaseCapture();
            break;

        case WM_MOUSEMOVE:
            if(g_bSplitMove)
            {
                RECT    rect;
                // change the value from unsigned to signed
                int     x = (int)(short)LOWORD(lParam);

                GetClientRect(hWnd, &rect);
                if (rect.left > x)
                {
                    x = rect.left;
                }
                else if (rect.right < x)
                {
                    x = rect.right;
                }
                g_xPaneSplit = (x - g_xHalfSplitWidth);
                DXView_OnSize(hWnd);
            }
            break;

        case WM_NOTIFY:
            if (((NMHDR*)lParam)->hwndFrom == g_hwndTV)
            {
                if (((NMHDR*)lParam)->code == TVN_SELCHANGED)
                    DXView_OnTreeSelect(g_hwndTV, (NM_TREEVIEW*)lParam);
                else if (((NMHDR*)lParam)->code == NM_RCLICK)
                {
                    NMHDR* pnmhdr = (NMHDR*)lParam;
                    TVHITTESTINFO info;
                    GetCursorPos(&info.pt);
                    ScreenToClient(pnmhdr->hwndFrom, &info.pt);
                    TreeView_HitTest(pnmhdr->hwndFrom, &info);
                    if (info.flags & TVHT_ONITEMLABEL)
                    {
                        TVITEM tvitem;
                        tvitem.mask = TVIF_TEXT;
                        tvitem.hItem = info.hItem;
                        tvitem.pszText = g_szClip;
                        tvitem.cchTextMax = 200;
                        TreeView_GetItem(g_hwndTV, &tvitem);
                        CreateCopyMenu();
                    }
                }
                else if(((NMHDR*)lParam)->code == TVN_KEYDOWN)
                {
                     NMTVKEYDOWN* ptvkd = (LPNMTVKEYDOWN)lParam;
                     if( ptvkd->wVKey == VK_TAB )
                         SetFocus( g_hwndLV );
                }
            }

            if (((NMHDR*)lParam)->hwndFrom == g_hwndLV)
            {
                if (((NMHDR*)lParam)->code == NM_RDBLCLK)
                    DXView_OnListViewDblClick(g_hwndLV, (NM_LISTVIEW*)lParam);
                else if (((NMHDR*)lParam)->code == NM_RCLICK)
                {
                    NMHDR* pnmhdr = (NMHDR*)lParam;
                    LVHITTESTINFO info;
                    GetCursorPos(&info.pt);
                    ScreenToClient(pnmhdr->hwndFrom, &info.pt);
                    ListView_SubItemHitTest(pnmhdr->hwndFrom, &info);
                    if (info.iItem >= 0)
                    {
                        ListView_GetItemText(g_hwndLV, info.iItem, info.iSubItem, g_szClip, 200);
                        CreateCopyMenu();
                    }
                }
                else if(((NMHDR*)lParam)->code == LVN_KEYDOWN)
                {
                     NMLVKEYDOWN* plvkd = (LPNMLVKEYDOWN)lParam;
                     if( plvkd->wVKey == VK_TAB )
                         SetFocus( g_hwndTV );
                }
            }

            break;

        case WM_SETFOCUS:
            SetFocus( g_hwndTV );
            break;

       case WM_COMMAND:  // message: command from application menu
            DXView_OnCommand(hWnd, wParam);
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:  // message: window being destroyed
            DXView_Cleanup();  // Free per item struct for all items
            PostQuitMessage(0);
            break;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID CreateCopyMenu(VOID)
{
    HMENU hPopupMenu;
    HMENU hMenu;
    MENUITEMINFO mii;
    POINT pt;
    TCHAR szMenu[200];
    TCHAR szMenuNew[200];

    hPopupMenu = LoadMenu(NULL, "POPUP");
    if( hPopupMenu == NULL )
        return;
    hMenu = GetSubMenu(hPopupMenu, 0);
    if( hMenu == NULL )
    {
        DestroyMenu(hPopupMenu);
        return;
    }
    GetMenuString(hMenu, IDM_COPY, szMenu, 200, MF_BYCOMMAND);
    sprintf_s(szMenuNew, sizeof(szMenuNew), szMenu, g_szClip);
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = szMenuNew;
    SetMenuItemInfo(hMenu, IDM_COPY, FALSE, &mii);
    GetCursorPos(&pt);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hwndMain, NULL);
}

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL DXView_OnCreate(HWND hWnd)
{
    HDC hDC;
    int PixelsPerInch;
    TEXTMETRIC tm;

    hDC = GetDC(hWnd);
    PixelsPerInch = GetDeviceCaps(hDC, LOGPIXELSX);

    NONCLIENTMETRICS nmi;
    nmi.cbSize = sizeof(nmi);

    (void)SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nmi, 0 );
    g_hFont = CreateFontIndirect( &nmi.lfMessageFont );


    SelectObject(hDC, g_hFont);
    GetTextMetrics(hDC, &tm);
    g_tmAveCharWidth = tm.tmAveCharWidth;
    ReleaseDC(hWnd, hDC);

    // Initialize global data
    g_dwViewState = IDM_VIEWALL;
    g_xPaneSplit = PixelsPerInch * 12 / 4;
    g_xHalfSplitWidth = GetSystemMetrics(SM_CXSIZEFRAME) / 2;

    g_dwView9Ex = DXG_Is9Ex()? 1 : 0;
    CheckMenuItem(GetMenu(hWnd), IDM_VIEW9EX, MF_BYCOMMAND | (g_dwView9Ex? MF_CHECKED : MF_UNCHECKED));

    // Make sure that the common control library read to rock
    InitCommonControls();

    CheckMenuItem(GetMenu(hWnd), g_dwViewState, MF_BYCOMMAND | MF_CHECKED);

    // Create the list view window.
    g_hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        0, 0, 0, 0, hWnd, (HMENU)IDC_LV, g_hInstance, NULL);
    ListView_SetExtendedListViewStyleEx(g_hwndLV, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    // create the tree view window.
    g_hwndTV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
        WS_VISIBLE | WS_CHILD | TVS_HASLINES |
        TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        0, 0, 0, 0, hWnd, (HMENU)IDC_TV, g_hInstance, NULL);

    // create our image list.
    DXView_InitImageList();

    // Add DXStuff stuff to the tree
    // view.
    DXGI_FillTree( g_hwndTV );
    DXG_FillTree( g_hwndTV );
    DD_FillTree( g_hwndTV );

    TreeView_SelectItem( g_hwndTV, TreeView_GetRoot( g_hwndTV ) );

    return(TRUE);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void AddCapsToTV(HTREEITEM hRoot, CAPDEFS *pcds, LPARAM lParam1)
{
    HTREEITEM hTree;
    HTREEITEM hParent[20];
    char* name;
    int   level=0;
    BOOL  bRoot = TRUE; // the first one is always a root

    hParent[0] = hRoot;

    while( name = pcds->strName )
    {
        if( name[0] == '-')
        {
            level--;
            if ( level < 0 )
                level = 0;
            name++;
        }

        if( name[0] == '+')
        {
            bRoot = TRUE;
            name++;
        }

        if( name[0] && (level >=0 && level < 20) )
        {
            hTree = TVAddNode( hParent[level], name, bRoot, IDI_CAPS,
                               pcds->fnDisplayCallback, lParam1,
                               pcds->lParam2 );

            if( bRoot )
            {
                level++;
                if ( level >= 20 )
                    level = 19;
                hParent[level] = hTree;
                bRoot = FALSE;
            }
        }

        pcds++;  // Get next Cap bit definition
    }
}




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
char c_szYes[] = "Yes";
char c_szNo[] = "No";
char c_szCurrentMode[] = "Current Mode";




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
// AddMoreCapsToLV is like AddCapsToLV, except it doesn't add the
// column headers like AddCapsToLV does.
void AddMoreCapsToLV(CAPDEF *pcd, LPVOID pv)
{
    DWORD dwValue;
    FLOAT fValue;
    TCHAR szBuff[64];

    while(pcd->strName && *pcd->strName)
    {
        if (!g_dwView9Ex && (pcd->dwCapsFlags & DXV_9EXCAP))
        {
            pcd++;  // Get next Cap bit definition
            continue;
        }

        dwValue = *(DWORD *)(((BYTE *)pv) + pcd->dwOffset);

        switch (pcd->dwFlag)
        {
            case 0:
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                Int2Str(szBuff,64,dwValue);
                LVAddText(g_hwndLV, 1, "%s", szBuff);
                break;
            case 0xFFFFFFFF:	// Hex
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                LVAddText(g_hwndLV, 1, "0x%08X", dwValue);
                break;
            case 0xEFFFFFFF:	// Shader Version
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                LVAddText(g_hwndLV, 1, "%d.%0d", D3DSHADER_VERSION_MAJOR(dwValue), D3DSHADER_VERSION_MINOR(dwValue));
                break;
            case 0xBFFFFFFF:	// FLOAT Support for new DX6 "D3DVALUE" values in D3DDeviceDesc
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                fValue = *(FLOAT*)(((BYTE *)pv) + pcd->dwOffset);
                LVAddText(g_hwndLV, 1, "%G", fValue);
                break;
            case 0x7FFFFFFF:	// HEX Support for new DX6 "WORD" values in D3DDeviceDesc
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                dwValue = *(WORD *)(((BYTE *)pv) + pcd->dwOffset);
                LVAddText(g_hwndLV, 1, "0x%04X", dwValue);
                break;
            case 0x3FFFFFFF:	// VAL Support for new DX6 "WORD" values in D3DDeviceDesc
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                dwValue = *(WORD *)(((BYTE *)pv) + pcd->dwOffset);
                Int2Str(szBuff,64,dwValue);
                LVAddText(g_hwndLV, 1, "%s", szBuff);
                break;
            case 0x1FFFFFFF:	// "-1 == unlimited"
                LVAddText(g_hwndLV, 0, "%s", pcd->strName);
                if (dwValue == 0xFFFFFFFF)
                {
                    LVAddText(g_hwndLV, 1, "Unlimited");
                }
                else
                {
                    Int2Str(szBuff,64,dwValue);
                    LVAddText(g_hwndLV, 1, "%s", szBuff);
                }
                break;
            case 0x0fffffff:    // Mask with 0xffff
                {
                    LVAddText(g_hwndLV, 0, pcd->strName);
                    dwValue = (*(DWORD *)(((BYTE *)pv) + pcd->dwOffset)) & 0xffff;
                    Int2Str(szBuff,64,dwValue);
                    LVAddText(g_hwndLV, 1, "%s", szBuff);
                }
                break;
            default:
                if (pcd->dwFlag & dwValue)
                {
                    LVAddText(g_hwndLV, 0, pcd->strName);
                    LVAddText(g_hwndLV, 1, c_szYes);
                }
                else if (g_dwViewState == IDM_VIEWALL)
                {
                    LVAddText(g_hwndLV, 0, pcd->strName);
                    LVAddText(g_hwndLV, 1, c_szNo);
                }
                break;
        }
        pcd++;  // Get next Cap bit definition
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
// AddColsToLV adds the column headers but no data.
void AddColsToLV(void)
{
    LVAddColumn(g_hwndLV, 0, "Name", 30);
    LVAddColumn(g_hwndLV, 1, "Value", 10);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID AddCapsToLV(CAPDEF *pcd, LPVOID pv)
{
    AddColsToLV();
    AddMoreCapsToLV(pcd, pv);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HRESULT PrintCapsToDC( CAPDEF *pcd, LPVOID pv, PRINTCBINFO * lpInfo )
{
    DWORD dwValue;
    DWORD cchLen;
    int   xName, xVal, yLine;
    TCHAR szValue[100];
    FLOAT fValue;

    // Check Parameters
    if ((! pcd) || (!lpInfo))
        return E_FAIL;

    // Calculate Name and Value column x offsets
    xName   = (lpInfo->dwCurrIndent * DEF_TAB_SIZE * lpInfo->dwCharWidth);
    xVal    = xName + (50 * lpInfo->dwCharWidth);

    while (pcd->strName && *pcd->strName)
    {
        if (!g_dwView9Ex && (pcd->dwCapsFlags & DXV_9EXCAP))
        {
            pcd++;  // Get next Cap bit definition
            continue;
        }

        dwValue = *(DWORD *)(((BYTE *)pv) + pcd->dwOffset);
        yLine = lpInfo->dwCurrLine * lpInfo->dwLineHeight;

        if (pcd->dwFlag)
        {
            switch (pcd->dwFlag)
            {
                case 0xFFFFFFFF:	// Hex
                    // Print name
                    cchLen = _tcslen (pcd->strName);
                    if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                        return E_FAIL;
                    // Print value in hex
                    sprintf_s(szValue, sizeof(szValue), "0x%08X", dwValue);
                    if( FAILED( PrintLine (xVal, yLine, szValue, strlen(szValue), lpInfo) ) )
                        return E_FAIL;
                    // Advance to next line on page
                    if( FAILED( PrintNextLine(lpInfo) ) )
                        return E_FAIL;
                    break;

                case 0xEFFFFFFF:	// Shader Version
                    // Print name
                    cchLen = _tcslen (pcd->strName);
                    if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                        return E_FAIL;
                    // Print version
                    sprintf_s(szValue, sizeof(szValue), "%d.%0d", D3DSHADER_VERSION_MAJOR(dwValue), D3DSHADER_VERSION_MINOR(dwValue));
                    if( FAILED( PrintLine (xVal, yLine, szValue, strlen(szValue), lpInfo) ) )
                        return E_FAIL;
                    // Advance to next line on page
                    if( FAILED( PrintNextLine(lpInfo) ) )
                        return E_FAIL;
                    break;

                case 0xBFFFFFFF:	// FLOAT Support for new DX6 "D3DVALUE" values in D3DDeviceDesc
                    // Print name
                    cchLen = _tcslen (pcd->strName);
                    if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                        return E_FAIL;
                    // Print value
                    fValue = *(FLOAT*)(((BYTE *)pv) + pcd->dwOffset);
                    sprintf_s(szValue, sizeof(szValue), "%G", fValue);
                    if( FAILED( PrintLine (xVal, yLine, szValue, strlen(szValue), lpInfo) ) )
                        return E_FAIL;
                    // Advance to next line on page
                    if( FAILED( PrintNextLine(lpInfo) ) )
                        return E_FAIL;
                    break;

                case 0x7FFFFFFF:	// HEX Support for new DX6 "WORD" values in D3DDeviceDesc
                    // Print name
                    cchLen = _tcslen (pcd->strName);
                    if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                        return E_FAIL;
                    // Print value
                    dwValue = *(WORD *)(((BYTE *)pv) + pcd->dwOffset);
                    sprintf_s(szValue, sizeof(szValue), "0x%04X", dwValue);
                    if( FAILED( PrintLine (xVal, yLine, szValue, strlen(szValue), lpInfo) ) )
                        return E_FAIL;
                    // Advance to next line on page
                    if( FAILED( PrintNextLine(lpInfo) ) )
                        return E_FAIL;
                    break;

                case 0x3FFFFFFF:	// VAL Support for new DX6 "WORD" values in D3DDeviceDesc
                    // Print name
                    cchLen = _tcslen (pcd->strName);
                    if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                        return E_FAIL;
                    // Print value
                    dwValue = *(WORD *)(((BYTE *)pv) + pcd->dwOffset);
                    *szValue = 0;
                    Int2Str(szValue,100,dwValue);
                    if( FAILED( PrintLine (xVal, yLine, szValue, strlen(szValue), lpInfo) ) )
                        return E_FAIL;
                    // Advance to next line on page
                    if( FAILED( PrintNextLine(lpInfo) ) )
                        return E_FAIL;
                    break;
                
                case 0x1FFFFFFF:	// "-1 == unlimited"
                    // Print name
                    cchLen = _tcslen (pcd->strName);
                    if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                        return E_FAIL;
                    // Print value
                    *szValue = 0;
                    if (dwValue == 0xFFFFFFFF)
                        strcpy_s(szValue, sizeof(szValue), "Unlimited");
                    else
                        Int2Str(szValue, 100, dwValue);
                    if( FAILED( PrintLine (xVal, yLine, szValue, strlen(szValue), lpInfo) ) )
                        return E_FAIL;
                    // Advance to next line on page
                    if( FAILED( PrintNextLine(lpInfo) ) )
                        return E_FAIL;
                    break;

                default:
                    if (pcd->dwFlag & dwValue)
                    {
                        // Print Name
                        cchLen = _tcslen (pcd->strName);
                        if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                            return E_FAIL;

                        // Print Yes in value column
                        cchLen = _tcslen (c_szYes);
                        if( FAILED( PrintLine (xVal, yLine, c_szYes, cchLen, lpInfo) ) )
                            return E_FAIL;
        
                        // Advance to next line on page
                        if( FAILED( PrintNextLine(lpInfo) ) )
                            return E_FAIL;
                    }
                    else if (g_dwViewState == IDM_VIEWALL)
                    {
                        // Print name
                        cchLen = _tcslen (pcd->strName);
                        if( FAILED( PrintLine (xName, yLine, pcd->strName, cchLen, lpInfo) ) )
                            return E_FAIL;

                        // Print No in value column
                        cchLen = _tcslen (c_szNo);
                        if( FAILED( PrintLine (xVal, yLine, c_szNo, cchLen, lpInfo) ) )
                            return E_FAIL;

                        // Advance to next line on page
                        if( FAILED( PrintNextLine(lpInfo) ) )
                            return E_FAIL;
                    }
                break;
            }
        }
        else
        {
            char    szBuff[80];

            // Print name
            sprintf_s (szBuff, sizeof(szBuff), pcd->strName, "test");
            cchLen = _tcslen (pcd->strName);
            if( FAILED( PrintLine (xName, yLine, szBuff, cchLen, lpInfo) ) )
                return E_FAIL;

            // Print value
            Int2Str(szBuff, 80, dwValue);
            cchLen = _tcslen (szBuff);
            if( FAILED( PrintLine (xVal, yLine, szBuff, cchLen, lpInfo) ) )
                return E_FAIL;

            // Advance to next line on page
            if( FAILED( PrintNextLine(lpInfo) ) )
                return E_FAIL;
        }

        pcd++;  // Get next Cap bit definition
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void DXView_OnTreeSelect(HWND hwndTV, NM_TREEVIEW *ptv)
{
    NODEINFO *pni;

    SendMessage(g_hwndLV, WM_SETREDRAW, FALSE, 0);
    LVDeleteAllItems(g_hwndLV);
    LVAddColumn(g_hwndLV, 0, "", 0);

    if (ptv == NULL)
    {
        TV_ITEM tvi;
        // get lParam of current tree node
        tvi.hItem  = TreeView_GetSelection(g_hwndTV);
        tvi.mask   = TVIF_PARAM;
        tvi.lParam = 0;
        TreeView_GetItem(g_hwndTV, &tvi);
        pni = (NODEINFO*)tvi.lParam;
    }
    else
    {
        pni = (NODEINFO*)ptv->itemNew.lParam;
    }

    if (pni && pni->fnDisplayCallback)
    {
        if( pni->bUseLParam3 )
            ((DISPLAYCALLBACKEX)(pni->fnDisplayCallback))(pni->lParam1, pni->lParam2, pni->lParam3, NULL );
        else
            pni->fnDisplayCallback(pni->lParam1, pni->lParam2, NULL );
    }

    ListView_SetItemState( g_hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

    SendMessage(g_hwndLV, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hwndLV, NULL, TRUE);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void DXView_OnListViewDblClick(HWND hwndLV, NM_LISTVIEW *plv)
{
    LV_ITEM lvi;

    lvi.mask   = LVIF_PARAM;
    lvi.lParam = 0;
    lvi.iItem  = plv->iItem;
    ListView_GetItem(hwndLV, &lvi);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void DXView_OnCommand(HWND hWnd, WPARAM wParam)
{
    HMENU hMenu;

    switch(LOWORD(wParam))
    {
        case IDM_VIEWAVAIL:
        case IDM_VIEWALL:
            hMenu = GetMenu(hWnd);
            CheckMenuItem(hMenu, g_dwViewState, MF_BYCOMMAND | MF_UNCHECKED);
            g_dwViewState = LOWORD(wParam);
            CheckMenuItem(hMenu, g_dwViewState, MF_BYCOMMAND | MF_CHECKED);
            DXView_OnTreeSelect(g_hwndTV, NULL);
            break;

        case IDM_VIEW9EX:
            hMenu = GetMenu(hWnd);
            g_dwView9Ex = !g_dwView9Ex;
            CheckMenuItem(hMenu, IDM_VIEW9EX, MF_BYCOMMAND | (g_dwView9Ex? MF_CHECKED : MF_UNCHECKED));
            DXView_OnTreeSelect(g_hwndTV, NULL);
            break;

        case IDM_PRINTWHOLETREETOPRINTER:
            g_PrintToFile = FALSE;
            DXView_OnPrint( hWnd, g_hwndTV, TRUE );
            break;

        case IDM_PRINTSUBTREETOPRINTER:
            g_PrintToFile = FALSE;
            DXView_OnPrint( hWnd, g_hwndTV, FALSE );
            break;

        case IDM_PRINTWHOLETREETOFILE:
            g_PrintToFile = TRUE;
            DXView_OnFile( hWnd, g_hwndTV, TRUE );
            break;

        case IDM_PRINTSUBTREETOFILE:
            g_PrintToFile = TRUE;
            DXView_OnFile( hWnd, g_hwndTV, FALSE );
            break;

        case IDM_COPY:
            {
                // Put g_szClip into the clipboard
                HGLOBAL hGlobal;
                VOID* pGlobal;
                UINT GlobalSize= strlen(g_szClip) + 1;
                hGlobal = GlobalAlloc(GHND | GMEM_SHARE, GlobalSize );
                if( hGlobal != NULL )
                {
                    pGlobal = GlobalLock(hGlobal);
                    if( pGlobal != NULL )
                    {
                        strcpy_s((CHAR*)pGlobal, GlobalSize, g_szClip);
                        GlobalUnlock(hGlobal);
                        OpenClipboard(g_hwndMain);
                        EmptyClipboard();
                        SetClipboardData(CF_TEXT, hGlobal);
                        CloseClipboard();
                    }
                }
            }
            break;

        case IDM_ABOUT:
            DialogBox(g_hInstance, "About", hWnd, (DLGPROC)About);
            break;

        case IDM_HELP:
            InvokeHelp();
            break;

        case IDM_EXIT:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void DXView_Cleanup()
{
    DXGI_CleanUp();

    DXG_CleanUp();

    DD_CleanUp();

    if( g_hImageList )
        ImageList_Destroy( g_hImageList );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL DXView_InitImageList()
{
    int cxSmIcon;
    int cySmIcon;
    UINT Index;
    HICON hIcon;

    if (g_hImageList)
        return TRUE;

    cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    cySmIcon = GetSystemMetrics(SM_CYSMICON);

    //  First, create the image list that is needed.
    if((g_hImageList = ImageList_Create(cxSmIcon, cySmIcon, TRUE, IDI_LASTIMAGE - IDI_FIRSTIMAGE, 10)) == NULL)
        return(FALSE);

    //
    //  Initialize the image list with all of the icons that we'll be using
    //  Once set, send its handle to all interested child windows.
    //
    for (Index = IDI_FIRSTIMAGE; Index <= IDI_LASTIMAGE; Index++)
    {
        hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(Index), IMAGE_ICON, cxSmIcon, cySmIcon, 0);
        if( hIcon != NULL )
        {
            ImageList_AddIcon(g_hImageList, hIcon);
            DestroyIcon(hIcon);
        }
    }

    TreeView_SetImageList( g_hwndTV, g_hImageList, TVSIL_NORMAL );

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: About()
// Desc: Process about box
//-----------------------------------------------------------------------------
LRESULT CALLBACK About( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch( msg )
    {
        case WM_INITDIALOG:
            TCHAR str[500];
            TCHAR strFmt[500];
            TCHAR szFile[MAX_PATH];
            TCHAR strVersion[100];
            UINT cb;
            DWORD dwHandle;
            BYTE FileVersionBuffer[1024];
            VS_FIXEDFILEINFO* pVersion;

            pVersion = NULL;
            GetWindowText( GetDlgItem( hDlg, IDC_VERSION ), strFmt, 500 );
            *szFile = 0;
            GetModuleFileName(NULL, szFile, MAX_PATH);

            cb = GetFileVersionInfoSize(szFile, &dwHandle/*ignored*/);
            if (cb > 0)
            {
                if (cb > sizeof(FileVersionBuffer))
                    cb = sizeof(FileVersionBuffer);

                if (GetFileVersionInfo(szFile, 0, cb, FileVersionBuffer))
                {
                    pVersion = NULL;
                    if (VerQueryValue(FileVersionBuffer, "\\", (VOID**)&pVersion, &cb)
                        && pVersion != NULL) 
                    {
                        sprintf_s(strVersion, sizeof(strVersion), "%d.%02d.%02d.%04d", 
                            HIWORD(pVersion->dwFileVersionMS),
                            LOWORD(pVersion->dwFileVersionMS), 
                            HIWORD(pVersion->dwFileVersionLS), 
                            LOWORD(pVersion->dwFileVersionLS) );

                        sprintf_s(str, sizeof(str), strFmt, strVersion );
                        SetWindowText( GetDlgItem( hDlg, IDC_VERSION ), str );
                    }
                }
            }

            // Note: The warning text is static, but it must be set via code, 
            // not in the .rc file, because it is > 256 characters (warning RC4206).
            {
                const TCHAR* pstrWarning = TEXT("Warning:  This computer program is protected by copyright law and international treaties.  Unauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under the law.");
                SetWindowText( GetDlgItem( hDlg, IDC_WARNING), pstrWarning );
            }
            return TRUE;

        case WM_COMMAND:
            if( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL )
            {
                EndDialog( hDlg, TRUE ); // Exit the dialog
                return TRUE;
            }
            break;
    }
    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: DXView_OnSize()
// Desc: Called whenever the size of the app window has changed or the size
//       of its child controls should be adjusted.
//-----------------------------------------------------------------------------
VOID DXView_OnSize( HWND hWnd )
{
    HDWP hDWP;
    RECT ClientRect;
    int Height;
    HWND hKeyTreeWnd;
    HWND hValueListWnd;
    int x;
    int dx;

    if( IsIconic(hWnd) )
        return;

    if( (hDWP = BeginDeferWindowPos(2)) != NULL )
    {
        //  Data structure used when calling GetEffectiveClientRect (which takes into
        //  account space taken up by the toolbars/status bars).  First half of the
        //  pair is zero when at the end of the list, second half is the control id.
        int s_EffectiveClientRectData[] =
        {
            1, 0,                               //  For the menu bar, but is unused
            0, 0                                //  First zero marks end of data
        };

        GetEffectiveClientRect(hWnd, &ClientRect, s_EffectiveClientRectData);
        Height = ClientRect.bottom - ClientRect.top;

        hKeyTreeWnd = g_hwndTV;

        DeferWindowPos(hDWP, hKeyTreeWnd, NULL, 0, ClientRect.top, g_xPaneSplit,
            Height, SWP_NOZORDER | SWP_NOACTIVATE);

        x = g_xPaneSplit + GetSystemMetrics(SM_CXSIZEFRAME);
        dx = ClientRect.right - ClientRect.left - x;

        hValueListWnd = g_hwndLV;

        DeferWindowPos(hDWP, hValueListWnd, NULL, x, ClientRect.top, dx, Height,
            SWP_NOZORDER | SWP_NOACTIVATE);

        EndDeferWindowPos(hDWP);
    }
}



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void LVAddColumn(HWND hwndLV, int i, const char *name, int width)
{
    LV_COLUMN col;

    if (i == 0)
    {
        while(ListView_DeleteColumn(hwndLV, 0))
            ;
    }

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.fmt  = LVCFMT_LEFT;
    col.pszText = (char*)name;
    col.cchTextMax = 0;
    col.cx = width * g_tmAveCharWidth;
    col.iSubItem = 0;
    ListView_InsertColumn(hwndLV, i, &col);
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
int LVAddText(HWND hwndLV, int col, const char *sz, ...)
{
    LV_ITEM lvi;
    char    ach[80];
    va_list vl;

    va_start(vl, sz );
    
    vsprintf_s(ach, sizeof(ach), sz, vl);
    ach[79] = '\0';

    lvi.mask        = LVIF_TEXT;
    lvi.iSubItem    = 0;
    lvi.state       = 0;
    lvi.stateMask   = 0;
    lvi.pszText     = ach;
    lvi.cchTextMax  = 0;
    lvi.iImage      = 0;
    lvi.lParam      = 0;

    if (col == 0)
    {
        lvi.iItem    = 0x7FFF;
        lvi.iSubItem = 0;
        va_end(vl);
        return ListView_InsertItem(hwndLV, &lvi);
    }
    else
    {
        lvi.iItem    = ListView_GetItemCount(hwndLV)-1;
        lvi.iSubItem = col;
        va_end(vl);
        return ListView_SetItem(hwndLV, &lvi);
    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
void LVDeleteAllItems( HWND hwndLV )
{
    ListView_DeleteAllItems( hwndLV );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HTREEITEM TVAddNode( HTREEITEM hParent, LPCSTR strText, BOOL fKids, 
                     int iImage, DISPLAYCALLBACK fnDisplayCallback, LPARAM lParam1, 
                     LPARAM lParam2 )
{
    TV_INSERTSTRUCT tvi;
    NODEINFO* pni;

    pni = (NODEINFO*)LocalAlloc( LPTR, sizeof(NODEINFO) );

    if( pni == NULL )
        return NULL;

    pni->bUseLParam3       = FALSE;
    pni->lParam1           = lParam1;
    pni->lParam2           = lParam2;
    pni->lParam3           = 0;
    pni->fnDisplayCallback = fnDisplayCallback;

    // Add Node to treeview
    tvi.hParent             = hParent;
    tvi.hInsertAfter        = TVI_LAST;
    tvi.item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
                              TVIF_PARAM | TVIF_CHILDREN;
    tvi.item.iImage         = iImage - IDI_FIRSTIMAGE;
    tvi.item.iSelectedImage = iImage - IDI_FIRSTIMAGE;
    tvi.item.lParam         = (LPARAM)pni;
    tvi.item.cChildren      = fKids;
    tvi.item.pszText        = (LPSTR)strText;

    return TreeView_InsertItem( g_hwndTV, &tvi );
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
HTREEITEM TVAddNodeEx( HTREEITEM hParent, LPCSTR strText, BOOL fKids, 
                     int iImage, DISPLAYCALLBACKEX fnDisplayCallback, LPARAM lParam1, 
                     LPARAM lParam2, LPARAM lParam3 )
{
    TV_INSERTSTRUCT tvi;
    NODEINFO* pni;

    pni = (NODEINFO*)LocalAlloc( LPTR, sizeof(NODEINFO) );

    if( pni == NULL )
        return NULL;

    pni->bUseLParam3       = TRUE;
    pni->lParam1           = lParam1;
    pni->lParam2           = lParam2;
    pni->lParam3           = lParam3;
    pni->fnDisplayCallback = (DISPLAYCALLBACK)fnDisplayCallback;

    // Add Node to treeview
    tvi.hParent             = hParent;
    tvi.hInsertAfter        = TVI_LAST;
    tvi.item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
                              TVIF_PARAM | TVIF_CHILDREN;
    tvi.item.iImage         = iImage - IDI_FIRSTIMAGE;
    tvi.item.iSelectedImage = iImage - IDI_FIRSTIMAGE;
    tvi.item.lParam         = (LPARAM)pni;
    tvi.item.cChildren      = fKids;
    tvi.item.pszText        = (LPSTR)strText;

    return TreeView_InsertItem( g_hwndTV, &tvi );
}


//-----------------------------------------------------------------------------
VOID FindHelp()
{
    *g_helpPath = 0;

    TCHAR strPath[MAX_PATH];
    *strPath = 0;
    GetModuleFileName( NULL, strPath, MAX_PATH );
    PathRemoveFileSpec( strPath );
    PathAppend( strPath, TEXT("..") ); // up to bin
    PathAppend( strPath, TEXT("..") ); // up to utilities
    PathAppend( strPath, TEXT("..") ); // up to SDK
    PathAppend( strPath, TEXT("Documentation") );
    PathAppend( strPath, TEXT("DirectX9") );

    TCHAR strHelp[MAX_PATH];
    sprintf_s( strHelp, MAX_PATH, "%s\\directx_sdk.chm", strPath );

    if( !PathFileExists( strHelp ) )
    {
        // Try the current directory instead 
        GetModuleFileName( NULL, strPath, MAX_PATH );
        PathRemoveFileSpec( strPath );
        sprintf_s( strHelp, MAX_PATH, "%s\\directx_sdk.chm", strPath );

        if( !PathFileExists( strHelp ) )
        {
            // Look for the environment variable
            ::GetEnvironmentVariable( TEXT("DXSDK_DIR"), strPath, MAX_PATH);
            sprintf_s( strHelp, MAX_PATH, "%sDocumentation\\DirectX9\\directx_sdk.chm", strPath );
            if( !PathFileExists( strHelp ) )
            {
                return;
            }
        }
    }

    strcpy_s( g_helpPath, MAX_PATH, strHelp );
}


//-----------------------------------------------------------------------------
VOID InvokeHelp()
{
    // Check if help is available
    if( *g_helpPath == 0 )
    {
        MessageBox( g_hwndMain, TEXT("Help file not found."), g_strAppName, MB_OK );
        return;
    }

    TCHAR strHelp[ MAX_PATH ];
    strcpy_s( strHelp, MAX_PATH, g_helpPath );

    strcat_s( strHelp, MAX_PATH, "::/DirectX_Caps_Viewer_Tool.htm" );

#if _MFC_VER >= 0x0710
    HtmlHelp( (DWORD_PTR)strHelp, HH_DISPLAY_TOPIC );
#else
    ::HtmlHelp( NULL, strHelp, HH_DISPLAY_TOPIC, NULL );
#endif
}

#ifdef _X86_
#pragma optimize("", on)
#endif




//-----------------------------------------------------------------------------
// Name: DisplayErrorMessage()
// Desc: Displays an error message in a message box
//-----------------------------------------------------------------------------
VOID DisplayErrorMessage( const CHAR* strError )
{
    if( !g_PrintToFile )
        MessageBox( g_hwndMain, strError, g_strAppName, MB_OK );
}



