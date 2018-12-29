#pragma once

#include "Const.h"

enum MeterId {
    METER_ID_UNKNOWN    =   0,
    METER_ID_CPU        = 100,
    METER_ID_CORES      = 101,
    METER_ID_MEMORY     = 200,
    METER_ID_NETWORK    = 300,
    METER_ID_DRIVES     = 400,
};

struct MeterConfig {
    MeterId id;

    MeterConfig(MeterId id_) 
        : id(id_)
    {}
};

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

    boolean mDebugMode = false;

    // 設定値
    int mFps = 30;
    const int FPS_MIN = 10;
    const int FPS_MAX = 60;

    boolean mShowCoreMeters = true;

    long mTrafficMax = 300 * MB;

    boolean mAlwaysOnTop = true;
    boolean mDrawBorder = true;

    std::vector<MeterConfig> mMeterConfigs;

private:
    const wchar_t* szAppName = L"CompactMeter";

public:
    IniConfig();
    ~IniConfig();
    void Load();
    void Save();
    void NormalizeFps();

private:
    int ReadIntEntry(LPCTSTR key, int defaultValue);
    boolean ReadBooleanEntry(LPCTSTR key, boolean defaultValue);
    void WriteIntEntry(LPCTSTR key, int value);
};

