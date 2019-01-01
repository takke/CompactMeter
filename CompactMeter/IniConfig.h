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

/**
 * メーターの設定値
 */
struct MeterConfig {
    MeterId id;
    boolean enable;
//    int backgroundColor;

    MeterConfig(MeterId id_, boolean enable_=true) 
        : id(id_), enable(enable_)
    {}
    MeterConfig() : id(METER_ID_UNKNOWN), enable(false) 
    {}

    LPCWSTR getName() const {

        switch (id) {
        case METER_ID_CPU:
            return L"CPU";
        case METER_ID_CORES:
            return L"Core";
        case METER_ID_MEMORY:
            return L"Memory";
        case METER_ID_NETWORK:
            return L"Network";
        case METER_ID_DRIVES:
            return L"Drives";
        case METER_ID_UNKNOWN:
        default:
            return L"Unknown";
        }
    }

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(id), CEREAL_NVP(enable));
    }
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

    long mTrafficMax = 300 * MB;

    boolean mAlwaysOnTop = true;
    boolean mDrawBorder = true;

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

private:
    void WriteStringEntry(LPCWSTR key, LPCWSTR v);
    void WriteIntEntry(LPCTSTR key, int value);
    void ReadStringEntry(const LPCWSTR &key, const LPCWSTR &szDefault, ATL::CString &s);
    int ReadIntEntry(LPCTSTR key, int defaultValue);
    boolean ReadBooleanEntry(LPCTSTR key, boolean defaultValue);
};

