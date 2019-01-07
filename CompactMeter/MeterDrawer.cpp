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

void MeterDrawer::InitMeterGuide() {

    DWORD maxTrafficKB = g_pIniConfig->mTrafficMax / 1000;

    int i = 0;
    m_netGuides[i++] = { KbToPercent(1'000'000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"1G" };
    m_netGuides[i++] = { KbToPercent(  100'000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"100M" };
    m_netGuides[i++] = { KbToPercent(   10'000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"10M" };
    m_netGuides[i++] = { KbToPercent(    1'000, maxTrafficKB), D2D1::ColorF(0xFF4040), L"1M" };
    m_netGuides[i++] = { KbToPercent(      100, maxTrafficKB), D2D1::ColorF(0xFF8040), L"100K" };
    m_netGuides[i++] = { KbToPercent(       10, maxTrafficKB), D2D1::ColorF(0xC0C040), L"10K" };
    m_netGuides[i++] = {                                  0.0, D2D1::ColorF(0xC0C0C0), L"" };
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

void addWithConfig(std::vector<MeterInfo*>& meters, MeterInfo* pMeter, const MeterConfig* pMeterConfig) {

    // config値を引き渡す
    pMeter->pConfig = pMeterConfig;
    if (!pMeter->children.empty()) {
        for (auto& v : pMeter->children) {
            v->pConfig = pMeterConfig;
        }
    }

    meters.push_back(pMeter);
}

void MeterDrawer::DrawMeters(HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight)
{
    //--------------------------------------------------
    // 各メーター情報の収集
    //--------------------------------------------------
    int nCore = 0;

    MeterInfo cpuMeter;
    MeterInfo coreMeters;
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

    for (const auto& mc : g_pIniConfig->mMeterConfigs) {
        if (!mc.enable) {
            continue;
        }

        switch (mc.id) {
        case METER_ID_UNKNOWN:
            break;
        case METER_ID_CPU:
            addWithConfig(meters, &cpuMeter, &mc);
            break;
        case METER_ID_CORES:
            addWithConfig(meters, &coreMeters, &mc);
            break;
        case METER_ID_MEMORY:
            addWithConfig(meters, &memoryMeter, &mc);
            break;
        case METER_ID_NETWORK:
            addWithConfig(meters, &netMeterOut, &mc);
            addWithConfig(meters, &netMeterIn, &mc);
            break;
        default:
            if (METER_ID_DRIVE_A <= mc.id && mc.id <= METER_ID_DRIVE_Z) {
                char letter = 'A' + (mc.id - METER_ID_DRIVE_A);
                // Drive{letter} を探して追加する
                for (auto& d : driveMeters) {
                    if (d.label[0] == letter) {
                        // 該当ドライブを見つけたので追加
                        addWithConfig(meters, &d, &mc);
                    }
                }
            }
            break;
        }
    }


    //--------------------------------------------------
    // 各メーターの描画
    //--------------------------------------------------

    // 各ウィジェットの標準サイズ(width, height)
    float boxSize = screenWidth / g_pIniConfig->mColumnCount / g_dpiScale;     // box size
    float y = 0;

    const float width = screenWidth / g_dpiScale;
    const float height = screenHeight / g_dpiScale;

    if (!DrawMetersRecursive(meters, 0, boxSize, 0, y, width, height)) {
        return;
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

/**
 * 指定された描画範囲(left -> left+width, y -> bottom) 内に収まるように meters を描画する
 *
 * 各メーターが子要素を持つ場合はそれを再帰的に描画する
 * 
 * 描画できた要素数を返す
 */
int MeterDrawer::DrawMetersRecursive(std::vector<MeterInfo *> &meters, int startIndex, float boxSize, float left, float &y, float right, float bottom)
{
    float x = left;

    int nDrawn = 0;
    for (int i = startIndex; i < (int)meters.size(); i++) {
        MeterInfo* pmi = meters[i];

        float size = boxSize / pmi->div;

        if (pmi->children.size() >= 1) {
            
            // 子要素全てを再帰的に描画する
            for (int iChildren = 0; ; ) {

                float y1 = y;
                int n = DrawMetersRecursive(pmi->children, iChildren, boxSize, x, y1, x + boxSize, y + boxSize);
                if (n == 0) {
                    return nDrawn;
                }
                iChildren += n;

                nDrawn += n;

                // 次の描画範囲に移動する
                if (!MoveToNextBox(x, y, size, left, right, bottom)) {
                    return nDrawn;
                }

                if (iChildren >= (int)pmi->children.size()) {
                    // 全て描画し終わったので終了
                    break;
                }
                // children がまだ残っているので次の描画範囲に移動して継続する
            }

        }
        else {

            //--------------------------------------------------
            // 描画
            //--------------------------------------------------
            D2D1_RECT_F rect = D2D1::RectF(x, y, x + size, y + size);

            DrawMeter(rect, *pmi);

            nDrawn++;

            // 次の描画範囲に移動する
            if (!MoveToNextBox(x, y, size, left, right, bottom)) {
                return nDrawn;
            }
        }

    }

    if (x > left) {
        y += boxSize;
    }

    return nDrawn;
}

bool MeterDrawer::MoveToNextBox(float &x, float & y, float size, float left, float right, float bottom)
{
    // 描画範囲を右に移動する
    x += size;

    // 幅が足りなくなったら次の行へ
    if (x + size - 1 > right) {       // 誤差で足りなくなる場合があるので -1 する

        y += size;
        x = left;

        // この行を描画できなさそうなら終了
        if (y + size - 1 >= bottom) {
            return false;
        }
    }

    return true;
}

void MeterDrawer::MakeCpuMemoryMeterInfo(int &nCore, CWorker * pWorker, MeterInfo &cpuMeter, MeterInfo &coreMeters, MeterInfo &memoryMeter)
{
    static MeterGuide cpuGuides[] = {
        { 100.0, D2D1::ColorF(0xFF4040), L"" },
        {  90.0, D2D1::ColorF(0xFF4040), L"" },
        {  80.0, D2D1::ColorF(0xFF8040), L"" },
        {  70.0, D2D1::ColorF(0xC0C040), L"" },
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
        mi.guides = cpuGuides;
    }

    // 各Core
    for (int i = 0; i < nCore; i++) {

        // children はスコープ外で自動削除される
        MeterInfo* pmi = new MeterInfo();
        coreMeters.children.push_back(pmi);
        MeterInfo& mi = *pmi;

        mi.percent = cpuUsage.usages[i + 1];
//          mi.label.Format(L"Core%d (%.0f%%)", i + 1, percent);
        mi.label.Format(L"Core%d", i + 1);
        mi.guides = cpuGuides;
        mi.div = 2;

        // 仮想的に 2 コアなどを模擬するなら下記のような感じで。
//        if (i+1 >= 2) break;
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
    static MeterGuide driveGuides[] = {
        { KbToPercent(10'000'000, maxDriveKB), D2D1::ColorF(0xFF4040), L"10G" },
        { KbToPercent( 1'000'000, maxDriveKB), D2D1::ColorF(0xFF4040), L"1G" },
        { KbToPercent(   100'000, maxDriveKB), D2D1::ColorF(0xFF4040), L"100M" },
        { KbToPercent(    10'000, maxDriveKB), D2D1::ColorF(0xFF8040), L"10M" },
        { KbToPercent(     1'000, maxDriveKB), D2D1::ColorF(0xC0C040), L"1M" },
        { KbToPercent(       100, maxDriveKB), D2D1::ColorF(0xC0C0C0), L"100K" },
        { KbToPercent(        10, maxDriveKB), D2D1::ColorF(0xC0C0C0), L"10K" },
        {                                 0.0, D2D1::ColorF(0xC0C0C0), L"" },
    };

    for (size_t i = 0; i < nDrive; i++) {

        // Write
        {
            MeterInfo& mi = addMeter(driveMeters);

            long kb = driveUsage.writeUsages[i] / 1024;
            mi.percent = KbToPercentL(kb, maxDriveKB);
            mi.label.Format(L"%c: Write ", driveUsage.letters[i]);
            AppendFormatOfKb(kb, mi);
            mi.guides = driveGuides;
        }
        // Read
        {
            MeterInfo& mi = addMeter(driveMeters);

            long kb = driveUsage.readUsages[i] / 1024;
            mi.percent = KbToPercentL(kb, maxDriveKB);
            mi.label.Format(L"%c: Read ", driveUsage.letters[i]);
            AppendFormatOfKb(kb, mi);
            mi.guides = driveGuides;
        }
    }
}

void MeterDrawer::MakeNetworkMeterInfo(CWorker * pWorker, MeterInfo &netMeterOut, MeterInfo &netMeterIn)
{
    DWORD maxTrafficKB = g_pIniConfig->mTrafficMax / 1000;

    const Traffic& t = pWorker->traffics.back();    // 一番新しいもの
    const Traffic& t0 = pWorker->traffics.front();  // 一番古いもの

    DWORD duration = t.tick - t0.tick;

    // duration が ms なので *1000 してから除算
    float inb = (t.in - t0.in) * 1000.0f / duration;
    float outb = (t.out - t0.out) * 1000.0f / duration;

    // KB単位
    inb /= 1000;
    outb /= 1000;

    // Up(KB単位)
    {
        float percent = outb == 0 ? 0.0f : KbToPercent(outb, maxTrafficKB);
        percent = percent < 0.0f ? 0.0f : percent;

        MeterInfo& mi = netMeterOut;
        mi.percent = percent;
        mi.label.Format(L"▲ %.1f KB/s", outb);
        mi.guides = m_netGuides;
    }

    // Down(KB単位)
    {
        float percent = inb == 0 ? 0.0f : KbToPercent(inb, maxTrafficKB);
        percent = percent < 0.0f ? 0.0f : percent;

        MeterInfo& mi = netMeterIn;
        mi.percent = percent;
        mi.label.Format(L"▼ %.1f KB/s", inb);
        mi.guides = m_netGuides;
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
void MeterDrawer::DrawMeter(D2D1_RECT_F& rect, const MeterInfo& mi)
{
    // 単純に小さくすると見えなくなるので少し大きくするためのスケーリング
    float fontScale = 1.0f;
    switch (mi.div) {
    case 2:
        fontScale = 1.4f;
        break;
    case 4:
        fontScale = 2.0f;
        break;
    }

    float percent = mi.percent;
    const WCHAR* str = mi.label;
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
        if (percent >= guideLines[i].percent) {
            color = guideLines[i].color;
            break;
        }

        if (guideLines[i].percent == 0.0f) {
            break;
        }
    }

    //--------------------------------------------------
    // 背景
    //--------------------------------------------------
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    if (mi.pConfig != NULL) {
        m_pBrush->SetColor(D2D1::ColorF(mi.pConfig->backgroundColor));
        rect.right += 1;
        rect.bottom += 1;
        m_pRenderTarget->FillRectangle(rect, m_pBrush);
        rect.right -= 1;
        rect.bottom -= 1;
    }

    //--------------------------------------------------
    // 枠線
    //--------------------------------------------------
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

        m_pRenderTarget->DrawText(str, (UINT32)wcslen(str), pTextFormat,
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
                if (SUCCEEDED(m_pDWFactory->CreateTextLayout(text, (UINT32)wcslen(text), pTextFormat, rect1.right - rect1.left, rect1.bottom - rect1.top, &pTextLayout))) {

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

bool MeterDrawer::CreateMyTextFormat(float fontSize, IDWriteTextFormat** ppTextFormat) {

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
