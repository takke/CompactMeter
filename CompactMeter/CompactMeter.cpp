// CompactMeter.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "AboutDlg.h"
#include "ConfigDlg.h"
#include "CompactMeter.h"
#include "Worker.h"
#include "MyInifileUtil.h"
#include "Logger.h"

using namespace Gdiplus;

#define MAX_LOADSTRING 100

#define PI 3.14159265f

//--------------------------------------------------
// グローバル変数
//--------------------------------------------------
DWORD threadId = 0;    // スレッド ID 
CWorker* pMyWorker = NULL;

HINSTANCE g_hInst;                              // 現在のインスタンス
WCHAR g_szAppTitle[MAX_LOADSTRING];             // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

Bitmap* g_offScreenBitmap = NULL;
Graphics* g_offScreen = NULL;

MyInifileUtil* g_pMyInifile = NULL;

// ドラッグ中
boolean g_dragging = false;

// タスクトレイ関連
#define WM_NOTIFYTASKTRAYICON   (WM_USER+100)
#define TRAYICON_ID             0

// 設定画面
HWND g_hConfigDlgWnd = NULL;

//--------------------------------------------------
// プロトタイプ宣言
//--------------------------------------------------
struct MeterColor {
    float percent;
    Color color;
};
struct MeterGuide {
    float percent;
    Color color;
    LPCWSTR text;
};
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                AddTaskTrayIcon(const HWND &hWnd);
void                RemoveTaskTrayIcon(const HWND &hWnd);
void                ShowPopupMenu(const HWND &hWnd, POINT &pt);
void                ToggleBorder();
void                ToggleDebugMode();
void                ToggleAlwaysOnTop(const HWND &hWnd);
void                DrawAll(HWND hWnd, HDC hdc, PAINTSTRUCT ps, CWorker* pWorker, Graphics* offScreen, Bitmap* offScreenBitmap);
void                DrawMeters(Graphics& g, HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight);
float               KbToPercent(float outb, const DWORD &maxTrafficBytes);
void                DrawMeter(Graphics& g, Gdiplus::RectF& rect, float percent, const WCHAR* str, MeterColor colors[], MeterGuide guideLines[], float scale);
void                DrawLineByAngle(Graphics& g, Pen* p, Gdiplus::PointF& center, float angle, float length1, float length2);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // グローバル文字列を初期化
    LoadStringW(hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_COMPACTMETER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    Logger::Setup();

    g_pMyInifile = new MyInifileUtil();
    g_pMyInifile->Load();

    // アプリケーション初期化
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
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
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, g_szAppTitle, 
                    WS_POPUP,
                    g_pMyInifile->mPosX, g_pMyInifile->mPosY,
                    g_pMyInifile->mWindowWidth, g_pMyInifile->mWindowHeight, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    if (g_pMyInifile->mAlwaysOnTop) {
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GdiplusStartupInput gdiSI;
    static ULONG_PTR gdiToken = NULL;

    switch (message)
    {
    case WM_CREATE:
        {
            // 初期化
            GdiplusStartup(&gdiToken, &gdiSI, NULL);

            // OffScreen
            g_offScreenBitmap = new Bitmap(g_pMyInifile->mWindowWidth, g_pMyInifile->mWindowHeight);
            g_offScreen = new Graphics(g_offScreenBitmap);

            // スレッド準備
            pMyWorker = new CWorker();
            pMyWorker->SetParams(hWnd);

            // スレッドの作成 
            HANDLE hThread = CreateThread(NULL, 0,
                CWorker::ThreadFunc, (LPVOID)pMyWorker,
                CREATE_SUSPENDED, &threadId);

            // スレッドの起動
            ResumeThread(hThread);

            // タスクトレイに常駐
            AddTaskTrayIcon(hWnd);
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
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
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
            case ID_POPUPMENU_SHOW_CONFIG_DIALOG:
                if (g_hConfigDlgWnd == NULL) {
                    Logger::d(L"Create config dlg");
                    g_hConfigDlgWnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDlgProc);
                }
                Logger::d(L"Show config dlg");
                ShowWindow(g_hConfigDlgWnd, SW_SHOW);
                SetForegroundWindow(g_hConfigDlgWnd);
                return 0L;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            DrawAll(hWnd, hdc, ps, pMyWorker, g_offScreen, g_offScreenBitmap);

            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        // 終了処理
        pMyWorker->Terminate();

        GdiplusShutdown(gdiToken);

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
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            RECT rt;
            GetClientRect(hWnd, &rt);

            const int BORDER_SIZE = 6;
            const int width = rt.right;
            const int height = rt.bottom;

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
        return 0L;

    case WM_SIZE:
        {
            // サイズ変更 => INIにサイズを保存
            RECT rectWindow;
            GetClientRect(hWnd, &rectWindow);
//          printf("size: %dx%d\n", rectWindow.right, rectWindow.bottom);

            Logger::d(L"サイズ変更 %dx%d => %dx%d",
                g_pMyInifile->mWindowWidth, g_pMyInifile->mWindowHeight,
                rectWindow.right, rectWindow.bottom);

            if (g_pMyInifile->mWindowWidth == rectWindow.right && g_pMyInifile->mWindowHeight == rectWindow.bottom) {
                Logger::d(L"同一なので無視");
                return 0;
            }

            g_pMyInifile->mWindowWidth = rectWindow.right;
            g_pMyInifile->mWindowHeight = rectWindow.bottom;
            g_pMyInifile->Save();

            // オフスクリーン再生成
            if (g_offScreenBitmap != NULL) {
                delete g_offScreenBitmap;
            }
            if (g_offScreen != NULL) {
                delete g_offScreen;
            }
            g_offScreenBitmap = new Bitmap(g_pMyInifile->mWindowWidth, g_pMyInifile->mWindowHeight);
            g_offScreen = new Graphics(g_offScreenBitmap);

        }
        return 0;

    case WM_MOVE:
        {
            // 移動 => INIに位置を保存
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_pMyInifile->mPosX = x;
            g_pMyInifile->mPosY = y;
            g_pMyInifile->Save();
        }
        return 0;

    case WM_GETMINMAXINFO:
        MINMAXINFO *pmmi;
        pmmi = (MINMAXINFO*)lParam;
        pmmi->ptMinTrackSize.x = 300;
        pmmi->ptMinTrackSize.y = 300;
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void AddTaskTrayIcon(const HWND &hWnd)
{
    NOTIFYICONDATA nid;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = TRAYICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_NOTIFYTASKTRAYICON;
    nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_COMPACTMETER));
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

    hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_POPUP_MENU));
    hSubMenu = GetSubMenu(hMenu, 0);

    // 初期チェック状態を反映
    CheckMenuItem(hSubMenu, ID_POPUPMENU_ALWAYSONTOP, MF_BYCOMMAND | (g_pMyInifile->mAlwaysOnTop ? MFS_CHECKED : MFS_UNCHECKED));
    CheckMenuItem(hSubMenu, ID_POPUPMENU_DEBUGMODE, MF_BYCOMMAND | (g_pMyInifile->mDebugMode ? MFS_CHECKED : MFS_UNCHECKED));
    CheckMenuItem(hSubMenu, ID_POPUPMENU_DRAW_BORDER, MF_BYCOMMAND | (g_pMyInifile->mDrawBorder ? MFS_CHECKED : MFS_UNCHECKED));

    SetForegroundWindow(hWnd);

    Logger::d(L"Show Popup menu");
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    Logger::d(L"Close Popup menu");
}

void ToggleBorder()
{
    g_pMyInifile->mDrawBorder = !g_pMyInifile->mDrawBorder;
    g_pMyInifile->Save();
}

void ToggleDebugMode()
{
    g_pMyInifile->mDebugMode = !g_pMyInifile->mDebugMode;
    g_pMyInifile->Save();
}

void ToggleAlwaysOnTop(const HWND &hWnd)
{
    g_pMyInifile->mAlwaysOnTop = !g_pMyInifile->mAlwaysOnTop;
    g_pMyInifile->Save();
    SetWindowPos(hWnd, g_pMyInifile->mAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void DrawAll(HWND hWnd, HDC hdc, PAINTSTRUCT ps, CWorker* pWorker, Graphics* offScreen, Bitmap* offScreenBitmap)
{
    Graphics& g = *offScreen;

    //--------------------------------------------------
    // dump
    //--------------------------------------------------
    SolidBrush backgroundBrush(Color(255, 10, 10, 10));
    float screenWidth = (float)g_pMyInifile->mWindowWidth;
    float screenHeight = (float)g_pMyInifile->mWindowHeight;
    Gdiplus::RectF rect = Gdiplus::RectF(0.0f, 0.0f, screenWidth, screenHeight);

    g.FillRectangle(&backgroundBrush, rect);

    if (pWorker != NULL && pWorker->traffics.size() >= 2) {

        pWorker->criticalSection.Lock();

        DrawMeters(g, hWnd, pWorker, screenWidth, screenHeight);

        pWorker->criticalSection.Unlock();
    }

    //--------------------------------------------------
    // 実画面に転送
    //--------------------------------------------------
    Graphics onScreen(hdc);
    onScreen.DrawImage(offScreenBitmap, 0, 0);
}

void DrawMeters(Graphics& g, HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight)
{
    Gdiplus::RectF rect;

    CString str;
    RECT rectWindow;
    GetClientRect(hWnd, &rectWindow);
    // 各ウィジェットのサイズ(width, height)
    float size = rectWindow.right / 2.0f;


    //--------------------------------------------------
    // CPU+Memory タコメーター描画
    //--------------------------------------------------
    static MeterColor cpuColors[] = {
        { 90.0, Color(255, 64, 64) },
        { 80.0, Color(255, 128, 64) },
        { 70.0, Color(192, 192, 64) },
        {  0.0, Color(192, 192, 192) }
    };
    static MeterGuide cpuGuides[] = {
        { 100.0, Color(255,  64,  64), L"" },
        {  90.0, Color(255,  64,  64), L"" },
        {  80.0, Color(255,  64,  64), L"" },
        {  70.0, Color(255,  64,  64), L"" },
        {  60.0, Color(192, 192, 192), L"" },
        {  50.0, Color(192, 192, 192), L"" },
        {  40.0, Color(192, 192, 192), L"" },
        {  30.0, Color(192, 192, 192), L"" },
        {  20.0, Color(192, 192, 192), L"" },
        {  10.0, Color(192, 192, 192), L"" },
        {   0.0, Color(192, 192, 192), L"" },
    };

    CpuUsage cpuUsage;
    int nCore = pWorker->GetCpuUsage(&cpuUsage);

    float y = 0;
    float height = size;

    // 全コアの合計
    {
        rect = Gdiplus::RectF(0, 0, size, size);
        float percent = cpuUsage.usages[0];
        str.Format(L"CPU (%.0f%%)", percent);
        DrawMeter(g, rect, percent, str, cpuColors, cpuGuides, 1);
    }

    // メモリ使用量
    {
        MEMORYSTATUSEX ms;

        ms.dwLength = sizeof(ms);
        GlobalMemoryStatusEx(&ms);

        //printf("dwMemoryLoad     %d\n", ms.dwMemoryLoad);
        //printf("ullTotalPhys     %I64d\n", ms.ullTotalPhys);         // 物理メモリの搭載容量
        //printf("ullAvailPhys     %I64d\n", ms.ullAvailPhys);         // 物理メモリの空き容量
        //printf("ullTotalPageFile %I64d\n", ms.ullTotalPageFile);     // ページングの搭載容量
        //printf("ullAvailPageFile %I64d\n", ms.ullAvailPageFile);     // ページングの空き容量
        //printf("ullTotalVirtual  %I64d\n", ms.ullTotalVirtual);      // 仮想メモリの搭載容量
        //printf("ullAvailVirtual  %I64d\n", ms.ullAvailVirtual);      // 仮想メモリの空き容量

        rect = Gdiplus::RectF(size, 0, size, size);
        DWORDLONG ullUsing = ms.ullTotalPhys - ms.ullAvailPhys;
        float percent = ullUsing * 100.0f / ms.ullTotalPhys;
        str.Format(L"Memory (%.0f%%)\n%I64d / %I64d MB", percent, ullUsing/1024/1024, ms.ullTotalPhys/1024/1024);
        DrawMeter(g, rect, percent, str, cpuColors, cpuGuides, 1);
    }

    //--------------------------------------------------
    // 各Core
    //--------------------------------------------------
    int div = 4;
    float scale = 2.0f / div;    // 1.0 or 0.5
    float coreSize = size * scale;
    float x = 0;
    for (int i = 0; i < nCore; i++) {
        if (i % div == 0) {
            // 左側
            x = 0;
            if (i == 0) {
                y += height;
            }
            else {
                y += coreSize;
            }
            if (y + coreSize >= screenHeight) {
                return;
            }

            rect = Gdiplus::RectF(x, y, coreSize, coreSize);
        }
        else {
            // 右側
            x += coreSize;
            rect = Gdiplus::RectF(x, y, coreSize, coreSize);
        }
        float percent = cpuUsage.usages[i + 1];
//      str.Format(L"Core%d (%.0f%%)", i + 1, percent);
        str.Format(L"Core%d", i + 1);
        DrawMeter(g, rect, percent, str, cpuColors, cpuGuides, scale);
    }
    y += coreSize;
    if (y + size >= screenHeight) {
        return;
    }


    //--------------------------------------------------
    // Network タコメーター描画
    //--------------------------------------------------
    DWORD MB = 1000 * 1000;
    DWORD maxTrafficBytes = 300 * MB;
    float percent = 0.0f;

    const Traffic& t = pWorker->traffics.back();    // 一番新しいもの
    const Traffic& t0 = pWorker->traffics.front();  // 一番古いもの

    DWORD duration = t.tick - t0.tick;

    // duration が ms なので *1000 してから除算
    float inb = (t.in - t0.in) * 1000.0f / duration;
    float outb = (t.out - t0.out) * 1000.0f / duration;

    // KB単位
    maxTrafficBytes /= 1000;
    inb /= 1000;
    outb /= 1000;

    static MeterColor netColors[] = {
        { KbToPercent(1000, maxTrafficBytes), Color(255,  64,  64) },
        { KbToPercent( 100, maxTrafficBytes), Color(255, 128,  64) },
        { KbToPercent(  10, maxTrafficBytes), Color(192, 192,  64) },
        {                                0.0, Color(192, 192, 192) }
    };
    static MeterGuide netGuides[] = {
        { KbToPercent(100000, maxTrafficBytes), Color(255,  64,  64), L"100M" },
        { KbToPercent( 10000, maxTrafficBytes), Color(255,  64,  64), L"10M"  },
        { KbToPercent(  1000, maxTrafficBytes), Color(255,  64,  64), L"1M"   },
        { KbToPercent(   100, maxTrafficBytes), Color(255, 128,  64), L"100K" },
        { KbToPercent(    10, maxTrafficBytes), Color(192, 192,  64), L"10K"  },
        {                                  0.0, Color(192, 192, 192), L""     },
    };

    // Up(KB単位)
    rect = Gdiplus::RectF(0, y, size, size);
    percent = outb == 0 ? 0.0f : KbToPercent(outb, maxTrafficBytes);
    percent = percent < 0.0f ? 0.0f : percent;
    str.Format(L"▲ %.1f KB/s", outb);
    DrawMeter(g, rect, percent, str, netColors, netGuides, 1);

    // Down(KB単位)
    rect = Gdiplus::RectF(size, y, size, size);
    percent = inb == 0 ? 0.0f : KbToPercent(inb, maxTrafficBytes);
    percent = percent < 0.0f ? 0.0f : percent;
    str.Format(L"▼ %.1f KB/s", inb);
    DrawMeter(g, rect, percent, str, netColors, netGuides, 1);
    y += height * 1;


    //--------------------------------------------------
    // デバッグ表示
    //--------------------------------------------------
    static int iCalled = 0;
    iCalled++;

    if (g_pMyInifile->mDebugMode) {
        Font fontTahoma(L"Tahoma", 12);
        StringFormat format;
        format.SetAlignment(StringAlignmentNear);

        // 開始 Y 座標
        rect = Gdiplus::RectF(0, y, screenWidth, screenHeight-y);

        //str.Format(L"Up=%lld(%lld), Down=%lld(%lld)",
        //  t.out, t.out - t0.out,
        //  t.in, t.in - t0.in);
        //g.DrawString(str, str.GetLength(), &fontTahoma, rect, &format, &mainBrush);

        //str.Format(L"Up=%.0f[b/s], Down=%.0f[b/s], %ldms", outb, inb, duration);
        //rect.Offset(0, 30);
        //g.DrawString(str, str.GetLength(), &fontTahoma, rect, &format, &mainBrush);

        //str.Format(L"Up=%.1f[kb/s], Down=%.1f[kb/s]", outb / 1024, inb / 1024);
        //rect.Offset(0, 30);
        //g.DrawString(str, str.GetLength(), &fontTahoma, rect, &format, &mainBrush);

        SolidBrush mainBrush(Color(255, 192, 192, 192));

        CString strDateTime;
        SYSTEMTIME st;
        GetLocalTime(&st);
        strDateTime.Format(L"%d/%d/%d %d:%d:%d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);


        str.Format(L"i=%d, n=%d size=%dx%d %s %.0f", iCalled, pWorker->traffics.size(), rectWindow.right, rectWindow.bottom, (LPCTSTR)strDateTime, size);
        g.DrawString(str, str.GetLength(), &fontTahoma, rect, &format, &mainBrush);
    }
}

inline float KbToPercent(float outb, const DWORD &maxTrafficBytes)
{
    return (log10f((float)outb) / log10f((float)maxTrafficBytes))*100.0f;
}

/**
 * メーターを描画する
 *
 * colors, guideLines の最後は必ず percent=0.0 にすること
 */
void DrawMeter(Graphics& g, Gdiplus::RectF& rect, float percent, const WCHAR* str, MeterColor colors[], MeterGuide guideLines[], float scale)
{
    Color color;
    for (int i = 0; ; i++) {
        if (percent >= colors[i].percent) {
            color = Color(colors[i].color);
            break;
        }

        if (colors[i].percent == 0.0f) {
            break;
        }
    }

    //--------------------------------------------------
    // 枠線
    //--------------------------------------------------
    if (g_pMyInifile->mDrawBorder) {

        Pen pen1(Color(64, 64, 64), 1);
        g.DrawRectangle(&pen1, rect);
    }

    float margin = 5 * (float)scale;
    rect.Offset(margin, margin);
    rect.Width -= margin*2;
    rect.Height -= margin*2;

    // ペン
    Pen p(color, 1.2f);
    SolidBrush mainBrush(color);

    //--------------------------------------------------
    // ラベル
    //--------------------------------------------------
    Font fontTahoma(L"Tahoma", scale == 1.0f ? 11.0f : 9.0f);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    g.DrawString(str, (int)wcslen(str), &fontTahoma, rect, &format, &mainBrush);
    rect.Offset(0, 35 * (float)scale);

    //--------------------------------------------------
    // メーター描画
    //--------------------------------------------------

    // アンチエイリアス
//  g.SetSmoothingMode(SmoothingModeNone);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    // 左下、右下の角度
    float PMIN = -30;
    float PMAX = 180 - PMIN;

    // 外枠
    Gdiplus::PointF center(rect.X + rect.Width / 2, rect.Y + rect.Height / 2);
    g.DrawArc(&p, rect, -180+PMIN, PMAX-PMIN);

    float length0 = rect.Width / 2;

    // 真ん中から左下へ。
    DrawLineByAngle(g, &p, center, PMIN, 0, length0);

    // 真ん中から右下へ。
    DrawLineByAngle(g, &p, center, PMAX, 0, length0);

    // 凡例の線
    p.SetWidth(1.9f * scale);
    Font font(L"Tahoma", 6.5f);
    StringFormat format1;
    format1.SetAlignment(StringAlignmentCenter);
    format1.SetLineAlignment(StringAlignmentCenter);
    for (int i = 0; guideLines[i].percent != 0.0f; i++) {
        p.SetColor(guideLines[i].color);

        float angle = guideLines[i].percent / 100.0f * (PMAX - PMIN) + PMIN;
        DrawLineByAngle(g, &p, center, angle, length0 * 0.85f, length0);

        LPCWSTR text = guideLines[i].text;
        if (wcslen(text) >= 1) {
            float rad = PI * angle / 180;
            float w = length0 / 3;
            float h = length0 / 5;
            float length = length0 * 0.72f;
            Gdiplus::RectF rect1(center.X - length * cosf(rad), center.Y - length * sinf(rad), w, h);
            rect1.Offset(-w / 2, -h / 2);
            SolidBrush fontBrush(guideLines[i].color);
            g.DrawString(text, (int)wcslen(text), &font, rect1, &format1, &fontBrush);
//            g.DrawRectangle(&p, rect1);
        }
    }


    // 針を描く
    p.SetColor(color);
    p.SetWidth(5 * scale);
    DrawLineByAngle(g, &p, center, percent / 100.0f * (PMAX - PMIN) + PMIN, 0, length0 * 0.9f);
}

inline void DrawLineByAngle(Graphics& g, Pen* p, Gdiplus::PointF& center, float angle, float length1, float length2)
{
    float rad = PI * angle / 180;
    float c1 = cosf(rad);
    float s1 = sinf(rad);
    g.DrawLine(p,
        center.X - (length1 == 0 ? 0 : length1 * c1), center.Y - (length1 == 0 ? 0 : length1 * s1),
        center.X - length2 * c1,                      center.Y - length2 * s1
    );
}

