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
#include "MyUtil.h"
#include "Const.h"

//--------------------------------------------------
// グローバル変数
//--------------------------------------------------
HINSTANCE   g_hInstance = nullptr;
WCHAR       g_szAppTitle[MAX_LOADSTRING];           // タイトル バーのテキスト
WCHAR       g_szWindowClass[MAX_LOADSTRING];        // メイン ウィンドウ クラス名
HWND        g_hWnd = nullptr;

// 高DPI対応
int         g_dpiX = 96;
int         g_dpiY = 96;
float       g_dpiScale = g_dpiX / 96.0f;

// ワーカースレッド
DWORD       g_threadId = 0;
CWorker*    g_pWorker = nullptr;

// 描画関連
MeterDrawer g_meterDrawer;

// 設定データ
IniConfig*  g_pIniConfig = nullptr;

// ドラッグ中
bool        g_dragging = false;

// 設定画面
HWND        g_hConfigDlgWnd = nullptr;

// 全ディスプレイ情報
std::vector<RECT> g_desktops;


//--------------------------------------------------
// プロトタイプ宣言
//--------------------------------------------------
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                UpdateMyWindowSize(WPARAM wParam, LPARAM lParam);
void                ClipMovingArea(LPRECT rcDesktop, LPRECT rcWindow);
void                OnPaint(const HWND &hWnd);
void                OnMouseMove(const HWND &hWnd, const WPARAM & wParam, const LPARAM & lParam);
void                ShowConfigDlg(HWND &hWnd);
void                AddTaskTrayIcon(const HWND &hWnd);
void                RemoveTaskTrayIcon(const HWND &hWnd);
void                ShowPopupMenu(const HWND &hWnd, POINT &pt);
void                ToggleBorder();
void                ToggleDebugMode();
void                ToggleAlwaysOnTop(const HWND &hWnd);
void                ToggleFitToDesktop();
BOOL CALLBACK       MyMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);


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

    Logger::Setup(L"log.txt");

    g_pIniConfig = new IniConfig();
    g_pIniConfig->Load();
    g_meterDrawer.InitMeterGuide();

    CString appName;
    MyUtil::GetAppNameWithVersion(appName);

    // 幅から最適な縦サイズを算出する
    const int width = g_pIniConfig->mWindowWidth;
    const int newHeight = MyUtil::CalcMeterWindowHeight(width);

    const HWND hWnd = CreateWindowW(g_szWindowClass, appName,
        WS_POPUP,
        g_pIniConfig->mPosX, g_pIniConfig->mPosY,
        width, newHeight, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }

    g_hWnd = hWnd;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    if (g_pIniConfig->mAlwaysOnTop) {
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    const HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COMPACTMETER));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // モードレスダイアログはここで処理しない
        if (g_hConfigDlgWnd != nullptr && IsDialogMessage(g_hConfigDlgWnd, &msg)) {
            continue;
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Logger::Close();
    
    return static_cast<int>(msg.wParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    // ReSharper disable once IdentifierTypo
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COMPACTMETER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = nullptr;// MAKEINTRESOURCEW(IDC_COMPACTMETER);
    wcex.lpszClassName  = g_szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static RECT rectDesktop;

    switch (message)
    {
    case WM_CREATE:
        {
            // DPI取得
            const auto dc = GetWindowDC(nullptr);
            g_dpiX = GetDeviceCaps(dc, LOGPIXELSX);
            g_dpiY = GetDeviceCaps(dc, LOGPIXELSY);
            g_dpiScale = g_dpiX / 96.0f;
            ReleaseDC(nullptr, dc);

    		// 全ディスプレイの座標を取得しておく
            g_desktops.clear();
            EnumDisplayMonitors(nullptr, nullptr, MyMonitorEnumProc, 0);

            // GDI+, Direct2D 初期化
            g_meterDrawer.Init(hWnd, g_pIniConfig->mWindowWidth, g_pIniConfig->mWindowHeight);

            // スレッド準備
            g_pWorker = new CWorker();
            g_pWorker->SetParams(hWnd);

            // スレッドの作成 
            const HANDLE hThread = CreateThread(nullptr, 0,
                CWorker::ThreadFunc, static_cast<LPVOID>(g_pWorker),
                CREATE_SUSPENDED, &g_threadId);
            if (hThread == nullptr) {
                exit(0);
            }

            // スレッドの起動
            ResumeThread(hThread);

            // タスクトレイに常駐
            AddTaskTrayIcon(hWnd);

            // デバッグモードでは設定画面を初期表示
            //if (g_pIniConfig->mDebugMode) {
            //    ShowConfigDlg(hWnd);
            //}
        }
        break;

    case WM_NOTIFYTASKTRAYICON:
//        Logger::d(L"0x%08x, 0x%08x", lParam, wParam);

        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
        switch (lParam) {
        case WM_RBUTTONUP:
            {
                POINT pt;
                GetCursorPos(&pt);
                ShowPopupMenu(hWnd, pt);
            }
            break;

        case WM_LBUTTONDOWN:
            {
                const BOOL visible = ::IsWindowVisible(hWnd);
                Logger::d(L"visible: %s", (visible ? L"yes" : L"no"));
                ::ShowWindow(hWnd, visible ? SW_HIDE : SW_SHOW);
            }
            break;
        }
        return 0L;

    case WM_COMMAND:
        {
            const int wmId = LOWORD(wParam);
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
            case ID_POPUPMENU_FIT_TO_DESKTOP:
                ToggleFitToDesktop();
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
        OnPaint(hWnd);
        break;
    	
    case WM_DISPLAYCHANGE:
        OnPaint(hWnd);

    	// ディスプレイの配置情報を再取得する
        g_desktops.clear();
        EnumDisplayMonitors(nullptr, nullptr, MyMonitorEnumProc, 0);

    	break;

    case WM_ERASEBKGND:
        // サイズ変更時にちらつかないようにする
        return TRUE;

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
            if (g_pIniConfig->mCloseByESC) {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
                return 0L;
            }
            break;

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

        case 'F':
            // Toggle Fit to Desktop
            ToggleFitToDesktop();
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

    case WM_SIZING:
        UpdateMyWindowSize(wParam, lParam);
        return TRUE;

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

            // リサイズ通知
            g_meterDrawer.Resize(hWnd, g_pIniConfig->mWindowWidth, g_pIniConfig->mWindowHeight);
        }
        return 0;

    // 移動範囲をデスクトップに限定する
    case WM_ENTERSIZEMOVE:
        if (g_pIniConfig->mFitToDesktop) {

        	// 現在のカーソル位置のデスクトップに限定する
            POINT pt;
            GetCursorPos(&pt);

        	// 念のため取得しておく
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rectDesktop, 0);

        	// 現在のデスクトップを探す
        	for (u_int i=0; i<g_desktops.size(); i++) {
        		if (PtInRect(&g_desktops[i], pt)) {
                    Logger::d(L"desktop: %d", i);
                    rectDesktop = g_desktops[i];
                    break;
        		}
        	}
        	
            Logger::d(L"desktop: [%d, %d], [%d, %d]", rectDesktop.left, rectDesktop.top, rectDesktop.right, rectDesktop.bottom);
            ClipCursor(&rectDesktop);
        }
        break;

    case WM_EXITSIZEMOVE:
        if (g_pIniConfig->mFitToDesktop) {
            ClipCursor(nullptr);
        }
        break;

    case WM_MOVING:
        if (g_pIniConfig->mFitToDesktop) {
            ClipMovingArea(&rectDesktop, reinterpret_cast<LPRECT>(lParam));
        }
        break;

    case WM_MOVE:
        {
            // 移動 => INIに位置を保存
            const int x = (int)(short) LOWORD(lParam);
            const int y = (int)(short)HIWORD(lParam);
//            Logger::d(L"WM_MOVE %d,%d", x, y);
            g_pIniConfig->mPosX = x;
            g_pIniConfig->mPosY = y;
            g_pIniConfig->Save();
        }
        return 0;

    case WM_GETMINMAXINFO:
        MINMAXINFO *pmmi;
        pmmi = reinterpret_cast<MINMAXINFO*>(lParam);
        pmmi->ptMinTrackSize.x = MAIN_WINDOW_MIN_WIDTH;
        pmmi->ptMinTrackSize.y = MAIN_WINDOW_MIN_HEIGHT;
        return 0;

    case WM_DPICHANGED:
        Logger::d(L"DPICHANGED, %d,%d", LOWORD(wParam), HIWORD(wParam));
        g_dpiX = LOWORD(wParam);
        g_dpiY = HIWORD(wParam);
        g_dpiScale = g_dpiX / 96.0f;
        g_meterDrawer.SetDpi(static_cast<float>(g_dpiX), static_cast<float>(g_dpiY));
        return 0;

    case WM_CONFIG_DLG_UPDATED:
        Logger::d(L"WM_CONFIG_DLG_UPDATED");
        g_pWorker->criticalSection.Lock();
        g_meterDrawer.InitMeterGuide();
        g_pWorker->criticalSection.Unlock();
        return 0;

    case WM_UPDATE_METER_WINDOW_SIZE:
        {
            // 幅から最適な縦サイズを算出する
            const int w = g_pIniConfig->mWindowWidth;
            const int h = MyUtil::CalcMeterWindowHeight(w);

            // 描画中にサイズ変更すると画面が乱れるためロックする
            g_meterDrawer.criticalSection.Lock();
            ::SetWindowPos(g_hWnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
            g_meterDrawer.criticalSection.Unlock();
            return 0;
        }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void UpdateMyWindowSize(WPARAM wParam, LPARAM lParam)
{
    RECT* rc = reinterpret_cast<RECT*>(lParam);
    const int width = rc->right - rc->left;
    const int height = rc->bottom - rc->top;

    Logger::d(L"Updating %d: width=%d, height=%d", wParam, width, height);

    // メーター設定から最適な縦サイズを算出する
    const int newHeight = MyUtil::CalcMeterWindowHeight(width);

    switch (wParam) {
    case WMSZ_LEFT:         // 左辺
    case WMSZ_RIGHT:        // 右辺
        rc->bottom = rc->top + newHeight;
        break;
    case WMSZ_TOP:          // 上辺
    case WMSZ_BOTTOM:       // 下辺
        // TODO 縦サイズから逆に幅を算出できるといい
        break;
    }
}

// ウインドウのクリッピング処理
void ClipMovingArea(LPRECT rcDesktop, LPRECT rcWindow)
{
    if (rcWindow->left < rcDesktop->left) {
        rcWindow->right = (rcDesktop->left + (rcWindow->right - rcWindow->left));
        rcWindow->left = (rcDesktop->left);
    }
    if (rcWindow->right > rcDesktop->right) {
        rcWindow->left = (rcDesktop->right - (rcWindow->right - rcWindow->left));
        rcWindow->right = (rcDesktop->right);
    }
    if (rcWindow->top < rcDesktop->top) {
        rcWindow->bottom = (rcDesktop->top + (rcWindow->bottom - rcWindow->top));
        rcWindow->top = (rcDesktop->top);
    }
    if (rcWindow->bottom > rcDesktop->bottom) {
        rcWindow->top = (rcDesktop->bottom - (rcWindow->bottom - rcWindow->top));
        rcWindow->bottom = (rcDesktop->bottom);
    }
}

void OnPaint(const HWND &hWnd)
{
    PAINTSTRUCT ps;
    const HDC hdc = BeginPaint(hWnd, &ps);

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

    const float BORDER_SIZE = 16 * g_dpiScale;
    const auto width = static_cast<float>(rt.right);
    const auto height = static_cast<float>(rt.bottom);

    // カーソル位置判定
    int cursorEdge = 0;    // WM_SIZING の wParam と同じ値(1～8)を設定する(範囲外は0)
    if (pt.x <= BORDER_SIZE && pt.y <= BORDER_SIZE)
        // 左上
        cursorEdge = WMSZ_TOPLEFT;
    else if (pt.x >= width - BORDER_SIZE && pt.y >= height - BORDER_SIZE)
        // 右下
        cursorEdge = WMSZ_BOTTOMRIGHT;
    else if (pt.x <= BORDER_SIZE && pt.y >= height - BORDER_SIZE)
        // 左下
        cursorEdge = WMSZ_BOTTOMLEFT;
    else if (pt.x >= width - BORDER_SIZE && pt.y <= BORDER_SIZE)
        // 右上
        cursorEdge = WMSZ_TOPRIGHT;
    else if (pt.x <= BORDER_SIZE)
        // 左辺
        cursorEdge = WMSZ_LEFT;
    else if (pt.x >= width - BORDER_SIZE)
        // 右辺
        cursorEdge = WMSZ_RIGHT;
    else if (pt.y <= BORDER_SIZE)
        // 上辺
        cursorEdge = WMSZ_TOP;
    else if (pt.y >= height - BORDER_SIZE)
        // 下辺
        cursorEdge = WMSZ_BOTTOM;


    // カーソル変更
    switch (cursorEdge) {
    //case WMSZ_TOPLEFT:
    //case WMSZ_BOTTOMRIGHT:
    //    SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZENWSE, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));
    //    break;
    //case WMSZ_TOPRIGHT:
    //case WMSZ_BOTTOMLEFT:
    //    SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZENESW, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));
    //    break;
    case WMSZ_LEFT:
    case WMSZ_RIGHT:
        SetCursor(static_cast<HCURSOR>(LoadImage(nullptr, IDC_SIZEWE, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED)));
        break;
    //case WMSZ_TOP:
    //case WMSZ_BOTTOM:
    //    SetCursor((HCURSOR)LoadImage(NULL, IDC_SIZENS, IMAGE_CURSOR, NULL, NULL, LR_DEFAULTCOLOR | LR_SHARED));
    //    break;
    }

    // ドラッグ中は非クライアント領域を偽装する
    if (wParam & MK_LBUTTON) {
        g_dragging = true;

        switch (cursorEdge) {
        //case WMSZ_TOPLEFT:
        //    // 左上
        //    SendMessage(hWnd, WM_NCLBUTTONDOWN, HTTOPLEFT, MAKELPARAM(pt.x, pt.y));
        //    break;
        //case WMSZ_BOTTOMRIGHT:
        //    // 右下
        //    SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, MAKELPARAM(pt.x, pt.y));
        //    break;
        //case WMSZ_TOPRIGHT:
        //    // 右上
        //    SendMessage(hWnd, WM_NCLBUTTONDOWN, HTTOPRIGHT, MAKELPARAM(pt.x, pt.y));
        //    break;
        //case WMSZ_BOTTOMLEFT:
        //    // 左下
        //    SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOMLEFT, MAKELPARAM(pt.x, pt.y));
        //    break;
        case WMSZ_LEFT:
            // 左辺
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTLEFT, MAKELPARAM(pt.x, pt.y));
            break;
        case WMSZ_RIGHT:
            // 右辺
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTRIGHT, MAKELPARAM(pt.x, pt.y));
            break;
        //case WMSZ_TOP:
        //    // 上辺
        //    SendMessage(hWnd, WM_NCLBUTTONDOWN, HTTOP, MAKELPARAM(pt.x, pt.y));
        //    break;
        //case WMSZ_BOTTOM:
        //    // 下辺
        //    SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOM, MAKELPARAM(pt.x, pt.y));
        //    break;
        default:
            // 外枠以外のドラッグで移動
            if (wParam & MK_SHIFT) {
                // Shift+ドラッグでリサイズ
                Logger::d(L"resize start");
                SendMessage(hWnd, WM_NCLBUTTONDOWN, HTRIGHT, MAKELPARAM(pt.x, pt.y));
                Logger::d(L"resize end");
            } else {
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
    if (g_hConfigDlgWnd == nullptr) {
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
    CheckMenuItem(hSubMenu, ID_POPUPMENU_FIT_TO_DESKTOP, MF_BYCOMMAND | (g_pIniConfig->mFitToDesktop ? MFS_CHECKED : MFS_UNCHECKED));
    CheckMenuItem(hSubMenu, ID_POPUPMENU_DEBUGMODE, MF_BYCOMMAND | (g_pIniConfig->mDebugMode ? MFS_CHECKED : MFS_UNCHECKED));
    CheckMenuItem(hSubMenu, ID_POPUPMENU_DRAW_BORDER, MF_BYCOMMAND | (g_pIniConfig->mDrawBorder ? MFS_CHECKED : MFS_UNCHECKED));

    SetForegroundWindow(hWnd);

    Logger::d(L"Show Popup menu");
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
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

    // メーターウィンドウサイズ更新
    ::PostMessage(g_hWnd, WM_UPDATE_METER_WINDOW_SIZE, 0, 0);
}

void ToggleAlwaysOnTop(const HWND &hWnd)
{
    g_pIniConfig->mAlwaysOnTop = !g_pIniConfig->mAlwaysOnTop;
    g_pIniConfig->Save();
    SetWindowPos(hWnd, g_pIniConfig->mAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void ToggleFitToDesktop()
{
    g_pIniConfig->mFitToDesktop = !g_pIniConfig->mFitToDesktop;
    g_pIniConfig->Save();

}

BOOL CALLBACK MyMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    MONITORINFOEX monitorInfo;

    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(hMonitor, &monitorInfo);

    Logger::d(L"MonitorInfo: %d, %d, %d, %d", monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right, monitorInfo.rcMonitor.bottom);
    g_desktops.push_back(monitorInfo.rcMonitor);

    return TRUE;
}
