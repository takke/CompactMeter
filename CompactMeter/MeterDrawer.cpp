﻿#include "stdafx.h"
#include "MeterDrawer.h"
#include "Worker.h"
#include "IniConfig.h"
#include "Logger.h"

extern int g_dpix;
extern int g_dpiy;
extern float g_dpiScale;
extern IniConfig* g_pIniConfig;

void MeterDrawer::Init(HWND hWnd, int width, int height) {

    HRESULT hr = CreateDeviceIndependentResources();
    if (FAILED(hr)) return;

    // Direct2D 初期化
    CreateDeviceResources(hWnd, width, height);
}

void MeterDrawer::Resize(HWND hWnd, int width, int height) {

    m_pRenderTarget->Resize(D2D1::SizeU(width, height));
}

void MeterDrawer::Shutdown() {

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
        //--------------------------------------------------
        // 描画
        //--------------------------------------------------
        m_stopWatch1.Start();

        m_pRenderTarget->BeginDraw();

        Draw(hWnd, pWorker);

        m_pRenderTarget->EndDraw();

        m_stopWatch1.Stop();

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

void MeterDrawer::Draw(HWND hWnd, CWorker* pWorker)
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

        DrawMeters(hWnd, pWorker, screenWidth, screenHeight);

        pWorker->criticalSection.Unlock();
    }
}

void MeterDrawer::DrawMeters(HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight)
{
    //--------------------------------------------------
    // 各メーター情報の収集
    //--------------------------------------------------
    int nCore = 0;

    MeterInfo cpuMeter;
    std::vector<MeterInfo> coreMeters;
    MeterInfo memoryMeter;
    MeterInfo netMeterIn;
    MeterInfo netMeterOut;
    std::vector<MeterInfo> driveMeters;

    // CPU+Memory
    MakeCpuMemoryMeterInfo(nCore, pWorker, cpuMeter, coreMeters, memoryMeter);

    // Drive
    MakeDriveMeterInfo(pWorker, driveMeters);

    // Network
    MakeNetworkMeterInfo(pWorker, netMeterOut, netMeterIn);


    //--------------------------------------------------
    // 描画順序設定に合わせて詰める
    //--------------------------------------------------
    std::vector<MeterInfo*> meters;

    for (auto mc : g_pIniConfig->mMeterConfigs) {
        switch (mc.id) {
        case METER_ID_UNKNOWN:
            break;
        case METER_ID_CPU:
            meters.push_back(&cpuMeter);
            break;
        case METER_ID_CORES:
            for (size_t i = 0; i < coreMeters.size(); i++) {
                coreMeters[i].div = 2;
                meters.push_back(&coreMeters[i]);
            }
            break;
        case METER_ID_MEMORY:
            meters.push_back(&memoryMeter);
            break;
        case METER_ID_NETWORK:
            meters.push_back(&netMeterOut);
            meters.push_back(&netMeterIn);
            break;
        case METER_ID_DRIVES:
            for (size_t i = 0; i < driveMeters.size(); i++) {
                meters.push_back(&driveMeters[i]);
            }
            break;
        }
    }


    //--------------------------------------------------
    // 各メーターの描画
    //--------------------------------------------------

    float y = 0;
    // 各ウィジェットの標準サイズ(width, height)
    float boxSize = screenWidth / 2.0f / g_dpiScale;     // box size

    {
        float x = 0;
        const float width = screenWidth / g_dpiScale;
        float remainWidth = width;
        float hRow = 0;

        for (const MeterInfo* pmi : meters) {
            float size = boxSize / pmi->div;

            // 幅が足りなくなったら次の行へ
            if (remainWidth < size - 1) {       // 誤差で足りなくなる場合があるので -1 する
                remainWidth = width;

                // 「前の行で一番大きかった高さ」分だけ増やす
                y += hRow;
                x = 0;
                hRow = 0;

                // この行を描画できなさそうなら終了
                if (y + size >= screenHeight / g_dpiScale) {
                    return;
                }
            }

            // この行の高さを更新
            if (size >= hRow) {
                hRow = size;
            }

            D2D1_RECT_F rect = D2D1::RectF(x, y, x + size, y + size);

            // 単純に小さくすると見えなくなるので少し大きくするためのスケーリング
            float fontScale = 1.0f;
            switch (pmi->div) {
            case 2:
                fontScale = 1.4f;
                break;
            case 4:
                fontScale = 2.0f;
                break;
            }

            DrawMeter(rect, fontScale, *pmi);

            x += size;
            remainWidth -= size;
        }
        y += hRow;
    }

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
        DWORD duration2 = m_stopWatch2.GetAverageDurationMicroseconds();
        DWORD durationWorker = pWorker->m_stopWatch.GetAverageDurationMicroseconds();

        CString str;
        str.Format(L"i=%d, FPS=%d/%d, "
            L"n=%d size=%.0fx%.0f \n"
            L"%s \n"
            L"box=%.0f\n"
            L"DPI=%d,%d(%.2f)\n"
            L"描画=%5.1fms, フォント生成=%5.3fms\n"
            L"計測=%5.1fms\n"
            , iCalled, m_fpsCounter.GetAverageFps(), g_pIniConfig->mFps
            , pWorker->traffics.size(), screenWidth, screenHeight
            , (LPCTSTR)strDateTime
            , boxSize
            , g_dpix, g_dpiy, g_dpiScale
            , duration1 / 1000.0
            , duration2 / 1000.0
            , durationWorker / 1000.0
        );

        m_pBrush->SetColor(D2D1::ColorF(0xC0C0C0));
        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        // TextFormat
        IDWriteTextFormat* pTextFormat;
        m_stopWatch2.Start();
        if (CreateMyTextFormat(12.0f / g_dpiScale, &pTextFormat)) {
            m_stopWatch2.Stop();

            m_pRenderTarget->DrawText(str, str.GetLength(), pTextFormat,
                &D2D1::RectF(4, y+4, screenWidth, screenHeight), m_pBrush, D2D1_DRAW_TEXT_OPTIONS_NO_SNAP,
                DWRITE_MEASURING_MODE_NATURAL);
            SafeRelease(&pTextFormat);
        }

    }
}

void MeterDrawer::MakeCpuMemoryMeterInfo(int &nCore, CWorker * pWorker, MeterInfo &cpuMeter, std::vector<MeterInfo> &coreMeters, MeterInfo &memoryMeter)
{
    static MeterColor cpuColors[] = {
        { 90.0, D2D1::ColorF(0xFF4040) },
        { 80.0, D2D1::ColorF(0xFF8040) },
        { 70.0, D2D1::ColorF(0xC0C040) },
        {  0.0, D2D1::ColorF(0xC0C0C0) }
    };

    static MeterGuide cpuGuides[] = {
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
    nCore = pWorker->GetCpuUsage(&cpuUsage);

    // 全コアの合計
    {
        MeterInfo& mi = cpuMeter;

        mi.percent = cpuUsage.usages[0];
        mi.label.Format(L"CPU (%.0f%%)", mi.percent);
        mi.colors = cpuColors;
        mi.guides = cpuGuides;
    }

    // 各Core
    if (g_pIniConfig->mShowCoreMeters) {
        for (int i = 0; i < nCore; i++) {
            MeterInfo& mi = addMeter(coreMeters);

            mi.percent = cpuUsage.usages[i + 1];
            //              mi.label.Format(L"Core%d (%.0f%%)", i + 1, percent);
            mi.label.Format(L"Core%d", i + 1);
            mi.colors = cpuColors;
            mi.guides = cpuGuides;
        }
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

        DWORDLONG ullUsing = ms.ullTotalPhys - ms.ullAvailPhys;

        MeterInfo& mi = memoryMeter;
        mi.percent = ullUsing * 100.0f / ms.ullTotalPhys;
        mi.label.Format(L"Memory (%.0f%%)\n%I64d / %I64d MB", mi.percent, ullUsing / 1024 / 1024, ms.ullTotalPhys / 1024 / 1024);
        mi.colors = cpuColors;
        mi.guides = cpuGuides;
    }
}

void MeterDrawer::MakeDriveMeterInfo(CWorker * pWorker, std::vector<MeterInfo> &driveMeters)
{
    if (pWorker->driveUsages.size() == 0) {
        return;
    }

    DriveUsage driveUsage;
    pWorker->GetDriveUsages(&driveUsage);

    size_t nDrive = driveUsage.letters.size();

    long maxDriveKB = 10 * 1024 * 1024;
    // TODO メンバーにすること
    static MeterColor driveColors[] = {
        { KbToPercent(100000, maxDriveKB), D2D1::ColorF(0xFF4040) },
        { KbToPercent( 10000, maxDriveKB), D2D1::ColorF(0XFF8040) },
        { KbToPercent(  1000, maxDriveKB), D2D1::ColorF(0xC0C040) },
        {                             0.0, D2D1::ColorF(0xC0C0C0) }
    };
    static MeterGuide driveGuides[] = {
        { KbToPercent(10000000, maxDriveKB), D2D1::ColorF(0xFF4040), L"10G" },
        { KbToPercent( 1000000, maxDriveKB), D2D1::ColorF(0xFF4040), L"1G" },
        { KbToPercent(  100000, maxDriveKB), D2D1::ColorF(0xFF4040), L"100M" },
        { KbToPercent(   10000, maxDriveKB), D2D1::ColorF(0xFF8040), L"10M" },
        { KbToPercent(    1000, maxDriveKB), D2D1::ColorF(0xC0C040), L"1M" },
        { KbToPercent(     100, maxDriveKB), D2D1::ColorF(0xC0C040), L"100K" },
        { KbToPercent(      10, maxDriveKB), D2D1::ColorF(0xC0C040), L"10K" },
        {                               0.0, D2D1::ColorF(0xC0C0C0), L"" },
    };

    for (size_t i = 0; i < nDrive; i++) {

        // Write
        {
            MeterInfo& mi = addMeter(driveMeters);

            long kb = driveUsage.writeUsages[i] / 1024;
            mi.percent = KbToPercentL(kb, maxDriveKB);
            mi.label.Format(L"%c: Write ", driveUsage.letters[i]);
            AppendFormatOfKb(kb, mi);
            mi.colors = driveColors;
            mi.guides = driveGuides;
        }
        // Read
        {
            MeterInfo& mi = addMeter(driveMeters);

            long kb = driveUsage.readUsages[i] / 1024;
            mi.percent = KbToPercentL(kb, maxDriveKB);
            mi.label.Format(L"%c: Read ", driveUsage.letters[i]);
            AppendFormatOfKb(kb, mi);
            mi.colors = driveColors;
            mi.guides = driveGuides;
        }
    }
}

void MeterDrawer::MakeNetworkMeterInfo(CWorker * pWorker, MeterInfo &netMeterOut, MeterInfo &netMeterIn)
{
    DWORD maxTrafficKB = g_pIniConfig->mTrafficMax;

    const Traffic& t = pWorker->traffics.back();    // 一番新しいもの
    const Traffic& t0 = pWorker->traffics.front();  // 一番古いもの

    DWORD duration = t.tick - t0.tick;

    // duration が ms なので *1000 してから除算
    float inb = (t.in - t0.in) * 1000.0f / duration;
    float outb = (t.out - t0.out) * 1000.0f / duration;

    // KB単位
    maxTrafficKB /= 1000;
    inb /= 1000;
    outb /= 1000;

    // TODO メンバーにすること
    static MeterColor netColors[] = {
        { KbToPercent(1000, maxTrafficKB), D2D1::ColorF(0xFF4040) },
        { KbToPercent( 100, maxTrafficKB), D2D1::ColorF(0XFF8040) },
        { KbToPercent(  10, maxTrafficKB), D2D1::ColorF(0xC0C040) },
        {                             0.0, D2D1::ColorF(0xC0C0C0) }
    };
    static MeterGuide netGuides[] = {
        { KbToPercent(1000000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"1G" },
        { KbToPercent( 100000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"100M" },
        { KbToPercent(  10000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"10M" },
        { KbToPercent(   1000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"1M" },
        { KbToPercent(    100, maxTrafficKB), D2D1::ColorF(0xFF8040), L"100K" },
        { KbToPercent(     10, maxTrafficKB), D2D1::ColorF(0xC0C040), L"10K" },
        {                                0.0, D2D1::ColorF(0xC0C0C0), L"" },
    };

    // Up(KB単位)
    {
        float percent = outb == 0 ? 0.0f : KbToPercent(outb, maxTrafficKB);
        percent = percent < 0.0f ? 0.0f : percent;

        MeterInfo& mi = netMeterOut;
        mi.percent = percent;
        mi.label.Format(L"▲ %.1f KB/s", outb);
        mi.colors = netColors;
        mi.guides = netGuides;
    }

    // Down(KB単位)
    {
        float percent = inb == 0 ? 0.0f : KbToPercent(inb, maxTrafficKB);
        percent = percent < 0.0f ? 0.0f : percent;

        MeterInfo& mi = netMeterIn;
        mi.percent = percent;
        mi.label.Format(L"▼ %.1f KB/s", inb);
        mi.colors = netColors;
        mi.guides = netGuides;
    }
}

void MeterDrawer::AppendFormatOfKb(long kb, MeterInfo & mi)
{
    if (kb >= 1024) {
        mi.label.AppendFormat(L"%.1f MB/s", kb/1024.0);
    }
    else {
        mi.label.AppendFormat(L"%ld KB/s", kb);
    }
}

/**
 * メーターを描画する
 *
 * colors, guideLines の最後は必ず percent=0.0 にすること
 */
void MeterDrawer::DrawMeter(D2D1_RECT_F& rect, float fontScale, const MeterInfo& mi)
{
    float percent = mi.percent;
    const WCHAR* str = mi.label;
    MeterColor* colors = mi.colors;
    MeterGuide* guideLines = mi.guides;
        
    auto size = rect.right - rect.left;

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
        
        m_pRenderTarget->DrawRectangle(rect, m_pBrush);
    }

    float margin = size / 50;
    rect.left += margin;
    rect.top += margin;
    rect.right -= margin;
    rect.bottom -= margin;

    //--------------------------------------------------
    // ラベル
    //--------------------------------------------------
    float scale = 1 * size / 150.0f * fontScale;

    m_pBrush->SetColor(color);
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

    // TextFormat
    IDWriteTextFormat* pTextFormat;
    if (CreateMyTextFormat(11 * scale, &pTextFormat)) {

        m_pRenderTarget->DrawText(str, wcslen(str), pTextFormat,
            rect, m_pBrush, D2D1_DRAW_TEXT_OPTIONS_NO_SNAP,
            DWRITE_MEASURING_MODE_NATURAL);
        SafeRelease(&pTextFormat);
    }


    rect.top += size / 5.0f;
    rect.bottom += size / 5.0f;

    //--------------------------------------------------
    // メーター描画
    //--------------------------------------------------

    float mw = rect.right - rect.left;
    float length0 = mw / 2;


    // 外枠
    D2D1_POINT_2F center = { rect.left + mw / 2, rect.top + mw / 2 };

    {
        D2D1::Matrix3x2F matrix1 =
            // サイズ 100 で描画されているので拡縮する
            D2D1::Matrix3x2F::Scale({ length0 / 100.0f, length0 / 100.0f })
            *
            // 中心点に移動する
            D2D1::Matrix3x2F::Translation(center.x, center.y);

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
            0.8f * scale);

        LPCWSTR text = guideLines[i].text;
        if (wcslen(text) >= 1) {
            float rad = PI * angle / 180;
            float w = length0 / 3;
            float h = length0 / 5;
            float length = length0 * 0.72f;

            D2D1_RECT_F rect1 = { 
                center.x - length * cosf(rad),
                center.y - length * sinf(rad), 
                center.x - length * cosf(rad) + w,
                center.y - length * sinf(rad) + h };

            rect1.left -= w / 2;
            rect1.right -= w / 2;
            rect1.top -= h / 2;
            rect1.bottom -= h / 2;

            m_pBrush->SetColor(guideLines[i].color);
            m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

            IDWriteTextFormat* pTextFormat;
            if (CreateMyTextFormat(8 * scale, &pTextFormat)) {

                IDWriteTextLayout* pTextLayout = NULL;
                if (SUCCEEDED(m_pDWFactory->CreateTextLayout(text, wcslen(text), pTextFormat, rect1.right - rect1.left, rect1.bottom - rect1.top, &pTextLayout))) {

                    pTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                    m_pRenderTarget->DrawTextLayout(D2D1::Point2F(rect1.left, rect1.top), pTextLayout, m_pBrush);

                    SafeRelease(&pTextLayout);
                }

                SafeRelease(&pTextFormat);
            }

//            m_pRenderTarget->DrawRectangle(ToD2D1Rect(rect1), m_pBrush);
        }
    }


    // 針を描く
    float strokeWidth = 3 * scale;
    float angle = percent / 100.0f * (PMAX - PMIN) + PMIN;
    m_pBrush->SetColor(color);
    DrawLineByAngle(center, angle, 0, length0 * 0.9f, strokeWidth);
}

void MeterDrawer::DrawLineByAngle(D2D1_POINT_2F& center, float angle, float length1, float length2, float strokeWidth)
{
    m_pRenderTarget->SetTransform(
        D2D1::Matrix3x2F::Rotation(angle)
        *
        D2D1::Matrix3x2F::Translation(center.x, center.y)
    );
    m_pRenderTarget->DrawLine(
        D2D1::Point2F(-length1, 0),
        D2D1::Point2F(-length2, 0),
        m_pBrush,
        strokeWidth);
}

boolean MeterDrawer::CreateMyTextFormat(float fontSize, IDWriteTextFormat** ppTextFormat) {

    HRESULT hr = m_pDWFactory->CreateTextFormat(
        L"メイリオ"
        , NULL
        , DWRITE_FONT_WEIGHT_NORMAL
        , DWRITE_FONT_STYLE_NORMAL
        , DWRITE_FONT_STRETCH_NORMAL
        , fontSize
        , L""
        , ppTextFormat
    );
    if (FAILED(hr)) {
        Logger::d(L"cannot init TextFormat");
        return false;
    }
    return true;
}
