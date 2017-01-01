// HexViewer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "HexViewer.h"

#include <windowsx.h>
#include <commdlg.h>
#include <richedit.h>

#include <string>
using namespace std;


#define MAX_LOADSTRING 100


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL                LoadChildDialog(HINSTANCE hInstance, HWND hWindow, INT iDialogID, DLGPROC lpDialogProc);
INT_PTR CALLBACK    HexViewerDlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL                OpenHexFile(HWND hOwnerWindow);
BOOL                OpenFileDialog(HWND hOwnerWindow, wstring* pstrFileName);


typedef struct
{
    union
    {
        struct
        {
            unsigned char ucLow : 4;
            unsigned char ucHigh : 4;
        } sByte;
        char cByte;
    };
} sCombinedByte;


class CEditCtlText
{
private:
    void* m_pTextBuffer = nullptr;
    size_t m_sTextBufferSize = 0;
    void* m_pFormattedTextBuffer = nullptr;
    size_t m_sFormattedTextBufferSize = 0;
    HWND m_hEditCtlWnd = 0;


    void FreeMemoryBuffer()
    {
        if (m_pTextBuffer)
        {
            free(m_pTextBuffer);
            m_pTextBuffer = nullptr;
        }

        if (m_pFormattedTextBuffer)
        {
            free(m_pFormattedTextBuffer);
            m_pFormattedTextBuffer = nullptr;
        }

        m_sTextBufferSize = 0;
        m_sFormattedTextBufferSize = 0;
    }

public:
    CEditCtlText()
    {
        AllocateMemoryBuffer(1024);
    }

    ~CEditCtlText()
    {
        FreeMemoryBuffer();
    }

    void* FormatTextBufferForEditCtl(HWND hEditCtlWnd)
    {
        int iExtraTextChar = 0;
        TEXTMETRIC tmEditCtl = {};
        HDC hEditCtlDC = 0;
        RECT rctEditCtl = {};
        int iCharsPerLine = 0;
        int iCopiedBytes = 0;

        void* pSrcBuffer = nullptr;
        void* pDstBuffer = nullptr;


        if (hEditCtlWnd == 0 || m_pTextBuffer == nullptr || m_pFormattedTextBuffer == nullptr)
        {
            return nullptr;
        }

        hEditCtlDC = GetDC(hEditCtlWnd);

        iExtraTextChar = GetTextCharacterExtra(hEditCtlDC);
        GetTextMetrics(hEditCtlDC, &tmEditCtl);

        ReleaseDC(hEditCtlWnd, hEditCtlDC);

        GetClientRect(hEditCtlWnd, &rctEditCtl);

        iCharsPerLine = rctEditCtl.right / (((tmEditCtl.tmAveCharWidth + tmEditCtl.tmAveCharWidth + tmEditCtl.tmMaxCharWidth) / 3) + iExtraTextChar);

        pSrcBuffer = m_pTextBuffer;
        pDstBuffer = m_pFormattedTextBuffer;

        memset(m_pFormattedTextBuffer, 0, m_sFormattedTextBufferSize);

        while (iCopiedBytes <= m_sTextBufferSize)
        {
            memcpy_s(pDstBuffer, iCharsPerLine, pSrcBuffer, iCharsPerLine);
            pDstBuffer = (void*)((__int64)pDstBuffer + iCharsPerLine);
            pSrcBuffer = (void*)((__int64)pSrcBuffer + iCharsPerLine);

            strcpy_s((char*)pDstBuffer, 3, "\r\n");

            pDstBuffer = (void*)((__int64)pDstBuffer + 2);

            iCopiedBytes += iCharsPerLine;
        }

        return m_pFormattedTextBuffer;
    }

    void SetMemoryBufferToValue( int iClearValue )
    {
        if (m_pTextBuffer)
        {
            memset(m_pTextBuffer, iClearValue, m_sTextBufferSize);
        }
    }

    bool AllocateMemoryBuffer(size_t _bytes_to_allocate)
    {
        FreeMemoryBuffer();

        m_pTextBuffer = malloc(_bytes_to_allocate);
        m_pFormattedTextBuffer = malloc(_bytes_to_allocate * 2);

        if (m_pTextBuffer == nullptr || m_pFormattedTextBuffer == nullptr)
        {
            return false;
        }

        m_sTextBufferSize = _bytes_to_allocate;
        m_sFormattedTextBufferSize = _bytes_to_allocate * 2;

        return true;
    }

    void ReplaceRangeOfByteValues(unsigned char ucRangeStart, unsigned char ucRangeEnd, unsigned char ucReplaceValue)
    {
        unsigned char* pByte = nullptr;

        if (m_pTextBuffer == nullptr || m_sTextBufferSize == 0)
        {
            return;
        }

        for (int iByteIndex = 0; iByteIndex < m_sTextBufferSize; iByteIndex++)
        {
            pByte = &((unsigned char*)m_pTextBuffer)[iByteIndex];

            if ( *pByte >= ucRangeStart && *pByte <= ucRangeEnd )
            {
                *pByte = ucReplaceValue;
            }
        }
    }

    void* GetMemoryPointer()
    {
        return m_pTextBuffer;
    }

    ///////////////////////////////////////////////////////
    // ConvertToHexFromByteBuffer
    // Return: true if successful, otherwise false
    ///////////////////////////////////////////////////////
    static bool ConvertToHexFromByteBuffer(__in void* pByteBuffer, __in size_t sBufferSize, __inout void* pHexBuffer, __in size_t sHexBufferSize)
    {
        unsigned char* pByte;
        unsigned char pNibbleHigh;
        unsigned char pNibbleLow;
        //sCombinedByte *pFileByte = nullptr;
        //
        ////Create a conversion of all the bytes to a 2 character hex value
        //for (DWORD dwIndex = 0; dwIndex < dwFileSize; dwIndex++)
        //{
        //    pFileByte = (sCombinedByte*)(&((BYTE*)pFileBuffer)[dwIndex]);
        //
        //    ((char*)pHexBuffer)[dwIndex * 3] = (pFileByte->sByte.ucHigh <= 9 ? (48 + pFileByte->sByte.ucHigh) : (56 + pFileByte->sByte.ucHigh));
        //    ((char*)pHexBuffer)[dwIndex * 3 + 1] = (pFileByte->sByte.ucLow <= 9 ? (48 + pFileByte->sByte.ucLow) : (56 + pFileByte->sByte.ucLow));
        //    ((char*)pHexBuffer)[dwIndex * 3 + 2] = ' ';
        //}

        if (pByteBuffer == nullptr || pHexBuffer == nullptr)
        {
            return false;
        }

        if (sHexBufferSize < (3 * sBufferSize))
        {
            return false;
        }

        for (int iByteIndex = 0; iByteIndex < sBufferSize; iByteIndex++)
        {
            pByte = &((BYTE*)pByteBuffer)[iByteIndex];

            pNibbleHigh = (*pByte & 0xF0) >> 4;
            pNibbleLow = (*pByte & 0x0F);

        
            ((char*)pHexBuffer)[iByteIndex * 3] = (pNibbleHigh <= 9 ? (48 + pNibbleHigh) : (55 + pNibbleHigh));
            ((char*)pHexBuffer)[iByteIndex * 3 + 1] = (pNibbleLow <= 9 ? (48 + pNibbleLow) : (55 + pNibbleLow));
            ((char*)pHexBuffer)[iByteIndex * 3 + 2] = ' ';
        }


        return true;
    }

};//end of class CEditCtlMemory

CEditCtlText g_mmHexCtl;
CEditCtlText g_mmAsciiCtl;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    HMODULE hLoadedModule = LoadLibrary(TEXT("msftedit.dll"));

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HEXVIEWER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HEXVIEWER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HEXVIEWER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HEXVIEWER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pControlMsgHdr;
    wchar_t wszCommandInformation[256];

    switch (message)
    {
    case WM_CREATE:
        LoadChildDialog(GetModuleHandle(NULL), hWnd, IDD_HEXVIEWER, HexViewerDlgProc );
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_FILE_OPEN:
                OpenHexFile(hWnd);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_NOTIFY:
        pControlMsgHdr = (LPNMHDR)lParam;
        wsprintf(wszCommandInformation, L"The WM_NOTIFY Message Was Issued With Code: %x\n", pControlMsgHdr->code);
        OutputDebugStringW(wszCommandInformation);
        return static_cast<INT_PTR>(TRUE);
    case WM_PARENTNOTIFY:
        wsprintf(wszCommandInformation, L"The WM_PARENTNOTIFY Message Was Issued With Code: %x\n", (UINT)LOWORD(wParam) );
        OutputDebugStringW(wszCommandInformation);
        return static_cast<INT_PTR>(TRUE);
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        SendMessage(GetWindow(hWnd, GW_CHILD), WM_SIZE, 0, 0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL LoadChildDialog(HINSTANCE hInstance, HWND hParentWnd, INT iDialogID, DLGPROC lpDialogProc)
{
    HWND hDialogWnd;

    DWORD dwErrorCode;
    LPVOID lpMsgBuffer;

    hDialogWnd = CreateDialog(hInstance, MAKEINTRESOURCE(iDialogID), hParentWnd, lpDialogProc);
    
    if (hDialogWnd == NULL)
    {
        dwErrorCode = GetLastError();
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                       NULL,
                       dwErrorCode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPWSTR>(&lpMsgBuffer),
                       0,
                       NULL);

        OutputDebugStringW(reinterpret_cast<LPWSTR>(lpMsgBuffer));

        LocalFree(lpMsgBuffer);
    }

    //SetParent(hDialogWnd, hParentWnd);
    ShowWindow(hDialogWnd, SW_SHOW);

    return TRUE;
}


//////////////////////////////////////////////////////////////
// Resize the dialog and all child controls based
// upon the parent window size
//////////////////////////////////////////////////////////////
VOID ResizeDialog(HWND hDialogWindow)
{
    HWND hParentWindow;
    RECT rctClientArea = {};
    RECT rctControlArea = {};

    hParentWindow = GetParent(hDialogWindow);
    GetClientRect(hParentWindow, &rctClientArea);

    SetWindowPos(hDialogWindow, HWND_TOP, rctClientArea.left, rctClientArea.top, rctClientArea.right, rctClientArea.bottom, 0);

    //Modify the static text control size & position
    HWND hControl = GetDlgItem(hDialogWindow, IDC_ASCII_FILE);

    rctControlArea.left = rctClientArea.left + 10;
    rctControlArea.right = (DWORD)((float)rctClientArea.right * (2.0f/3.0f)) - 10;
    rctControlArea.top = rctClientArea.top + 10;
    rctControlArea.bottom = rctClientArea.bottom - 20;

    MoveWindow(hControl, rctControlArea.left, rctControlArea.top, rctControlArea.right, rctControlArea.bottom, TRUE);

    //Modify the tree view control size & position
    hControl = GetDlgItem(hDialogWindow, IDC_HEX_FILE);

    rctControlArea.left = (DWORD)((float)rctClientArea.right * (2.0f/3.0f)) + 10;
    rctControlArea.right = rctClientArea.right - rctControlArea.left - 10;
    rctControlArea.top = rctClientArea.top + 10;
    rctControlArea.bottom = rctClientArea.bottom - 20;

    MoveWindow(hControl, rctControlArea.left, rctControlArea.top, rctControlArea.right, rctControlArea.bottom, TRUE);
}

//BOOL GenerateFontForEditCtrl(HWND hDialogWnd, INT iEditCtlID)
//{
//    HFONT hEditCtlFont = 0;
//    HWND hEditCtlWnd = 0;
//
//    hEditCtlFont = CreateFont(0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, NULL);
//
//    if (hEditCtlFont && hDialogWnd)
//    {
//        hEditCtlWnd = GetDlgItem(hDialogWnd, iEditCtlID);
//        SendMessageW(hEditCtlWnd, WM_SETFONT, (WPARAM)hEditCtlFont, MAKELPARAM(TRUE,0));
//    }
//    else
//    {
//        return FALSE;
//    }
//
//    return TRUE;
//}

void EditCtrlGainedFocus(HWND hEditCtl)
{
    int iExtraTextChar = 0;
    int iCaretWidth = 0;
    TEXTMETRIC tmEditCtl = {};
    SIZE sTextExtent;
    HDC hEditCtlDC = 0;
    
    hEditCtlDC = GetDC(hEditCtl);

    
    GetTextMetrics(hEditCtlDC, &tmEditCtl);
    GetTextExtentPoint32(hEditCtlDC, L"X", 1, &sTextExtent);

    ReleaseDC(hEditCtl, hEditCtlDC);

    if (GetDlgCtrlID(hEditCtl) == IDC_ASCII_FILE)
    {
        iCaretWidth = sTextExtent.cx;
    }
    else
    {
        iCaretWidth = sTextExtent.cx * 2;
    }
    
    CreateCaret(hEditCtl, 0, iCaretWidth, sTextExtent.cy - 3);
    ShowCaret(hEditCtl);
}

void EditCtrlLostFocus(HWND hEditCtl)
{
    DestroyCaret();
}

void AlignEditCtlSelToCaret(HWND hDialogWnd)
{
    wchar_t wszCommandInformation[256];

    HWND hFocusCtl = 0;

    HWND hEditAsciiCtl = 0;
    HWND hEditHexCtl = 0;

    HWND hEditSrcCtl = 0;
    HWND hEditDstCtl = 0;
    
    int iDstVisibleLine = 0;
    int iDstDesiredLine = 0;

    int iSrcRatio = 0;
    int iDstRatio = 0;

    int iSrcSelLength = 0;
    int iDstSelLength = 0;

    //Rich Edit Control
    //CHARRANGE crSrcSelect = {};
    //CHARRANGE crDstSelect = {};

    int iSrcCharIdx = 0;
    int iDstCharIdx = 0;

    LRESULT lpReturn;

    POINT ptCursorPos = {};

    if (hDialogWnd == 0)
    {
        return;
    }

    hEditAsciiCtl = GetDlgItem(hDialogWnd, IDC_ASCII_FILE);
    hEditHexCtl = GetDlgItem(hDialogWnd, IDC_HEX_FILE);

    if (hEditAsciiCtl == 0 || hEditHexCtl == 0)
    {
        return;
    }

    GetCursorPos(&ptCursorPos);
    hFocusCtl = GetFocus();
    
    if (hFocusCtl == hEditAsciiCtl)
    {
        hEditSrcCtl = hEditAsciiCtl;
        iSrcRatio = 1;
        iSrcSelLength = 1;

        hEditDstCtl = hEditHexCtl;
        iDstRatio = 3;
        iDstSelLength = 2;
    }
    else if (hFocusCtl == hEditHexCtl)
    {
        hEditSrcCtl = hEditHexCtl;
        iSrcRatio = 3;
        iSrcSelLength = 2;

        hEditDstCtl = hEditAsciiCtl;
        iDstRatio = 1;
        iDstSelLength = 1;
    }
    else
    {
        return;
    }

    ScreenToClient(hFocusCtl, &ptCursorPos);

    //Edit Control Message
    lpReturn = SendMessageA(hEditSrcCtl, EM_CHARFROMPOS, 0, MAKELPARAM(ptCursorPos.x,ptCursorPos.y));
    
    //Rich Edit Message
    //lpReturn = SendMessageA(hEditSrcCtl, EM_CHARFROMPOS, 0, (LPARAM)&ptCursorPos);

    //Edit Control Return
    iSrcCharIdx = LOWORD(lpReturn);

    //Rich Edit Control Return
    //iSrcCharIdx = static_cast<int>(lpReturn);


    iDstCharIdx = ((iSrcCharIdx * iDstRatio) / iSrcRatio);

    wsprintf(wszCommandInformation, L"Source Edit Control Index: %5i Destination Edit Control Index: %i\n", iSrcCharIdx, iDstCharIdx);
    OutputDebugStringW(wszCommandInformation);
    
    //Edit Control Selection
    SendMessageA(hEditSrcCtl, EM_SETSEL, (WPARAM)iSrcCharIdx, (LPARAM)(iSrcCharIdx + iSrcSelLength));
    SendMessageA(hEditDstCtl, EM_SETSEL, (WPARAM)iDstCharIdx, (LPARAM)(iDstCharIdx + iDstSelLength));

    //Rich Edit Control Selection
    //crSrcSelect.cpMin = iSrcCharIdx - 1;
    //crSrcSelect.cpMax = iSrcCharIdx - 1 + iSrcSelLength;
    //crDstSelect.cpMin = iDstCharIdx - 1;
    //crDstSelect.cpMax = iDstCharIdx - 1 + iDstSelLength;
    //
    //SendMessageA(hEditSrcCtl, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&crSrcSelect));
    //SendMessageA(hEditDstCtl, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&crDstSelect));

    lpReturn = SendMessageA(hEditDstCtl, EM_GETFIRSTVISIBLELINE, 0, 0);
    iDstVisibleLine = (int)lpReturn;
    lpReturn = SendMessageA(hEditDstCtl, EM_LINEFROMCHAR, (WPARAM)-1, 0);
    iDstDesiredLine = (int)lpReturn;

    wsprintf(wszCommandInformation, L"Destination Edit Control Visible Line: %5i Desired Line: %i\n", iDstVisibleLine, iDstDesiredLine);
    OutputDebugStringW(wszCommandInformation);

    SendMessageA(hEditDstCtl, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
    SendMessageA(hEditDstCtl, EM_LINESCROLL, 0, (LPARAM)iDstDesiredLine);

}

//////////////////////////////////////////////////////////////
// HexViewer dialog proc
//
//////////////////////////////////////////////////////////////
INT_PTR CALLBACK HexViewerDlgProc(HWND hDialogWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    //void* pFileBuffer = nullptr;
    //HWND hEditAsciiCtrl;
    //HWND hAsciiCtlWnd;
    POINT ptMouseButton;
    HWND hMouseActivateWnd;
    INT  iMouseActivateCtlID;
    LPNMHDR pControlMsgHdr;
    UINT uiMouseMessage;

    wchar_t wszCommandInformation[256];

    switch (uiMessage)
    {
    case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
            case EN_SETFOCUS:
                EditCtrlGainedFocus((HWND)lParam);
                break;
            case EN_KILLFOCUS:
                EditCtrlLostFocus((HWND)lParam);
                break;
            default:
                wsprintf(wszCommandInformation, L"The Following Command Was Issued: %x\n", (UINT)HIWORD(wParam));
                OutputDebugStringW(wszCommandInformation);
                break;
            }
        }
        return static_cast<INT_PTR>(TRUE);
        break;
    //case WM_LBUTTONDOWN:
    //    ptMouseButton.x = GET_X_LPARAM(lParam);
    //    ptMouseButton.y = GET_Y_LPARAM(lParam);
    //    hMouseClickWnd = WindowFromPoint(ptMouseButton);
    //    switch (GetWindowID(hMouseClickWnd))
    //    {
    //    case IDC_ASCII_FILE:
    //        OutputDebugStringW(L"Clicked Inside ASCII Edit Control\n");
    //        break;
    //    case IDC_HEX_FILE:
    //        OutputDebugStringW(L"Clicked Inside Hex Edit Control\n");
    //        break;
    //    }
    //    break;
    case WM_INITDIALOG:
        //GenerateFontForEditCtrl(hDialogWnd, IDC_HEX_FILE);
        return static_cast<INT_PTR>(TRUE);
    case WM_NOTIFY:
        pControlMsgHdr = (LPNMHDR)lParam;
        wsprintf(wszCommandInformation, L"The WM_NOTIFY Message Was Issued With Code: %x\n", pControlMsgHdr->code);
        OutputDebugStringW(wszCommandInformation);
        return static_cast<INT_PTR>(TRUE);
    case WM_PARENTNOTIFY:
        wsprintf(wszCommandInformation, L"The WM_PARENTNOTIFY Message Was Issued With Code: %x\n", (UINT)LOWORD(wParam) );
        OutputDebugStringW(wszCommandInformation);
        return static_cast<INT_PTR>(TRUE);
    case WM_MOUSEACTIVATE:
        hMouseActivateWnd = (HWND)wParam;
        
        GetCursorPos(&ptMouseButton);
        //wsprintf(wszCommandInformation, L"The WM_MOUSEACTIVATE Message Was Issued Physical Cursor Point: %i,%i\n", ptMouseButton.x, ptMouseButton.y );
        //OutputDebugStringW(wszCommandInformation);

        ScreenToClient(hMouseActivateWnd, &ptMouseButton);
        //wsprintf(wszCommandInformation, L"The WM_MOUSEACTIVATE Message Was Issued Client Point: %i,%i\n", ptMouseButton.x, ptMouseButton.y );
        //OutputDebugStringW(wszCommandInformation);

        //hMouseActivateWnd = WindowFromPhysicalPoint(ptMouseButton);
        hMouseActivateWnd = ChildWindowFromPointEx(hDialogWnd, ptMouseButton, CWP_ALL);
        //hMouseActivateWnd = RealChildWindowFromPoint(hMouseActivateWnd, ptMouseButton);

        iMouseActivateCtlID = GetDlgCtrlID(hMouseActivateWnd);

        uiMouseMessage = HIWORD(lParam);
        //
        //switch (uiMouseMessage)
        //{
        //case WM_LBUTTONDOWN:
        //    OutputDebugStringW(L"L Button Down\n");
        //    break;
        //case WM_LBUTTONUP:
        //    OutputDebugStringW(L"L Button Up\n");
        //    break;
        //case WM_RBUTTONDOWN:
        //    OutputDebugStringW(L"R Button Down\n");
        //    break;
        //case WM_RBUTTONUP:
        //    OutputDebugStringW(L"R Button Up\n");
        //    break;
        //default:
        //    OutputDebugStringW(L"Unknown Mouse Message\n");
        //    break;
        //}

        if ( uiMouseMessage == WM_LBUTTONDOWN && ( iMouseActivateCtlID == IDC_ASCII_FILE || iMouseActivateCtlID == IDC_HEX_FILE) )
        {
            AlignEditCtlSelToCaret(hDialogWnd);
        }

        return static_cast<INT_PTR>(MA_ACTIVATE);
    case WM_SIZE:
        ResizeDialog(hDialogWnd);
        //hEditAsciiCtrl = GetDlgItem(hDialogWnd, IDC_ASCII_FILE);
        //
        //pFileBuffer = g_mmAsciiCtl.FormatTextBufferForEditCtl(hEditAsciiCtrl);
        //
        //SendMessageA(hEditAsciiCtrl, WM_SETTEXT, 0, (LPARAM)pFileBuffer);

        return static_cast<INT_PTR>(TRUE);
    //default:
    //    wsprintf(wszCommandInformation, L"The Following Message Was Issued: %x\n", uiMessage);
    //    OutputDebugStringW(wszCommandInformation);
    //    break;
    }

    return static_cast<INT_PTR>(FALSE);
}

BOOL OpenHexFile(HWND hOwnerWindow)
{
    wstring wstrFileName;
    HANDLE hOpenFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize, dwFileSizeHigh, dwBytesRead;
    LPVOID pFileBuffer = nullptr;
    LPVOID pHexBuffer = nullptr;

    //Get a file name from using the common control "Open" dialog
    if (FALSE == OpenFileDialog(hOwnerWindow, &wstrFileName))
    {
        return FALSE;
    }

    //Attempt to open the return file name
    hOpenFile = CreateFile2(wstrFileName.data(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
    
    if (hOpenFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    dwFileSize = GetFileSize(hOpenFile, &dwFileSizeHigh);

    if (dwFileSize == INVALID_FILE_SIZE)
    {
        CloseHandle(hOpenFile);
        return FALSE;
    }

    //allocate for the file size
    g_mmAsciiCtl.AllocateMemoryBuffer(dwFileSize + 2);
    g_mmAsciiCtl.SetMemoryBufferToValue(0);
    g_mmHexCtl.AllocateMemoryBuffer((dwFileSize * 3) + 2);
    g_mmHexCtl.SetMemoryBufferToValue(0);

    //pFileBuffer = malloc(dwFileSize + 2);
    //pHexBuffer = malloc((dwFileSize * 3) + 1);

    pFileBuffer = g_mmAsciiCtl.GetMemoryPointer();
    pHexBuffer = g_mmHexCtl.GetMemoryPointer();

    if (pFileBuffer == nullptr || pHexBuffer == nullptr)
    {
        CloseHandle(hOpenFile);
        return FALSE;
    }


    if (FALSE == ReadFile(hOpenFile, pFileBuffer, dwFileSize, &dwBytesRead, NULL))
    {
        CloseHandle(hOpenFile);
        free(pFileBuffer);
        return FALSE;
    }

    g_mmHexCtl.ConvertToHexFromByteBuffer(pFileBuffer, (size_t)dwFileSize, pHexBuffer, (size_t)(dwFileSize * 3));

    //sCombinedByte *pFileByte = nullptr;
    //
    ////Create a conversion of all the bytes to a 2 character hex value
    //for (DWORD dwIndex = 0; dwIndex < dwFileSize; dwIndex++)
    //{
    //    pFileByte = (sCombinedByte*)(&((BYTE*)pFileBuffer)[dwIndex]);
    //
    //    ((char*)pHexBuffer)[dwIndex * 3] = (pFileByte->sByte.ucHigh <= 9 ? (48 + pFileByte->sByte.ucHigh) : (56 + pFileByte->sByte.ucHigh));
    //    ((char*)pHexBuffer)[dwIndex * 3 + 1] = (pFileByte->sByte.ucLow <= 9 ? (48 + pFileByte->sByte.ucLow) : (56 + pFileByte->sByte.ucLow));
    //    ((char*)pHexBuffer)[dwIndex * 3 + 2] = ' ';
    //}

    //Change all unsupported characters to spaces
    g_mmAsciiCtl.ReplaceRangeOfByteValues(0, 9, (unsigned char)' ');
    g_mmAsciiCtl.ReplaceRangeOfByteValues(11, 12, (unsigned char)' ');
    g_mmAsciiCtl.ReplaceRangeOfByteValues(14, 31, (unsigned char)' ');
    g_mmAsciiCtl.ReplaceRangeOfByteValues(127, 127, (unsigned char)' ');
    g_mmAsciiCtl.ReplaceRangeOfByteValues(255, 255, (unsigned char)' ');

    //unsigned char* pByte = nullptr;
    //
    //for (int iByteIndex = 0; iByteIndex < (int)dwFileSize; iByteIndex++)
    //{
    //    pByte = &((unsigned char*)pFileBuffer)[iByteIndex];
    //
    //    if ( *pByte >= 0 && *pByte <= 32 )
    //    {
    //        *pByte = '.';
    //    }
    //}

    //((char*)pFileBuffer)[dwFileSize - 1] = '\0';

    HWND hChildWindow = GetWindow(hOwnerWindow, GW_CHILD);
    HWND hEditAsciiCtrl = GetDlgItem(hChildWindow, IDC_ASCII_FILE);
    HWND hEditHexCtrl = GetDlgItem(hChildWindow, IDC_HEX_FILE);


    //pFileBuffer = g_mmAsciiCtl.FormatTextBufferForEditCtl(hEditAsciiCtrl);
    //SETTEXTEX sttRichEdit;
    //
    //sttRichEdit.flags = ST_DEFAULT;
    //sttRichEdit.codepage = -1;
    
    //Edit_SetText(hEditHexCtrl, (LPCTSTR)pFileBuffer);
    //SetWindowTextW(hEditHexCtrl, (LPCWSTR)pFileBuffer);
    SendMessageA(hEditHexCtrl, WM_SETTEXT, 0, (LPARAM)pHexBuffer);
    SendMessageA(hEditAsciiCtrl, WM_SETTEXT, 0, (LPARAM)pFileBuffer);

    //SendMessageW(hEditAsciiCtrl, EM_SETTEXTEX, (WPARAM)&sttRichEdit, (LPARAM)pFileBuffer);

    //free(pFileBuffer);
    //free(pHexBuffer);
    CloseHandle(hOpenFile);

    return TRUE;
}



BOOL OpenFileDialog(HWND hOwnerWindow, wstring* pstrFileName)
{
    OPENFILENAMEW ofnHexFile;
    wchar_t wszFileName[32767];

    memset(&ofnHexFile, 0, sizeof(ofnHexFile));
    memset(&wszFileName, 0, sizeof(wszFileName));

    //Set the open file dialog structure information
    ofnHexFile.lStructSize = sizeof(ofnHexFile);
    ofnHexFile.hwndOwner = hOwnerWindow;
    ofnHexFile.lpstrFilter = TEXT("All Files\0*.*\0\0");
    ofnHexFile.lpstrFile = wszFileName;
    ofnHexFile.nMaxFile = sizeof(wszFileName);
    ofnHexFile.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_FORCESHOWHIDDEN | OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_DONTADDTORECENT;
    ofnHexFile.FlagsEx = OFN_EX_NOPLACESBAR;

    if (FALSE == GetOpenFileNameW(&ofnHexFile))
    {
        return FALSE;
    }

    pstrFileName->clear();
    pstrFileName->assign(wszFileName);

    return TRUE;
}

