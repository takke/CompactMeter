#pragma once

class MyUtil
{
public:
    static CString GetModuleDirectoryPath();

    inline static COLORREF direct2DColorToRGB(COLORREF c) {
        return RGB((c & (0xff << 16U)) >> 16U, (c & (0xff << 8U)) >> 8U, c & 0xff);
    }
};

