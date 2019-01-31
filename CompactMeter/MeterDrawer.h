#pragma once

#include "Worker.h"
#include "FpsCounter.h"
#include "StopWatch.h"
#include "MeterConfig.h"

/**
 * メーターのガイドラインおよびカラー変更のしきい値
 */
struct MeterGuide {
    float percent;
    D2D1::ColorF color;
    LPCWSTR text;
    
    MeterGuide()
        : percent(0.0f), color(0x000000), text(L"") {}
    MeterGuide(float percent_, D2D1::ColorF color_, LPCWSTR text_)
        : percent(percent_), color(color_), text(text_) {}
};

/**
 * メーターの描画データ
 */
struct MeterInfo {

    std::vector<MeterInfo*> children;   // 子要素を持つ場合は size() >= 1

    CString     label;
    float       percent;
    MeterGuide* guides;
    int         div;                // 分割数(1 or 2 or 4)
    
    const MeterConfig* pConfig;

    MeterInfo()
        : percent(0.0f)
        , guides(nullptr)
        , div(1)
        , pConfig(nullptr)
    {}

    ~MeterInfo() {
        // children は動的に確保されるのでスコープ外で自動削除
        for (size_t i = 0; i < children.size(); i++) {
            delete children[i];
        }
    }
};

class MeterDrawer
{
private:
    FpsCounter  m_fpsCounter;

    // 左下、右下の角度
    const float P_MIN = -30;
    const float P_MAX = 180 - P_MIN;

    // デバッグ用計測器
    StopWatch   m_stopWatch1;
    StopWatch   m_stopWatch2;

    // Direct2D(DeviceDependent)
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush*  m_pBrush;

    // Direct2D(DeviceIndependent)
    ID2D1Factory*          m_pD2DFactory;
    ID2D1PathGeometry*     m_pPathGeometry;   // メーターの枠

    // DirectWrite(DeviceIndependent)
    IDWriteFactory*        m_pDWFactory;

    // ネットワークメーターのガイドライン
    // (設定値に依存するのでメンバーとして保持する)
    MeterGuide m_netGuides[10];

public:
    CComCriticalSection criticalSection;

    MeterDrawer()
        : m_pRenderTarget(nullptr)
        , m_pBrush(nullptr)
        , m_pD2DFactory(nullptr)
        , m_pPathGeometry(nullptr)
        , m_pDWFactory(nullptr)
    {
        criticalSection.Init();
    }

    ~MeterDrawer()
    {
        DiscardDeviceIndependentResources();
        DiscardDeviceResources();
    }

    void Init(HWND hWnd, int width, int height);
    void Resize(HWND hWnd, int width, int height) const;
    void SetDpi(float dpiX, float dpiY) const {
        m_pRenderTarget->SetDpi(dpiX, dpiY);
    }
    void Shutdown() const;

    void DrawToDC(HDC hdc, HWND hWnd, CWorker* pWorker);

    // IniConfig の変更後に呼び出すこと
    void InitMeterGuide();

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources(HWND hWnd, int width, int height);

    void DiscardDeviceIndependentResources()
    {
        SafeRelease(&m_pD2DFactory);
        SafeRelease(&m_pDWFactory);
        SafeRelease(&m_pPathGeometry);
    }

    void DiscardDeviceResources()
    {
        SafeRelease(&m_pRenderTarget);
        SafeRelease(&m_pBrush);
    }

    void Draw(HWND hWnd, CWorker* pWorker);
    void DrawMeters(HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight);

    int DrawMetersRecursive(std::vector<MeterInfo *> &meters, int startIndex, float boxSize, float left, float &y, float
                            right, float bottom, int depth) const;

    static bool MoveToNextBox(float &x, float & y, float size, float left, float right, float bottom);

    void MakeNetworkMeterInfo(CWorker * pWorker, MeterInfo &netMeterOut, MeterInfo &netMeterIn);
    static void MakeCpuMemoryMeterInfo(int &nCore, CWorker * pWorker, MeterInfo &cpuMeter, MeterInfo &coreMeters, MeterInfo &memoryInfo);
    void MakeDriveMeterInfo(CWorker * pWorker, std::vector<MeterInfo> &driveMeters);

    static void AppendFormatOfKb(long kb, MeterInfo & mi);
    void DrawMeter(D2D1_RECT_F& rect, const MeterInfo& mi) const;
    void DrawLineByAngle(D2D1_POINT_2F& center, float angle, float length1, float length2, float strokeWidth) const;
    bool CreateMyTextFormat(float fontSize, IDWriteTextFormat** ppTextFormat) const;

    static inline float KbToPercent(float kb, const DWORD &maxTrafficBytes)
    {
        return (log10f(static_cast<float>(kb)) / log10f(static_cast<float>(maxTrafficBytes)))*100.0f;
    }

    static inline float KbToPercentL(long kb, const DWORD &maxTrafficBytes)
    {
        return (log10f(static_cast<float>(kb)) / log10f(static_cast<float>(maxTrafficBytes)))*100.0f;
    }

    static inline MeterInfo& addMeter(std::vector<MeterInfo>& meters) {
        meters.resize(meters.size() + 1);
        return meters[meters.size() - 1];
    }

};
