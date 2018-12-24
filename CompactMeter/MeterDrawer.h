#pragma once

#include "Worker.h"

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

class MeterDrawer
{
public:
    MeterDrawer();
    ~MeterDrawer();
    void Draw(Graphics& g, HWND hWnd, CWorker* pWorker);

private:
    void DrawMeters(Graphics& g, HWND hWnd, CWorker* pWorker, float screenWidth, float screenHeight);
    float KbToPercent(float outb, const DWORD &maxTrafficBytes);
    void DrawMeter(Graphics& g, Gdiplus::RectF& rect, float percent, const WCHAR* str, MeterColor colors[], MeterGuide guideLines[], float fontScale);
    void DrawLineByAngle(Graphics& g, Pen* p, Gdiplus::PointF& center, float angle, float length1, float length2);
};

