// CompactMeter.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "AboutDlg.h"
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

HINSTANCE g_hInst;                                // 現在のインスタンス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

Bitmap* g_offScreenBitmap = NULL;
Graphics* g_offScreen = NULL;

MyInifileUtil* g_pMyInifile = NULL;

// ドラッグ中
boolean g_dragging = false;


//--------------------------------------------------
// プロトタイプ宣言
//--------------------------------------------------
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                ToggleBorder();
void                ToggleDebugMode();
void                ToggleAlwaysOnTop(const HWND &hWnd);
void                DrawAll(HWND hWnd, HDC hdc, PAINTSTRUCT ps, CWorker* pWorker, Graphics* offScreen, Bitmap* offScreenBitmap);
void                DrawMeters(Graphics& g, HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight);
void                DrawMeter(Graphics& g, Gdiplus::RectF& rect, float percent, const WCHAR* str, Color color, int scale);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // グローバル文字列を初期化
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
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

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
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
        }
        break;

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

        PostQuitMessage(0);
        break;

    case WM_KEYUP:
        switch (wParam) {
        case VK_ESCAPE:
        case VK_F12:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            return 0L;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
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

    case WM_LBUTTONDOWN:
        ReleaseCapture();

        g_dragging = true;
        if (GetKeyState(VK_SHIFT) & 0x8000) {
            // Shift+ドラッグでリサイズ(暫定)
            Logger::d(L"resize start");
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, 0);
            Logger::d(L"resize end");
        }
        else {
            // ドラッグで移動
            Logger::d(L"drag start");
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            Logger::d(L"drag end");
        }
        g_dragging = false;
        return 0;

    case WM_RBUTTONUP:
        {
            // 右クリックメニュー表示
            POINT po;
            po.x = LOWORD(lParam);
            po.y = HIWORD(lParam);
        
            static HMENU hMenu;
            static HMENU hSubMenu;

            hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_POPUP_MENU));
            hSubMenu = GetSubMenu(hMenu, 0);

            ClientToScreen(hWnd, &po);

            // 初期チェック状態を反映
            CheckMenuItem(hSubMenu, ID_POPUPMENU_ALWAYSONTOP, MF_BYCOMMAND | (g_pMyInifile->mAlwaysOnTop ? MFS_CHECKED : MFS_UNCHECKED));
            CheckMenuItem(hSubMenu, ID_POPUPMENU_DEBUGMODE, MF_BYCOMMAND | (g_pMyInifile->mDebugMode ? MFS_CHECKED : MFS_UNCHECKED));
            CheckMenuItem(hSubMenu, ID_POPUPMENU_DRAW_BORDER, MF_BYCOMMAND | (g_pMyInifile->mDrawBorder ? MFS_CHECKED : MFS_UNCHECKED));

            Logger::d(L"Show Popup menu");
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, po.x, po.y, 0, hWnd, NULL);
            Logger::d(L"Close Popup menu");

            return 0;
        }

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
            // 移動 => INIにイチを保存
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


    const Traffic& t = pWorker->traffics[pWorker->traffics.size() - 1]; // 一番新しいもの
    const Traffic& t0 = pWorker->traffics[0];   // 一番古いもの

    DWORD duration = t.tick - t0.tick;

    // duration が ms なので *1000 してから除算
    float inb = (t.in - t0.in) * 1000.0f / duration;
    float outb = (t.out - t0.out) * 1000.0f / duration;


    //--------------------------------------------------
    // CPU+Memory タコメーター描画
    //--------------------------------------------------
    Color colorCpu(255, 192, 192, 192);

    CpuUsage cpuUsage;
    int nCore = pWorker->GetCpuUsage(&cpuUsage);

    float y = 0;
    float height = size;

    // 全コアの合計
    {
        rect = Gdiplus::RectF(0, 0, size, size);
        float percent = cpuUsage.usages[0];
        str.Format(L"CPU (%.0f%%)", percent);
        DrawMeter(g, rect, percent, str, colorCpu, 1);
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
        DrawMeter(g, rect, percent, str, colorCpu, 1);
    }

    //--------------------------------------------------
    // 各Core
    //--------------------------------------------------
    int div = 4;
    int scale = div / 2;    // 1 or 2
    float coreSize = size / scale;
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
        DrawMeter(g, rect, percent, str, colorCpu, scale);
    }
    y += coreSize;
    if (y + coreSize >= screenHeight) {
        return;
    }


    //--------------------------------------------------
    // Network タコメーター描画
    //--------------------------------------------------
    Color colorNet(255, 96, 192, 96);

    DWORD MB = 1024 * 1024;
    DWORD maxTrafficBytes = 100 * MB;
    float percent = 0.0f;

    // Up(byte単位)
    //rect = Gdiplus::RectF(0, 0, 200.0f, 200.0f);
    //percent = outb == 0 ? 0.0f : (log10f((float)outb) / log10f((float)maxTrafficBytes))*100.0f;
    //str.Format(L"▲ %.0f[b/s], %.0f%%", outb, percent);
    //DrawMeter(g, rect, percent, str, colorNet);

    // Down(byte単位)
    //rect = Gdiplus::RectF(0 + 250.0f, 0, 200.0f, 200.0f);
    //percent = inb == 0 ? 0.0f : (log10f((float)inb) / log10f((float)maxTrafficBytes))*100.0f;
    //str.Format(L"▼ %.0f[b/s], %.0f%%", inb, percent);
    //DrawMeter(g, rect, percent, str, colorNet);

    // kB単位
    maxTrafficBytes /= 1024;
    inb /= 1024;
    outb /= 1024;

    // Up(kB単位)
    rect = Gdiplus::RectF(0, y, size, size);
    percent = outb == 0 ? 0.0f : (log10f((float)outb) / log10f((float)maxTrafficBytes))*100.0f;
    percent = percent < 0.0f ? 0.0f : percent;
    str.Format(L"▲ %.1f KB/s", outb);
    DrawMeter(g, rect, percent, str, colorNet, 1);

    // Down(kB単位)
    rect = Gdiplus::RectF(size, y, size, size);
    percent = inb == 0 ? 0.0f : (log10f((float)inb) / log10f((float)maxTrafficBytes))*100.0f;
    percent = percent < 0.0f ? 0.0f : percent;
    str.Format(L"▼ %.1f KB/s", inb);
    DrawMeter(g, rect, percent, str, colorNet, 1);
    y += height * 1;

    //--------------------------------------------------
    // デバッグ表示
    //--------------------------------------------------
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


    static int iCalled = 0;
    iCalled++;

    if (g_pMyInifile->mDebugMode) {
        str.Format(L"i=%d, n=%d size=%dx%d %s %.0f", iCalled, pWorker->traffics.size(), rectWindow.right, rectWindow.bottom, (LPCTSTR)strDateTime, size);
        g.DrawString(str, str.GetLength(), &fontTahoma, rect, &format, &mainBrush);
    }

}

void DrawMeter(Graphics& g, Gdiplus::RectF& rect, float percent, const WCHAR* str, Color color, int scale)
{
    if (percent >= 90.0) {
        color = Color(255, 64, 64);
    } else if (percent >= 80.0) {
        color = Color(255, 128, 64);
    } else if (percent >= 70.0) {
        color = Color(192, 192, 64);
    }

    //--------------------------------------------------
    // 枠線(デバッグのみ)
    //--------------------------------------------------
    if (g_pMyInifile->mDrawBorder) {

        Pen pen1(Color(64, 64, 64), 1);
        g.DrawRectangle(&pen1, rect);
    }

    float margin = 5 / (float)scale;
    rect.Offset(margin, margin);
    rect.Width -= margin*2;
    rect.Height -= margin*2;

    // ペン
    Pen p(color, 1);
    SolidBrush mainBrush(color);

    //--------------------------------------------------
    // ラベル
    //--------------------------------------------------
    Font fontTahoma(L"Tahoma", scale == 1 ? 11.0f : 9.0f);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    g.DrawString(str, (int)wcslen(str), &fontTahoma, rect, &format, &mainBrush);
    rect.Offset(0, 35 / (float)scale);

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
//  g.DrawLine(&p, rect.X, center.Y, rect.GetRight(), center.Y);

    float length0 = rect.Width / 2;

    // 真ん中から左下へ。
    float r1 = PI * PMIN / 180;
    g.DrawLine(&p, center.X, center.Y, center.X - length0 * cosf(r1), center.Y - length0 * sinf(r1));

    // 真ん中から右下へ。
    float r2 = PI * PMAX / 180;
    g.DrawLine(&p, center.X, center.Y, center.X - length0 * cosf(r2), center.Y - length0 * sinf(r2));

    p.SetWidth(5);

    // 線を引く
    float length = rect.Width / 2 * 0.9f;
//  float rad = PI * percent / 100.0f;
    float rad = PI * (percent / 100.0f * (PMAX - PMIN) + PMIN) / 180;
    float x = center.X - length * cosf(rad);
    float y = center.Y - length * sinf(rad);
    g.DrawLine(&p, center.X, center.Y, x, y);
}