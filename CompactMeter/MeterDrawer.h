#pragma once

#include "Worker.h"
#include "Const.h"
#include "FpsCounter.h"
#include "StopWatch.h"

using namespace Gdiplus;

struct MeterColor {
    float percent;
    Color color;
};
struct MeterGuide {
    float percent;
    Color color;
    LPCWSTR text;
};
struct MeterColorD2D {
    float percent;
    D2D1::ColorF color;
};
struct MeterGuideD2D {
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

    // Direct2D(DeviceDependent)
    ID2D1HwndRenderTarget*       m_pRenderTarget;
    ID2D1SolidColorBrush*        m_pBrush;

    // Direct2D(DeviceIndependent)
    ID2D1Factory*                m_pD2DFactory;
    IDWriteTextFormat*           m_pTextFormat;     // デバッグログ表示用
    IDWriteTextFormat*           m_pTextFormat2;    // メーターの文字用
    IDWriteTextFormat*           m_pTextFormat3;    // メーターのラベル用
    ID2D1PathGeometry*           m_pPathGeometry;   // メーターの枠

    // DirectWrite(DeviceIndependent)
    IDWriteFactory*              m_pDWFactory;

public:
    MeterDrawer()
        : m_pRenderTarget(NULL)
        , m_pBrush(NULL)
        , m_pD2DFactory(NULL)
        , m_pTextFormat(NULL)
        , m_pTextFormat2(NULL)
        , m_pTextFormat3(NULL)
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

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources(HWND hWnd, int width, int height);

    void DiscardDeviceIndependentResources()
    {
        SafeRelease(&m_pD2DFactory);
        SafeRelease(&m_pDWFactory);
        SafeRelease(&m_pTextFormat);
        SafeRelease(&m_pTextFormat2);
        SafeRelease(&m_pTextFormat3);
        SafeRelease(&m_pPathGeometry);
    }

    void DiscardDeviceResources()
    {
        SafeRelease(&m_pRenderTarget);
        SafeRelease(&m_pBrush);
    }

private:
    void Draw(HWND hWnd, CWorker* pWorker);
    void DrawMeters(HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight);
    void DrawMeter(D2D1_RECT_F& rect, float percent, const WCHAR* str, MeterColorD2D colors[], MeterGuideD2D guideLines[], float fontScale);

    void DrawLineByAngle(Gdiplus::PointF &center, float angle, float length1, float length2, float strokeWidth);

    inline float KbToPercent(float outb, const DWORD &maxTrafficBytes)
    {
        return (log10f((float)outb) / log10f((float)maxTrafficBytes))*100.0f;
    }

};
