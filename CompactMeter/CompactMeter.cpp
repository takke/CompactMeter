﻿// CompactMeter.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "AboutDlg.h"
#include "ConfigDlg.h"
#include "CompactMeter.h"
#include "Worker.h"
#include "MeterDrawer.h"
#include "IniConfig.h"
#include "Logger.h"
#include "Const.h"

using namespace Gdiplus;

//--------------------------------------------------
// グローバル変数
//--------------------------------------------------
HINSTANCE   g_hInstance = NULL;
WCHAR       g_szAppTitle[MAX_LOADSTRING];           // タイトル バーのテキスト
WCHAR       g_szWindowClass[MAX_LOADSTRING];        // メイン ウィンドウ クラス名

// 高DPI対応
int         g_dpix = 96;
int         g_dpiy = 96;
float       g_dpiScale = g_dpix / 96.0f;

// ワーカースレッド
DWORD       g_threadId = 0;
CWorker*    g_pWorker = NULL;

// 描画関連
MeterDrawer g_meterDrawer;

// 設定データ
IniConfig*  g_pIniConfig = NULL;

// ドラッグ中
boolean     g_dragging = false;

// 設定画面
HWND        g_hConfigDlgWnd = NULL;


//--------------------------------------------------
// プロトタイプ宣言
//--------------------------------------------------
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                OnPaint(const HWND &hWnd);
void                OnMouseMove(const HWND &hWnd, const WPARAM & wParam, const LPARAM & lParam);
void                ShowConfigDlg(HWND &hWnd);
void                AddTaskTrayIcon(const HWND &hWnd);
void                RemoveTaskTrayIcon(const HWND &hWnd);
void                ShowPopupMenu(const HWND &hWnd, POINT &pt);
void                ToggleBorder();
void                ToggleDebugMode();
void                ToggleAlwaysOnTop(const HWND &hWnd);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInstance = hInstance;

    // グローバル文字列を初期化
    LoadStringW(hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_COMPACTMETER, g_szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    Logger::Setup();

    g_pIniConfig = new IniConfig();
    g_pIniConfig->Load();

    HWND hWnd = CreateWindowW(g_szWindowClass, g_szAppTitle,
        WS_POPUP,
        g_pIniConfig->mPosX, g_pIniConfig->mPosY,
        g_pIniConfig->mWindowWidth, g_pIniConfig->mWindowHeight, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    if (g_pIniConfig->mAlwaysOnTop) {
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COMPACTMETER));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // モードレスダイアログはここで処理しない
        if (g_hConfigDlgWnd != NULL && IsDialogMessage(g_hConfigDlgWnd, &msg)) {
            continue;
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Logger::Close();
    
    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COMPACTMETER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;// MAKEINTRESOURCEW(IDC_COMPACTMETER);
    wcex.lpszClassName  = g_szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            // DPI取得
            auto dc = GetWindowDC(NULL);
            g_dpix = GetDeviceCaps(dc, LOGPIXELSX);
            g_dpiy = GetDeviceCaps(dc, LOGPIXELSY);
            g_dpiScale = g_dpix / 96.0f;
            ReleaseDC(NULL, dc);

            // GDI+, Direct2D 初期化
            g_meterDrawer.Init(hWnd, g_pIniConfig->mWindowWidth, g_pIniConfig->mWindowHeight);

            // スレッド準備
            g_pWorker = new CWorker();
            g_pWorker->SetParams(hWnd);

            // スレッドの作成 
            HANDLE hThread = CreateThread(NULL, 0,
                CWorker::ThreadFunc, (LPVOID)g_pWorker,
                CREATE_SUSPENDED, &g_threadId);
            if (hThread == FALSE) {
                exit(0);
            }

            // スレッドの起動
            ResumeThread(hThread);

            // タスクトレイに常駐
            AddTaskTrayIcon(hWnd);

//            if (g_pInConfig->mDebugMode) {
//                ShowConfigDlg(hWnd);
//            }
        }
        break;

    case WM_NOTIFYTASKTRAYICON:
//        Logger::d(L"%d, %d", lParam, wParam);
        switch (lParam) {
        case WM_RBUTTONUP:
            {
                POINT pt;
                GetCursorPos(&pt);
                ShowPopupMenu(hWnd, pt);
            }
            break;
        }
        return 0L;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case ID_POPUPMENU_SHOW_CONFIG_DIALOG:
                ShowConfigDlg(hWnd);
                return 0L;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_POPUPMENU_ALWAYSONTOP:
                ToggleAlwaysOnTop(hWnd);
                return 0L;
            case ID_POPUPMENU_DEBUGMODE:
                ToggleDebugMode();
                return 0L;
            case ID_POPUPMENU_DRAW_BORDER:
                ToggleBorder();
                return 0L;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
    case WM_DISPLAYCHANGE:
        OnPaint(hWnd);
        break;

    case WM_DESTROY:
        // 終了処理
        g_pWorker->Terminate();

        g_meterDrawer.Shutdown();

        RemoveTaskTrayIcon(hWnd);

        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
        case VK_F12:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            return 0L;

        case 'B':
            // Toggle Border
            ToggleBorder();
            return 0L;

        case 'D':
            // Toggle Debug Mode
            ToggleDebugMode();
            return 0L;

        case 'T':
            // Toggle AlwaysOnTop
            ToggleAlwaysOnTop(hWnd);
            return 0L;
        }
        break;

    case WM_RBUTTONUP:
        {
            // 右クリックメニュー表示
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
        
            ClientToScreen(hWnd, &pt);

            ShowPopupMenu(hWnd, pt);

            return 0;
        }

    case WM_MOUSEMOVE:
        OnMouseMove(hWnd, wParam, lParam);
        return 0L;

    case WM_SIZE:
        {
            // サイズ変更 => INIにサイズを保存
            RECT rectWindow;
            GetClientRect(hWnd, &rectWindow);
//          printf("size: %dx%d\n", rectWindow.right, rectWindow.bottom);

            Logger::d(L"サイズ変更 %dx%d => %dx%d",
                g_pIniConfig->mWindowWidth, g_pIniConfig->mWindowHeight,
                rectWindow.right, rectWindow.bottom);

            if (g_pIniConfig->mWindowWidth == rectWindow.right && g_pIniConfig->mWindowHeight == rectWindow.bottom) {
                Logger::d(L"同一なので無視");
                return 0;
            }

            g_pIniConfig->mWindowWidth = rectWindow.right;
            g_pIniConfig->mWindowHeight = rectWindow.bottom;
            g_pIniConfig->Save();

            // オフスクリーン再生成
            g_meterDrawer.Resize(hWnd, g_pIniConfig->mWindowWidth, g_pIniConfig->mWindowHeight);

        }
        return 0;

    case WM_MOVE:
        {
            // 移動 => INIに位置を保存
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_pIniConfig->mPosX = x;
            g_pIniConfig->mPosY = y;
            g_pIniConfig->Save();
        }
        return 0;

    case WM_GETMINMAXINFO:
        MINMAXINFO *pmmi;
        pmmi = (MINMAXINFO*)lParam;
        pmmi->ptMinTrackSize.x = 300;
        pmmi->ptMinTrackSize.y = 300;
        return 0;

    case WM_DPICHANGED:
        Logger::d(L"DPICHANGED, %d,%d", LOWORD(wParam), HIWORD(wParam));
        g_dpix = LOWORD(wParam);
        g_dpiy = HIWORD(wParam);
        g_dpiScale = g_dpix / 96.0f;
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void OnPaint(const HWND &hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    g_meterDrawer.DrawToDC(hdc, hWnd, g_pWorker);

    EndPaint(hWnd, &ps);
}

void OnMouseMove(const HWND &hWnd, const WPARAM & wParam, const LPARAM & lParam)
{
    POINT pt;
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    RECT rt;
    GetClientRect(hWnd, &rt);

    const float BORDER_SIZE = 8 * g_dpiScale;
    const float width = (float)rt.right;
    const float height = (float)rt.bottom;

    // カーソル変更
    if ((pt.x <= BORDER_SIZE) && (pt.y <= BORDER_SIZE) || (pt.x >= width - BORDER_SIZE) && (pt.y >= height - BORDER_SIZE))
        SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZENWSE, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));
    else if ((pt.x <= BORDER_SIZE) && (pt.y >= height - BORDER_SIZE) || (pt.x >= width - BORDER_SIZE) && (pt.y <= BORDER_SIZE))
        SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZENESW, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));
    else if ((pt.x <= BORDER_SIZE) || (pt.x >= width - BORDER_SIZE))
        SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZEWE, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));
    else if ((pt.y <= BORDER_SIZE) || (pt.y >= height - BORDER_SIZE))
        SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZENS, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));

    // ドラッグ中は非クライアント領域を偽装する
    if (wParam & MK_LBUTTON)
    {
        g_dragging = true;
        if (pt.x <= BORDER_SIZE && pt.y <= BORDER_SIZE)
            // 左上
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTTOPLEFT, MAKELPARAM(pt.x, pt.y));
        else if (pt.x >= rt.right - BORDER_SIZE && pt.y >= rt.bottom - BORDER_SIZE)
            // 右下
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, MAKELPARAM(pt.x, pt.y));
        else if (pt.x <= BORDER_SIZE && pt.y >= rt.bottom - BORDER_SIZE)
            // 左下
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOMLEFT, MAKELPARAM(pt.x, pt.y));
        else if (pt.x >= rt.right - BORDER_SIZE && pt.y <= BORDER_SIZE)
            // 右上
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTTOPRIGHT, MAKELPARAM(pt.x, pt.y));
        else if (pt.x <= BORDER_SIZE)
            // 左辺
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTLEFT, MAKELPARAM(pt.x, pt.y));
        else if (pt.x >= rt.right - BORDER_SIZE)
            // 右辺
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTRIGHT, MAKELPARAM(pt.x, pt.y));
        else if (pt.y <= BORDER_SIZE)
            // 上辺
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTTOP, MAKELPARAM(pt.x, pt.y));
        else if (pt.y >= rt.bottom - BORDER_SIZE)
            // 下辺
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOM, MAKELPARAM(pt.x, pt.y));
        else {
            // 外枠以外のドラッグで移動
            if (wParam & MK_SHIFT) {
                // Shift+ドラッグでリサイズ(暫定)
                Logger::d(L"resize start");
                SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, MAKELPARAM(pt.x, pt.y));
                Logger::d(L"resize end");
            }
            else {
                // ドラッグで移動
                Logger::d(L"drag start");
                SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
                Logger::d(L"drag end");
            }
        }
        g_dragging = false;

    }
}

void ShowConfigDlg(HWND &hWnd)
{
    if (g_hConfigDlgWnd == NULL) {
        Logger::d(L"Create config dlg");
        g_hConfigDlgWnd = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDlgProc);
    }
    Logger::d(L"Show config dlg");
    ShowWindow(g_hConfigDlgWnd, SW_SHOW);

    if (g_pIniConfig->mConfigDlgPosX == INT_MAX || g_pIniConfig->mConfigDlgPosY == INT_MAX) {
        Logger::d(L"初期値なので移動しない");
    }
    else {
        SetWindowPos(g_hConfigDlgWnd, HWND_TOP, g_pIniConfig->mConfigDlgPosX, g_pIniConfig->mConfigDlgPosY, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    SetForegroundWindow(g_hConfigDlgWnd);
}

void AddTaskTrayIcon(const HWND &hWnd)
{
    NOTIFYICONDATA nid;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = TRAYICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_NOTIFYTASKTRAYICON;
    nid.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_COMPACTMETER));
    wcscpy_s(nid.szTip, L"CompactMeter");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTaskTrayIcon(const HWND &hWnd)
{
    NOTIFYICONDATA nid;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = TRAYICON_ID;
    nid.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowPopupMenu(const HWND &hWnd, POINT &pt)
{
    static HMENU hMenu;
    static HMENU hSubMenu;

    hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU));
    hSubMenu = GetSubMenu(hMenu, 0);

    // 初期チェック状態を反映
    CheckMenuItem(hSubMenu, ID_POPUPMENU_ALWAYSONTOP, MF_BYCOMMAND | (g_pIniConfig->mAlwaysOnTop ? MFS_CHECKED : MFS_UNCHECKED));
    CheckMenuItem(hSubMenu, ID_POPUPMENU_DEBUGMODE, MF_BYCOMMAND | (g_pIniConfig->mDebugMode ? MFS_CHECKED : MFS_UNCHECKED));
    CheckMenuItem(hSubMenu, ID_POPUPMENU_DRAW_BORDER, MF_BYCOMMAND | (g_pIniConfig->mDrawBorder ? MFS_CHECKED : MFS_UNCHECKED));

    SetForegroundWindow(hWnd);

    Logger::d(L"Show Popup menu");
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    Logger::d(L"Close Popup menu");
}

void ToggleBorder()
{
    g_pIniConfig->mDrawBorder = !g_pIniConfig->mDrawBorder;
    g_pIniConfig->Save();
}

void ToggleDebugMode()
{
    g_pIniConfig->mDebugMode = !g_pIniConfig->mDebugMode;
    g_pIniConfig->Save();
}

void ToggleAlwaysOnTop(const HWND &hWnd)
{
    g_pIniConfig->mAlwaysOnTop = !g_pIniConfig->mAlwaysOnTop;
    g_pIniConfig->Save();
    SetWindowPos(hWnd, g_pIniConfig->mAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

