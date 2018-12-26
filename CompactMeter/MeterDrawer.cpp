#include "stdafx.h"
#include "MeterDrawer.h"
#include "Worker.h"
#include "IniConfig.h"
#include "Logger.h"

extern int g_dpix;
extern int g_dpiy;
extern float g_dpiScale;
extern IniConfig* g_pIniConfig;

void MeterDrawer::Init(HWND hWnd, int width, int height) {

    //--------------------------------------------------
    // Direct2D
    //--------------------------------------------------
    HRESULT hr = CreateDeviceIndependentResources();
    if (FAILED(hr)) return;

    //--------------------------------------------------
    // GDI+
    //--------------------------------------------------
    GdiplusStartupInput gdiSI;

    GdiplusStartup(&m_gdiToken, &gdiSI, NULL);

    // GDI+ OffScreen 作成, Direct2D 初期化
    Resize(hWnd, width, height);
}

void MeterDrawer::Resize(HWND hWnd, int width, int height) {

    //--------------------------------------------------
    // GDI+
    //--------------------------------------------------
    if (m_pOffScreenBitmap != NULL) {
        delete m_pOffScreenBitmap;
    }
    if (m_pOffScreenGraphics != NULL) {
        delete m_pOffScreenGraphics;
    }
    m_pOffScreenBitmap = new Bitmap(width, height);
    m_pOffScreenGraphics = new Graphics(m_pOffScreenBitmap);


    //--------------------------------------------------
    // Direct2D
    //--------------------------------------------------
    DiscardDeviceResources();
    CreateDeviceResources(hWnd, width, height);

}

void MeterDrawer::Shutdown() {

    GdiplusShutdown(m_gdiToken);
}

void MeterDrawer::DrawToDC(HDC hdc, HWND hWnd, CWorker * pWorker)
{
    HRESULT hr;
    RECT rc;

    GetClientRect(hWnd, &rc);

    // Create the DC render target.
    hr = CreateDeviceResources(hWnd, rc.right, rc.bottom);

    if (SUCCEEDED(hr))
    {
        if (g_pIniConfig->mDirect2DMode) {

            //--------------------------------------------------
            // 描画
            //--------------------------------------------------
            m_stopWatch1.Start();

            m_pRenderTarget->BeginDraw();

            DrawD2D(hWnd, pWorker);

            m_pRenderTarget->EndDraw();

            m_stopWatch1.Stop();

        } else {

            //--------------------------------------------------
            // 描画
            //--------------------------------------------------
            m_stopWatch1.Start();

            Draw(*m_pOffScreenGraphics, hWnd, pWorker);

            m_stopWatch1.Stop();

            //--------------------------------------------------
            // 実画面に転送
            //--------------------------------------------------
            m_stopWatch2.Start();

            Graphics onScreen(hdc);
            onScreen.DrawImage(m_pOffScreenBitmap, 0, 0);

            m_stopWatch2.Stop();
        }

    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;

        Logger::d(L"D2DERR_RECREATE_TARGET");

        DiscardDeviceResources();
    }


}

HRESULT MeterDrawer::CreateDeviceIndependentResources()
{
    HRESULT hr;

    // Direct2D factory
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) {
        Logger::d(L"cannot init D2D1Factory");
        return hr;
    }

    // DirectWrite factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWFactory));
    if (FAILED(hr)) {
        Logger::d(L"cannot init DWriteFactory");
        return hr;
    }

    // TODO リサイズ時に作り直すこと
    // TextFormat
    hr = m_pDWFactory->CreateTextFormat(
        L"ＭＳゴシック"
        , NULL
        , DWRITE_FONT_WEIGHT_NORMAL
        , DWRITE_FONT_STYLE_NORMAL
        , DWRITE_FONT_STRETCH_NORMAL
        , 12
        , L""
        , &m_pTextFormat
    );
    if (FAILED(hr)) {
        Logger::d(L"cannot init TextFormat");
        return hr;
    }

    // TextFormat
    hr = m_pDWFactory->CreateTextFormat(
        L"ＭＳゴシック"
        , NULL
        , DWRITE_FONT_WEIGHT_NORMAL
        , DWRITE_FONT_STYLE_NORMAL
        , DWRITE_FONT_STRETCH_NORMAL
        , 10
        , L""
        , &m_pTextFormat2
    );
    if (FAILED(hr)) {
        Logger::d(L"cannot init TextFormat");
        return hr;
    }

    // TextFormat
    hr = m_pDWFactory->CreateTextFormat(
        L"ＭＳゴシック"
        , NULL
        , DWRITE_FONT_WEIGHT_NORMAL
        , DWRITE_FONT_STYLE_NORMAL
        , DWRITE_FONT_STRETCH_NORMAL
        , 8
        , L""
        , &m_pTextFormat3
    );
    if (FAILED(hr)) {
        Logger::d(L"cannot init TextFormat");
        return hr;
    }

    // PathGeometry
    // (0, 0) を原点に 100 のサイズで描画
    {
        HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pPathGeometry);

        if (SUCCEEDED(hr))
        {
            ID2D1GeometrySink *pSink = NULL;

            // Write to the path geometry using the geometry sink.
            hr = m_pPathGeometry->Open(&pSink);

            if (SUCCEEDED(hr))
            {
                pSink->BeginFigure(
                    D2D1::Point2F(0, 0),
                    D2D1_FIGURE_BEGIN_FILLED
                );

                // 左下、右下の角度
                float length0 = 100.0;

                float x1 = -length0 * cosf(PI * PMIN / 180);
                float y1 = -length0 * sinf(PI * PMIN / 180);
                float x2 = -length0 * cosf(PI * PMAX / 180);
                float y2 = -length0 * sinf(PI * PMAX / 180);


                pSink->AddLine(D2D1::Point2F(x1, y1));

                pSink->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(-length0, 0), // end point
                        D2D1::SizeF(length0, length0),
                        0, // rotation angle
                        D2D1_SWEEP_DIRECTION_CLOCKWISE,
                        D2D1_ARC_SIZE_SMALL
                    ));

                pSink->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(0, -length0), // end point
                        D2D1::SizeF(length0, length0),
                        0, // rotation angle
                        D2D1_SWEEP_DIRECTION_CLOCKWISE,
                        D2D1_ARC_SIZE_SMALL
                    ));

                pSink->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(length0, 0), // end point
                        D2D1::SizeF(length0, length0),
                        0, // rotation angle
                        D2D1_SWEEP_DIRECTION_CLOCKWISE,
                        D2D1_ARC_SIZE_SMALL
                    ));

                pSink->AddArc(
                    D2D1::ArcSegment(
                        D2D1::Point2F(x2, y2), // end point
                        D2D1::SizeF(length0, length0),
                        0, // rotation angle
                        D2D1_SWEEP_DIRECTION_CLOCKWISE,
                        D2D1_ARC_SIZE_SMALL
                    ));

                pSink->AddLine(D2D1::Point2F(0, 0));

                pSink->EndFigure(D2D1_FIGURE_END_OPEN);

                hr = pSink->Close();
            }
            SafeRelease(&pSink);
        }
    }

    return hr;
}

HRESULT MeterDrawer::CreateDeviceResources(HWND hWnd, int width, int height)
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
    {
        FLOAT dpiX, dpiY;
        m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
        Logger::d(L"DPI: %.2f,%.2f", dpiX, dpiY);

        // RenderTarget
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU(width, height)),
            &m_pRenderTarget
        );
        if (FAILED(hr)) {
            Logger::d(L"cannot init HwndRenderTarget");
            return hr;
        }

        // アンチエイリアス
        m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);


        // Brush
        hr = m_pRenderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            &m_pBrush
        );
        if (FAILED(hr)) {
            Logger::d(L"cannot init SolidColorBrush");
            return hr;
        }
    }

    return hr;
}

void MeterDrawer::Draw(Graphics& g, HWND hWnd, CWorker* pWorker)
{
    //--------------------------------------------------
    // Draw
    //--------------------------------------------------
    SolidBrush backgroundBrush(Color(255, 10, 10, 10));
    float screenWidth = (float)g_pIniConfig->mWindowWidth;
    float screenHeight = (float)g_pIniConfig->mWindowHeight;
    Gdiplus::RectF rect = Gdiplus::RectF(0.0f, 0.0f, screenWidth, screenHeight);

    g.FillRectangle(&backgroundBrush, rect);

    if (pWorker != NULL && pWorker->traffics.size() >= 2) {

        pWorker->criticalSection.Lock();

        DrawMeters(g, hWnd, pWorker, screenWidth, screenHeight);

        pWorker->criticalSection.Unlock();
    }
}

void MeterDrawer::DrawMeters(Graphics& g, HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight)
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
        str.Format(L"Memory (%.0f%%)\n%I64d / %I64d MB", percent, ullUsing / 1024 / 1024, ms.ullTotalPhys / 1024 / 1024);
        DrawMeter(g, rect, percent, str, cpuColors, cpuGuides, 1);
    }
    y += height;

    //--------------------------------------------------
    // 各Core
    //--------------------------------------------------
    if (g_pIniConfig->mShowCoreMeters) {
        int div = 4;
        float scale = 2.0f / div;    // 1.0 or 0.5
        float coreSize = size * scale;
        float x = 0;
        for (int i = 0; i < nCore; i++) {
            if (i % div == 0) {
                // 左側
                x = 0;
                if (i != 0) {
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
            //          str.Format(L"Core%d (%.0f%%)", i + 1, percent);
            str.Format(L"Core%d", i + 1);
            DrawMeter(g, rect, percent, str, cpuColors, cpuGuides, 1.4f);
        }
        y += coreSize;
    }
    if (y + size >= screenHeight) {
        return;
    }


    //--------------------------------------------------
    // Network タコメーター描画
    //--------------------------------------------------
    DWORD maxTrafficBytes = g_pIniConfig->mTrafficMax;
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

    MeterColor netColors[] = {
        { KbToPercent(1000, maxTrafficBytes), Color(255,  64,  64) },
        { KbToPercent( 100, maxTrafficBytes), Color(255, 128,  64) },
        { KbToPercent(  10, maxTrafficBytes), Color(192, 192,  64) },
        {                                0.0, Color(192, 192, 192) }
    };
    MeterGuide netGuides[] = {
        { KbToPercent(1000000, maxTrafficBytes), Color(255,  64,  64), L"1G"   },
        { KbToPercent( 100000, maxTrafficBytes), Color(255,  64,  64), L"100M" },
        { KbToPercent(  10000, maxTrafficBytes), Color(255,  64,  64), L"10M"  },
        { KbToPercent(   1000, maxTrafficBytes), Color(255,  64,  64), L"1M"   },
        { KbToPercent(    100, maxTrafficBytes), Color(255, 128,  64), L"100K" },
        { KbToPercent(     10, maxTrafficBytes), Color(192, 192,  64), L"10K"  },
        {                                   0.0, Color(192, 192, 192), L""     },
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

    // FPS カウント
    m_fpsCounter.CountOnDraw();

    if (g_pIniConfig->mDebugMode) {
        Font fontTahoma(L"ＭＳゴシック", 19 / g_dpiScale * size / 300.0f);
        StringFormat format;
        format.SetAlignment(StringAlignmentNear);

        // 開始 Y 座標
        rect = Gdiplus::RectF(0, y, screenWidth, screenHeight - y);

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
        strDateTime.Format(L"%d/%02d/%02d %d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        // デバッグ用計測器
        DWORD duration1 = m_stopWatch1.GetAverageDurationMicroseconds();
        DWORD duration2 = m_stopWatch2.GetAverageDurationMicroseconds();
        DWORD durationWorker = pWorker->m_stopWatch.GetAverageDurationMicroseconds();

        str.Format(L"i=%d, FPS=%d/%d, "
                   L"n=%d size=%dx%d \n"
                   L"%s %.0f DPI=%d,%d(%.2f)\n"
                   L"描画=%5.1fms\n転送=%5.1fms\n計測=%5.1fms\n"
                   L"RenderMode=%s\n"
            ,
            iCalled, m_fpsCounter.GetAverageFps(), g_pIniConfig->mFps,
            pWorker->traffics.size(), rectWindow.right, rectWindow.bottom,
            (LPCTSTR)strDateTime, size,
            g_dpix, g_dpiy, g_dpiScale,
            duration1/1000.0, duration2/1000.0, durationWorker/1000.0,
            g_pIniConfig->mDirect2DMode ? L"Direct2D" : L"GDI"
        );
        g.DrawString(str, str.GetLength(), &fontTahoma, rect, &format, &mainBrush);
    }
}

/**
 * メーターを描画する
 *
 * colors, guideLines の最後は必ず percent=0.0 にすること
 */
void MeterDrawer::DrawMeter(Graphics& g, Gdiplus::RectF& rect, float percent, const WCHAR* str, MeterColor colors[], MeterGuide guideLines[], float fontScale)
{
    auto size = rect.Width;

    if (percent < 0.0f) {
        percent = 0.0f;
    }
    else if (percent > 100.0f) {
        percent = 100.0f;
    }

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
    if (g_pIniConfig->mDrawBorder) {

        Pen pen1(Color(64, 64, 64), 1);
        g.DrawRectangle(&pen1, rect);
    }

    float margin = size / 50;
    rect.Offset(margin, margin);
    rect.Width -= margin * 2;
    rect.Height -= margin * 2;

    // ペン
    Pen p(color, 1.2f);
    SolidBrush mainBrush(color);

    //--------------------------------------------------
    // ラベル
    //--------------------------------------------------
    float scale = 1 / g_dpiScale * size / 300.0f * fontScale;
    Font fontTahoma(L"Tahoma", 19.0f * scale);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    g.DrawString(str, (int)wcslen(str), &fontTahoma, rect, &format, &mainBrush);
    rect.Offset(0, size / 5.0f);

    //--------------------------------------------------
    // メーター描画
    //--------------------------------------------------

    // アンチエイリアス
//  g.SetSmoothingMode(SmoothingModeNone);
    g.SetSmoothingMode(SmoothingModeHighQuality);

    // 外枠
    Gdiplus::PointF center(rect.X + rect.Width / 2, rect.Y + rect.Height / 2);
    g.DrawArc(&p, rect, -180 + PMIN, PMAX - PMIN);

    float length0 = rect.Width / 2;

    // 真ん中から左下へ。
    DrawLineByAngle(g, &p, center, PMIN, 0, length0);

    // 真ん中から右下へ。
    DrawLineByAngle(g, &p, center, PMAX, 0, length0);

    // 凡例の線
    p.SetWidth(1.9f * scale);
    Font font(L"Tahoma", 11.5f * scale);
    StringFormat format1;
    format1.SetAlignment(StringAlignmentCenter);
    format1.SetLineAlignment(StringAlignmentCenter);
    for (int i = 0; guideLines[i].percent != 0.0f; i++) {

        if (guideLines[i].percent > 100) {
            continue;
        }

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

// TODO 最終的には Gdiplus::RectF を利用しないようにすること
inline D2D1_RECT_F ToD2D1Rect(const Gdiplus::RectF& rect) {

    D2D1_RECT_F rect2;
    rect2.left = rect.X;
    rect2.top = rect.Y;
    rect2.right = rect.X + rect.Width;
    rect2.bottom = rect.Y + rect.Height;
    return rect2;
}

void MeterDrawer::DrawD2D(HWND hWnd, CWorker* pWorker)
{
    //--------------------------------------------------
    // Draw
    //--------------------------------------------------
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_pRenderTarget->Clear(D2D1::ColorF(0x0A0A0A));

    float screenWidth = (float)g_pIniConfig->mWindowWidth;
    float screenHeight = (float)g_pIniConfig->mWindowHeight;

    if (pWorker != NULL && pWorker->traffics.size() >= 2) {

        pWorker->criticalSection.Lock();

        DrawMetersD2D(hWnd, pWorker, screenWidth, screenHeight);

        pWorker->criticalSection.Unlock();
    }
}

void MeterDrawer::DrawMetersD2D(HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight)
{
    Gdiplus::RectF rect;

    CString str;
    // 各ウィジェットのサイズ(width, height)
    float size = screenWidth / 2.0f;

    // 試しにDPIを設定してみる
//    m_pRenderTarget->SetDpi(96.0f, 96.0f);


    //--------------------------------------------------
    // CPU+Memory タコメーター描画
    //--------------------------------------------------
    static MeterColorD2D cpuColors[] = {
        { 90.0, D2D1::ColorF(0xFF4040) },
        { 80.0, D2D1::ColorF(0xFF8040) },
        { 70.0, D2D1::ColorF(0xC0C040) },
        {  0.0, D2D1::ColorF(0xC0C0C0) }
    };
    static MeterGuideD2D cpuGuides[] = {
        { 100.0, D2D1::ColorF(0xFF4040), L"" },
        {  90.0, D2D1::ColorF(0xFF4040), L"" },
        {  80.0, D2D1::ColorF(0xFF4040), L"" },
        {  70.0, D2D1::ColorF(0xFF4040), L"" },
        {  60.0, D2D1::ColorF(0xC0C0C0), L"" },
        {  50.0, D2D1::ColorF(0xC0C0C0), L"" },
        {  40.0, D2D1::ColorF(0xC0C0C0), L"" },
        {  30.0, D2D1::ColorF(0xC0C0C0), L"" },
        {  20.0, D2D1::ColorF(0xC0C0C0), L"" },
        {  10.0, D2D1::ColorF(0xC0C0C0), L"" },
        {   0.0, D2D1::ColorF(0xC0C0C0), L"" },
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
        DrawMeterD2D(rect, percent, str, cpuColors, cpuGuides, 1);
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
        str.Format(L"Memory (%.0f%%)\n%I64d / %I64d MB", percent, ullUsing / 1024 / 1024, ms.ullTotalPhys / 1024 / 1024);
        DrawMeterD2D(rect, percent, str, cpuColors, cpuGuides, 1);
    }
    y += height;

    //--------------------------------------------------
    // 各Core
    //--------------------------------------------------
    if (g_pIniConfig->mShowCoreMeters) {
        int div = 4;
        float scale = 2.0f / div;    // 1.0 or 0.5
        float coreSize = size * scale;
        float x = 0;
        for (int i = 0; i < nCore; i++) {
            if (i % div == 0) {
                // 左側
                x = 0;
                if (i != 0) {
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
            //          str.Format(L"Core%d (%.0f%%)", i + 1, percent);
            str.Format(L"Core%d", i + 1);
            DrawMeterD2D(rect, percent, str, cpuColors, cpuGuides, 1.4f);
        }
        y += coreSize;
    }
    if (y + size >= screenHeight) {
        return;
    }


    //--------------------------------------------------
    // Network タコメーター描画
    //--------------------------------------------------
    DWORD maxTrafficBytes = g_pIniConfig->mTrafficMax;
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

    MeterColorD2D netColors[] = {
        { KbToPercent(1000, maxTrafficBytes), D2D1::ColorF(0xFF4040) },
        { KbToPercent( 100, maxTrafficBytes), D2D1::ColorF(0XFF8040) },
        { KbToPercent(  10, maxTrafficBytes), D2D1::ColorF(0xC0C040) },
        {                                0.0, D2D1::ColorF(0xC0C0C0) }
    };
    MeterGuideD2D netGuides[] = {
        { KbToPercent(1000000, maxTrafficBytes), D2D1::ColorF(0xFF4040), L"1G"   },
        { KbToPercent( 100000, maxTrafficBytes), D2D1::ColorF(0xFF4040), L"100M" },
        { KbToPercent(  10000, maxTrafficBytes), D2D1::ColorF(0xFF4040), L"10M"  },
        { KbToPercent(   1000, maxTrafficBytes), D2D1::ColorF(0xFF4040), L"1M"   },
        { KbToPercent(    100, maxTrafficBytes), D2D1::ColorF(0xFF8040), L"100K" },
        { KbToPercent(     10, maxTrafficBytes), D2D1::ColorF(0xC0C040), L"10K"  },
        {                                   0.0, D2D1::ColorF(0xC0C0C0), L""     },
    };

    // Up(KB単位)
    rect = Gdiplus::RectF(0, y, size, size);
    percent = outb == 0 ? 0.0f : KbToPercent(outb, maxTrafficBytes);
    percent = percent < 0.0f ? 0.0f : percent;
    str.Format(L"▲ %.1f KB/s", outb);
    DrawMeterD2D(rect, percent, str, netColors, netGuides, 1);

    // Down(KB単位)
    rect = Gdiplus::RectF(size, y, size, size);
    percent = inb == 0 ? 0.0f : KbToPercent(inb, maxTrafficBytes);
    percent = percent < 0.0f ? 0.0f : percent;
    str.Format(L"▼ %.1f KB/s", inb);
    DrawMeterD2D(rect, percent, str, netColors, netGuides, 1);
    y += height * 1;


    //--------------------------------------------------
    // デバッグ表示
    //--------------------------------------------------
    static int iCalled = 0;
    iCalled++;

    // FPS カウント
    m_fpsCounter.CountOnDraw();

    if (g_pIniConfig->mDebugMode) {

        CString strDateTime;
        SYSTEMTIME st;
        GetLocalTime(&st);
        strDateTime.Format(L"%d/%02d/%02d %d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        // デバッグ用計測器
        DWORD duration1 = m_stopWatch1.GetAverageDurationMicroseconds();
        DWORD durationWorker = pWorker->m_stopWatch.GetAverageDurationMicroseconds();

        str.Format(L"i=%d, FPS=%d/%d, "
            L"n=%d size=%.0fx%.0f \n"
            L"%s \n"
            L"box=%.0f DPI=%d,%d(%.2f)\n"
            L"描画=%5.1fms\n計測=%5.1fms\n"
            L"RenderMode=%s\n"
            , iCalled, m_fpsCounter.GetAverageFps(), g_pIniConfig->mFps
            , pWorker->traffics.size(), screenWidth, screenHeight
            , (LPCTSTR)strDateTime
            , size, g_dpix, g_dpiy, g_dpiScale
            , duration1 / 1000.0, durationWorker / 1000.0
            , g_pIniConfig->mDirect2DMode ? L"Direct2D" : L"GDI"
        );

        m_pBrush->SetColor(D2D1::ColorF(0xC0C0C0));
        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->DrawText(str, str.GetLength(), m_pTextFormat,
            &D2D1::RectF(0, y, screenWidth, screenHeight), m_pBrush, D2D1_DRAW_TEXT_OPTIONS_NO_SNAP,
            DWRITE_MEASURING_MODE_NATURAL);
    }
}

/**
 * メーターを描画する
 *
 * colors, guideLines の最後は必ず percent=0.0 にすること
 */
void MeterDrawer::DrawMeterD2D(Gdiplus::RectF& rect, float percent, const WCHAR* str, MeterColorD2D colors[], MeterGuideD2D guideLines[], float fontScale)
{
    auto size = rect.Width;

    if (percent < 0.0f) {
        percent = 0.0f;
    }
    else if (percent > 100.0f) {
        percent = 100.0f;
    }

    D2D1::ColorF color(0x000000);
    for (int i = 0; ; i++) {
        if (percent >= colors[i].percent) {
            color = colors[i].color;
            break;
        }

        if (colors[i].percent == 0.0f) {
            break;
        }
    }

    //--------------------------------------------------
    // 枠線
    //--------------------------------------------------
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    if (g_pIniConfig->mDrawBorder) {

        m_pBrush->SetColor(D2D1::ColorF(0x404040));
        
        m_pRenderTarget->DrawRectangle(ToD2D1Rect(rect), m_pBrush);
    }

    float margin = size / 50;
    rect.Offset(margin, margin);
    rect.Width -= margin * 2;
    rect.Height -= margin * 2;

    //--------------------------------------------------
    // ラベル
    //--------------------------------------------------
    float scale = 1 / g_dpiScale * size / 300.0f * fontScale;

    m_pBrush->SetColor(color);
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_pRenderTarget->DrawText(str, wcslen(str), m_pTextFormat2,
        ToD2D1Rect(rect), m_pBrush, D2D1_DRAW_TEXT_OPTIONS_NO_SNAP,
        DWRITE_MEASURING_MODE_NATURAL);


    rect.Offset(0, size / 5.0f);

    //--------------------------------------------------
    // メーター描画
    //--------------------------------------------------

    float length0 = rect.Width / 2;

    // 外枠
    Gdiplus::PointF center(rect.X + rect.Width / 2, rect.Y + rect.Height / 2);

    {
        D2D1::Matrix3x2F matrix1 =
            // サイズ 100 で描画されているので拡縮する
            D2D1::Matrix3x2F::Scale({ length0 / 100.0f, length0 / 100.0f })
            *
            // 中心点に移動する
            D2D1::Matrix3x2F::Translation(center.X, center.Y);

        m_pRenderTarget->SetTransform(matrix1);
        m_pBrush->SetColor(color);
        m_pRenderTarget->DrawGeometry(m_pPathGeometry, m_pBrush, 1);
    }

    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // 凡例の線
    for (int i = 0; guideLines[i].percent != 0.0f; i++) {

        if (guideLines[i].percent > 100) {
            continue;
        }

        float angle = guideLines[i].percent / 100.0f * (PMAX - PMIN) + PMIN;
        m_pBrush->SetColor(guideLines[i].color);
        DrawLineByAngle(center, angle,
            length0 * 0.85f, length0,
            1.5f * scale);

        LPCWSTR text = guideLines[i].text;
        if (wcslen(text) >= 1) {
            float rad = PI * angle / 180;
            float w = length0 / 3;
            float h = length0 / 5;
            float length = length0 * 0.72f;
            Gdiplus::RectF rect1(center.X - length * cosf(rad), center.Y - length * sinf(rad), w, h);
            rect1.Offset(-w / 2, -h / 2);

            m_pBrush->SetColor(guideLines[i].color);
            m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

            IDWriteTextLayout* pTextLayout = NULL;
            if (SUCCEEDED(m_pDWFactory->CreateTextLayout(text, wcslen(text), m_pTextFormat3, rect1.Width, rect1.Height, &pTextLayout))) {

                pTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                m_pRenderTarget->DrawTextLayout(D2D1::Point2F(rect1.X, rect1.Y), pTextLayout, m_pBrush);

                SafeRelease(&pTextLayout);
            }

//            m_pRenderTarget->DrawRectangle(ToD2D1Rect(rect1), m_pBrush);
        }
    }


    // 針を描く
    float strokeWidth = 5 * scale;
    float angle = percent / 100.0f * (PMAX - PMIN) + PMIN;
    m_pBrush->SetColor(color);
    DrawLineByAngle(center, angle, 0, length0 * 0.9f, strokeWidth);
}

void MeterDrawer::DrawLineByAngle(Gdiplus::PointF &center, float angle, float length1, float length2, float strokeWidth)
{
    m_pRenderTarget->SetTransform(
        D2D1::Matrix3x2F::Rotation(angle)
        *
        D2D1::Matrix3x2F::Translation(center.X, center.Y)
    );
    m_pRenderTarget->DrawLine(
        D2D1::Point2F(-length1, 0),
        D2D1::Point2F(-length2, 0),
        m_pBrush,
        strokeWidth);
}
