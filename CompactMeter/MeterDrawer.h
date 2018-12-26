#pragma once

#include "Worker.h"
#include "Const.h"
#include "FpsCounter.h"
#include "StopWatch.h"

struct MeterColor {
    float percent;
    D2D1::ColorF color;
};
struct MeterGuide {
    float percent;
    D2D1::ColorF color;
    LPCWSTR text;
};

class MeterDrawer
{
private:
    FpsCounter  m_fpsCounter;

    const float PMIN = -30;
    const float PMAX = 180 - PMIN;

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

public:
    MeterDrawer()
        : m_pRenderTarget(NULL)
        , m_pBrush(NULL)
        , m_pD2DFactory(NULL)
        , m_pPathGeometry(NULL)
        , m_pDWFactory(NULL)
    {
    }

    ~MeterDrawer()
    {
        DiscardDeviceIndependentResources();
        DiscardDeviceResources();
    }

    void Init(HWND hWnd, int width, int height);
    void Resize(HWND hWnd, int width, int height);
    void Shutdown();

    void DrawToDC(HDC hdc, HWND hWnd, CWorker* pWorker);

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
    void DrawMeter(D2D1_RECT_F& rect, float percent, const WCHAR* str, MeterColor colors[], MeterGuide guideLines[], float fontScale);
    void DrawLineByAngle(D2D1_POINT_2F& center, float angle, float length1, float length2, float strokeWidth);
    boolean CreateMyTextFormat(float fontSize, IDWriteTextFormat** ppTextFormat);

    inline float KbToPercent(float outb, const DWORD &maxTrafficBytes)
    {
        return (log10f((float)outb) / log10f((float)maxTrafficBytes))*100.0f;
    }

};
