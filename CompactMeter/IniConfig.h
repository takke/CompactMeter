#pragma once

#include "MeterConfig.h"
#include "Const.h"

class IniConfig
{
public:
    CString mInifilePath;
    int mWindowWidth = 0;
    int mWindowHeight = 0;
    int mPosX = 0;
    int mPosY = 0;

    int mConfigDlgPosX = 0;
    int mConfigDlgPosY = 0;

    bool mDebugMode = false;

    // 設定値
    int mFps = 30;
    const int FPS_MIN = 10;
    const int FPS_MAX = 60;

    long mTrafficMax = 300 * MB;

    bool mAlwaysOnTop = true;
    bool mDrawBorder = true;

    // メーターの列数
    int mColumnCount = 2;
    const int COLUMN_COUNT_MIN = 1;
    const int COLUMN_COUNT_MAX = 6;

    // 各メーターの設定値(および順序)
    std::vector<MeterConfig> mMeterConfigs;

private:
    const wchar_t* szAppName = L"CompactMeter";

public:
    IniConfig();
    ~IniConfig();
    void Load();
    void Save();
    void NormalizeFps();
    void NormalizeColumnCount();
    void NormalizeWidthHeight();

private:
    void WriteStringEntry(LPCWSTR key, LPCWSTR v);
    void WriteIntEntry(LPCTSTR key, int value);
    void ReadStringEntry(const LPCWSTR &key, const LPCWSTR &szDefault, ATL::CString &s);
    int ReadIntEntry(LPCTSTR key, int defaultValue);
    bool ReadBoolEntry(LPCTSTR key, bool defaultValue);
};

