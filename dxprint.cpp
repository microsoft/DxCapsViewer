//-----------------------------------------------------------------------------
// Name: dxprint.cpp
//
// Desc: DirectX Capabilities Viewer Printing Support
//
// Copyright (c) Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#include "dxview.h"

#include <Windowsx.h>
#include <commdlg.h>
#include <shlobj.h>

BOOL   g_PrintToFile = FALSE; // Don't print to printer print to dxview.log
TCHAR  g_PrintToFilePath[MAX_PATH]; // "Print" to this file instead of dxview.log

namespace
{
    BOOL   g_fAbortPrint = FALSE; // Did User Abort Print operation ?!?
    HWND   g_hAbortPrintDlg = nullptr;  // Print Abort Dialog handle
    HANDLE g_FileHandle = nullptr;  // Handle to log file

    DWORD iLastXPos = 0;

    //-----------------------------------------------------------------------------
    // Local Prototypes
    //-----------------------------------------------------------------------------
#define MAX_TITLE   64
#define MAX_MESSAGE 256
    VOID DoMessage(DWORD dwTitle, DWORD dwMsg);

    BOOL CALLBACK PrintTreeStats(HINSTANCE hInstance, HWND hWnd, HWND hTreeWnd,
        HTREEITEM hRoot);


    //-----------------------------------------------------------------------------
    // Name: PrintDialogProc()
    // Desc: Dialog procedure for printing
    //-----------------------------------------------------------------------------
    BOOL CALLBACK PrintDialogProc(HWND hDlg, UINT msg, WPARAM /*wParam*/, LPARAM /*lParam*/)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            // Disable system menu on dialog
            EnableMenuItem(GetSystemMenu(hDlg, FALSE), SC_CLOSE, MF_GRAYED);
            return TRUE;

        case WM_COMMAND:
            // User is aborting print operation
            g_fAbortPrint = TRUE;
            EnableWindow(GetParent(hDlg), TRUE);
            DestroyWindow(hDlg);
            g_hAbortPrintDlg = nullptr;
            return TRUE;
        }

        return FALSE;
    }


    //-----------------------------------------------------------------------------
    // Name: AbortProc()
    // Desc: Abort procedure for printing
    //-----------------------------------------------------------------------------
    BOOL CALLBACK AbortProc(HDC /*hPrinterDC*/, int /*iCode*/)
    {
        MSG msg;

        while (!g_fAbortPrint && PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if ((!g_hAbortPrintDlg) || !IsDialogMessage(g_hAbortPrintDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        return !g_fAbortPrint;
    }


    //-----------------------------------------------------------------------------
    // Name: PrintStartPage
    // Desc: Check if we need to start a new page
    //-----------------------------------------------------------------------------
    HRESULT PrintStartPage(_In_ PRINTCBINFO* pci)
    {
        if (g_PrintToFile)
            return S_OK;

        if (!pci)
            return E_FAIL;

        // Check if we need to start a new page
        if (pci->fStartPage)
        {
            // Check for user abort
            if (g_fAbortPrint)
                return E_FAIL;

            // Start new page
            if (StartPage(pci->hdcPrint) < 0)
                return E_FAIL;

            // Reset line count
            pci->fStartPage = FALSE;
            pci->dwCurrLine = 0;
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // Name: PrintEndPage()
    // Desc: Check if we need to start a new page
    //-----------------------------------------------------------------------------
    HRESULT PrintEndPage(_In_ PRINTCBINFO* pci)
    {
        if (g_PrintToFile)
            return S_OK;

        if (!pci)
            return E_FAIL;

        // Check if we need to end this page
        if (!pci->fStartPage)
        {
            // End page
            if (EndPage(pci->hdcPrint) < 0)
                return E_FAIL;

            pci->fStartPage = TRUE;

            // Check for user abort
            if (g_fAbortPrint)
                return E_FAIL;
        }

        return S_OK;
    }


    //-----------------------------------------------------------------------------
    // Name: DoMessage()
    // Desc: Display warning message to user
    //-----------------------------------------------------------------------------
    VOID DoMessage(DWORD dwTitle, DWORD dwMsg)
    {
        TCHAR strTitle[MAX_TITLE];
        TCHAR strMsg[MAX_MESSAGE];

        LoadString(GetModuleHandle(nullptr), dwTitle, strTitle, MAX_TITLE);
        LoadString(GetModuleHandle(nullptr), dwMsg, strMsg, MAX_MESSAGE);
        MessageBox(nullptr, strMsg, strTitle, MB_OK);
    }


    //-----------------------------------------------------------------------------
    // Name: PrintStats()
    // Desc: Print user defined stuff
    //-----------------------------------------------------------------------------
    BOOL CALLBACK PrintTreeStats(HINSTANCE hInstance, HWND hWnd, HWND hTreeWnd,
        HTREEITEM hRoot)
    {
        static DOCINFO  di;
        static PRINTDLG pd = {};

        // Check Parameters
        if (!hInstance || !hWnd || !hTreeWnd)
            return FALSE;

        // Get Starting point for tree
        HTREEITEM   hStartTree = (hRoot) ? hRoot : TreeView_GetRoot(hTreeWnd);
        if (!hStartTree)
            return FALSE;

        BOOL        fDone = FALSE;
        BOOL        fFindNext = FALSE;
        BOOL        fResult = FALSE;
        BOOL        fStartDoc = FALSE;
        BOOL        fDisableWindow = FALSE;
        LPTSTR      pstrTitle = nullptr;
        LPTSTR      pstrBuff = nullptr;
        DWORD       dwCurrCopy;
        HANDLE      hHeap = nullptr;
        DWORD       buffSize;
        DWORD       cchLen;
        TV_ITEM     tvi;

        // Initialize Print Dialog structure
        pd.lStructSize = sizeof(PRINTDLG);
        pd.hwndOwner = hWnd;
        pd.Flags = PD_ALLPAGES | PD_RETURNDC;
        pd.nCopies = 1;

        PRINTCBINFO pci = {};
        TEXTMETRIC  tm = {};
        if (g_PrintToFile)
        {
            pci.hdcPrint = nullptr;
        }
        else
        {
            // Call Common Print Dialog to get printer DC
            if (!PrintDlg(&pd) || !pd.hDC)
            {
                // Print Dialog failed or user canceled
                return TRUE;
            }

            pci.hdcPrint = pd.hDC;
            // Get Text metrics for printing
            if (!GetTextMetrics(pci.hdcPrint, &tm))
            {
                // Error, TextMetrics failed
                goto lblCLEANUP;
            }
        }

        if (g_PrintToFile)
        {
            pci.dwLineHeight = 1;
            pci.dwCharWidth = 1;
            pci.dwCharsPerLine = 80;
            pci.dwLinesPerPage = 66;
        }
        else
        {
            pci.dwLineHeight = tm.tmHeight + tm.tmExternalLeading;
            pci.dwCharWidth = tm.tmAveCharWidth;
            pci.dwCharsPerLine = GetDeviceCaps(pci.hdcPrint, HORZRES) / pci.dwCharWidth;
            pci.dwLinesPerPage = GetDeviceCaps(pci.hdcPrint, VERTRES) / pci.dwLineHeight;
        }

        // Get Heap
        hHeap = GetProcessHeap();
        if (!hHeap)
        {
            // Error, no heap associated with this process
            goto lblCLEANUP;
        }

        // Create line buffer
        buffSize = (pci.dwCharsPerLine + 1) * sizeof(TCHAR);
        pstrBuff = (LPTSTR)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, buffSize);
        if (!pstrBuff)
        {
            // Error, not enough memory 
            goto lblCLEANUP;
        }

        *pstrBuff = 0;

        // Disable Parent window
        EnableWindow(hWnd, FALSE);
        fDisableWindow = TRUE;

        // Start Printer Abort Dialog
        g_fAbortPrint = FALSE;
        g_hAbortPrintDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABORTPRINTDLG),
            hWnd, (DLGPROC)PrintDialogProc);
        if (!g_hAbortPrintDlg)
        {
            // Error, unable to create abort dialog
            goto lblCLEANUP;
        }

        //
        // Set Document title to Root string
        //
        tvi.mask = TVIF_CHILDREN | TVIF_TEXT;
        tvi.hItem = hStartTree;
        tvi.pszText = pstrBuff;
        tvi.cchTextMax = pci.dwCharsPerLine;
        if (TreeView_GetItem(hTreeWnd, &tvi))
        {
            SetWindowText(g_hAbortPrintDlg, pstrBuff);
            SetAbortProc(pd.hDC, AbortProc);
            cchLen = static_cast<DWORD>(_tcsclen(pstrBuff));
            DWORD cbSize = (cchLen + 1) * sizeof(TCHAR);
            pstrTitle = (LPTSTR)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, cbSize);
            if (!pstrTitle)
            {
                // Error, not enough memory 
                goto lblCLEANUP;
            }

            strcpy_s(pstrTitle, cbSize, pstrBuff);
            pstrTitle[cchLen] = 0;
        }
        else
        {
            SetWindowText(g_hAbortPrintDlg, TEXT("Unknown"));
            SetAbortProc(pd.hDC, AbortProc);
            cchLen = static_cast<DWORD>(_tcsclen(TEXT("Unknown")));
            DWORD cbSize = (cchLen + 1) * sizeof(TCHAR);
            pstrTitle = (LPTSTR)HeapAlloc(hHeap, HEAP_NO_SERIALIZE, cbSize);
            if (!pstrTitle)
            {
                // Error, not enough memory 
                goto lblCLEANUP;
            }

            strcpy_s(pstrTitle, cbSize, TEXT("Unknown"));
            pstrTitle[cchLen] = 0;
        }

        // Initialize Document Structure
        di.cbSize = sizeof(DOCINFO);
        di.lpszDocName = pstrTitle;
        di.lpszOutput = nullptr;
        di.lpszDatatype = nullptr;
        di.fwType = 0;

        // Start document
        if (g_PrintToFile)
        {
            const TCHAR* pstrFile;
            TCHAR buff[MAX_PATH];
            if (strlen(g_PrintToFilePath) > 0)
                pstrFile = g_PrintToFilePath;
            else
            {
                HRESULT hr = SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, buff);
                if (SUCCEEDED(hr))
                {
                    strcat_s(buff, MAX_PATH, TEXT("\\dxview.log"));
                    pstrFile = buff;
                }
                else
                    pstrFile = TEXT("dxview.log");
            }
            g_FileHandle = CreateFile(pstrFile, GENERIC_WRITE, 0, nullptr,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }
        else
            if (StartDoc(pd.hDC, &di) < 0)
            {
                // Error, StartDoc failed
                goto lblCLEANUP;
            }
        fStartDoc = TRUE;

        // Print requested number of copies
        fResult = FALSE;
        for (dwCurrCopy = 0; dwCurrCopy < (DWORD)pd.nCopies; dwCurrCopy++)
        {

            pci.hCurrTree = hStartTree;
            pci.fStartPage = TRUE;
            pci.dwCurrIndent = 0;


            // Note:  We are basically doing an pre-order traversal
            //        of the Tree.  Printing the current node
            //        before moving on to it's children or siblings

            fDone = FALSE;
            while (!fDone)
            {
                // Check if we need to start a new page
                if (FAILED(PrintStartPage(&pci)))
                {
                    goto lblCLEANUP;
                }

                //
                // Get Current Item in Tree 
                // and print it's text info and associated Node caps
                //

                tvi.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_PARAM;
                tvi.hItem = pci.hCurrTree;
                tvi.pszText = pstrBuff;
                tvi.lParam = 0;
                tvi.cchTextMax = pci.dwCharsPerLine;
                if (TreeView_GetItem(hTreeWnd, &tvi))
                {
                    cchLen = static_cast<DWORD>(_tcslen(pstrBuff));
                    cchLen = __min(cchLen, buffSize);
                    if (cchLen > 0)
                    {
                        int xOffset = (int)(pci.dwCurrIndent * DEF_TAB_SIZE * pci.dwCharWidth);
                        int yOffset = (int)(pci.dwLineHeight * pci.dwCurrLine);

                        // Print this line
                        if (FAILED(PrintLine(xOffset, yOffset, pstrBuff, cchLen, &pci)))
                        {
                            goto lblCLEANUP;
                        }

                        // Advance to next line in page
                        if (FAILED(PrintNextLine(&pci)))
                        {
                            goto lblCLEANUP;
                        }

                        // Check if there is any additional node info 
                        // that needs to be printed
                        if (tvi.lParam != 0)
                        {
                            NODEINFO* pni = (NODEINFO*)tvi.lParam;

                            if (pni && pni->fnDisplayCallback)
                            {
                                // Force indent to offset node info from tree info
                                pci.dwCurrIndent += 2;

                                if (pni->bUseLParam3)
                                {
                                    if (FAILED(((DISPLAYCALLBACKEX)(pni->fnDisplayCallback))(pni->lParam1, pni->lParam2, pni->lParam3, &pci)))
                                    {
                                        // Error, callback failed
                                        goto lblCLEANUP;
                                    }
                                }
                                else
                                {
                                    if (FAILED(pni->fnDisplayCallback(pni->lParam1, pni->lParam2, &pci)))
                                    {
                                        // Error, callback failed
                                        goto lblCLEANUP;
                                    }
                                }

                                // Recover indent
                                pci.dwCurrIndent -= 2;
                            }
                        }
                    }
                } // End if TreeView_GetItem()



                // 
                // Get Next Item in tree
                //

                // Get first child, if any
                if (tvi.cChildren)
                {
                    // Get First child
                    HTREEITEM hTempTree = TreeView_GetChild(hTreeWnd, pci.hCurrTree);
                    if (hTempTree)
                    {
                        // Increase Indentation
                        pci.dwCurrIndent++;

                        pci.hCurrTree = hTempTree;
                        continue;
                    }
                }

                // Exit, if we are the root
                if (pci.hCurrTree == hRoot)
                {
                    // We have reached the root, so stop
                    PrintEndPage(&pci);
                    fDone = TRUE;
                    continue;
                }

                // Get next sibling in the chain
                HTREEITEM hTempTree = TreeView_GetNextSibling(hTreeWnd, pci.hCurrTree);
                if (hTempTree)
                {
                    pci.hCurrTree = hTempTree;
                    continue;
                }

                // Get next Ancestor yet to be processed
                // (uncle, granduncle, etc)
                fFindNext = FALSE;
                while (!fFindNext)
                {
                    hTempTree = TreeView_GetParent(hTreeWnd, pci.hCurrTree);
                    if ((!hTempTree) || (hTempTree == hRoot))
                    {
                        // We have reached the root, so stop
                        PrintEndPage(&pci);
                        fDone = TRUE;
                        fFindNext = TRUE;
                    }
                    else
                    {
                        // Move up to the parent
                        pci.hCurrTree = hTempTree;

                        // Decrease Indentation
                        pci.dwCurrIndent--;

                        // Since we have already processed the parent 
                        // we want to get the uncle/aunt node
                        hTempTree = TreeView_GetNextSibling(hTreeWnd, pci.hCurrTree);
                        if (hTempTree)
                        {
                            // Found a non-processed node
                            pci.hCurrTree = hTempTree;
                            fFindNext = TRUE;
                        }
                    }
                }
            } // End While (! fDone)
        } // End for num copies

        // Success
        fResult = TRUE;

    lblCLEANUP:
        // End Document
        if (fStartDoc)
        {
            if (g_PrintToFile)
            {
                CloseHandle(g_FileHandle);
            }
            else
                EndDoc(pd.hDC);
            fStartDoc = FALSE;
        }

        // Re-enable main window
        // Note:  Do this before destroying abort dialog
        //        otherwise the main window loses focus
        if (fDisableWindow)
        {
            EnableWindow(hWnd, TRUE);
            fDisableWindow = FALSE;
        }

        // Destroy Abort Dialog
        if (g_hAbortPrintDlg)
        {
            DestroyWindow(g_hAbortPrintDlg);
            g_hAbortPrintDlg = nullptr;
        }

        // Free title memory
        if (pstrTitle)
        {
            HeapFree(hHeap, 0, (VOID*)pstrTitle);
            pstrTitle = nullptr;
            di.lpszDocName = nullptr;
        }

        // Free buffer memory
        if (pstrBuff)
        {
            HeapFree(hHeap, 0, (VOID*)pstrBuff);
            pstrBuff = nullptr;
        }

        // Cleanup printer DC
        if (pd.hDC)
        {
            DeleteDC(pd.hDC);
            pd.hDC = nullptr;
        }

        return fResult;
    }
}


//-----------------------------------------------------------------------------
// Name: DXView_OnPrint()
// Desc: Print user defined stuff
//-----------------------------------------------------------------------------
BOOL DXView_OnPrint(HWND hWnd, HWND hTreeWnd, BOOL bPrintAll)
{
    HINSTANCE hInstance;
    HTREEITEM hRoot;

    // Check Parameters
    if (!hWnd || !hTreeWnd)
        return FALSE;

    // Get hInstance
    hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    if (!hInstance)
        return FALSE;

    if (bPrintAll)
    {
        hRoot = nullptr;
    }
    else
    {
        hRoot = TreeView_GetSelection(hTreeWnd);
        if (!hRoot)
            DoMessage(IDS_PRINT_WARNING, IDS_PRINT_NEEDSELECT);
    }

    g_PrintToFile = FALSE;

    // Do actual printing
    return PrintTreeStats(hInstance, hWnd, hTreeWnd, hRoot);
}


//-----------------------------------------------------------------------------
// Name: DXView_OnFile()
//-----------------------------------------------------------------------------
BOOL DXView_OnFile(HWND hWnd, HWND hTreeWnd, BOOL bPrintAll)
{
    HINSTANCE hInstance;
    HTREEITEM hRoot;

    // Check Parameters
    if (!hWnd || !hTreeWnd)
        return FALSE;

    // Get hInstance
    hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    if (!hInstance)
        return FALSE;

    if (bPrintAll)
    {
        hRoot = nullptr;
    }
    else
    {
        hRoot = TreeView_GetSelection(hTreeWnd);
        if (!hRoot)
            DoMessage(IDS_PRINT_WARNING, IDS_PRINT_NEEDSELECT);
    }

    g_PrintToFile = TRUE;

    // Do actual printing
    return PrintTreeStats(hInstance, hWnd, hTreeWnd, hRoot);
}


//-----------------------------------------------------------------------------
// Name: PrintLine()
// Desc: Prints text to page at specified location
//-----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT PrintLine(int xOffset, int yOffset, LPCTSTR pszBuff, size_t cchBuff,
    PRINTCBINFO* pci)
{
    if (!pci)
        return E_FAIL;

    // Check if we need to start a new page
    if (FAILED(PrintStartPage(pci)))
        return E_FAIL;

    // Return if there's nothing to print
    if (!pszBuff || !cchBuff)
        return S_OK;

    // Print text out to buffer current line
    if (g_PrintToFile)
    {
        DWORD dwDummy;
        TCHAR Temp[80];

        int offset = (xOffset - iLastXPos) / pci->dwCharWidth;

        if (offset < 0 || offset >= 80)
            return S_OK;

        memset(Temp, ' ', sizeof(TCHAR) * 79);
        Temp[offset] = 0;
        WriteFile(g_FileHandle, Temp, (xOffset - iLastXPos) / pci->dwCharWidth, &dwDummy, nullptr);
        iLastXPos = (xOffset - iLastXPos) + (pci->dwCharWidth * static_cast<DWORD>(cchBuff));

        WriteFile(g_FileHandle, pszBuff, static_cast<DWORD>(cchBuff), &dwDummy, nullptr);
    }
    else
    {
        TextOut(pci->hdcPrint, xOffset, yOffset, pszBuff, static_cast<int>(cchBuff));
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: PrintNextLine()
// Desc: Advance to next line on page
//-----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT PrintNextLine(PRINTCBINFO* pci)
{
    if (g_PrintToFile)
    {
        DWORD dwDummy;

        WriteFile(g_FileHandle, "\r\n", 2, &dwDummy, nullptr);
        iLastXPos = 0;
        return S_OK;
    }

    if (!pci)
        return E_FAIL;

    pci->dwCurrLine++;

    // Check if we need to end the page
    if (pci->dwCurrLine >= pci->dwLinesPerPage)
        return PrintEndPage(pci);

    return S_OK;
}
