#include "stdafx.h"
#include "IniConfig.h"
#include "MyUtil.h"
#include "Logger.h"


IniConfig::IniConfig()
{
    CString directoryPath = MyUtil::GetModuleDirectoryPath();
    mInifilePath.Format(L"%s\\%s", (LPCWSTR)directoryPath, L"CompactMeter.ini");
}


IniConfig::~IniConfig()
{
}

void IniConfig::Load()
{
    // 画面サイズのデフォルト値
    mWindowWidth = ReadIntEntry(L"WindowWidth", 300);
    mWindowHeight = ReadIntEntry(L"WindowHeight", 600);

    mPosX = ReadIntEntry(L"PosX", 0);
    mPosY = ReadIntEntry(L"PosY", 0);
    mConfigDlgPosX = ReadIntEntry(L"ConfigDlgPosX", INT_MAX);
    mConfigDlgPosY = ReadIntEntry(L"ConfigDlgPosY", INT_MAX);

    mFps = ReadIntEntry(L"FPS", 30);
    NormalizeFps();

    mTrafficMax = ReadIntEntry(L"TrafficMax", 300 * MB);

    mDebugMode = ReadBooleanEntry(L"DebugMode", false);
    mAlwaysOnTop = ReadBooleanEntry(L"AlwaysOnTop", true);
    mDrawBorder = ReadBooleanEntry(L"DrawBorder", true);

    mColumnCount = ReadIntEntry(L"ColumnCount", 2);
    NormalizeColumnCount();

    // TODO JSONからデシリアライズすること
    mMeterConfigs.clear();
    mMeterConfigs.push_back(MeterConfig(METER_ID_CPU));
    mMeterConfigs.push_back(MeterConfig(METER_ID_MEMORY));
    mMeterConfigs.push_back(MeterConfig(METER_ID_CORES));
    mMeterConfigs.push_back(MeterConfig(METER_ID_NETWORK));
    mMeterConfigs.push_back(MeterConfig(METER_ID_DRIVES));

    Logger::d(L"ini file loaded");
}

int IniConfig::ReadIntEntry(LPCTSTR key, int defaultValue)
{
    return GetPrivateProfileInt(szAppName, key, defaultValue, mInifilePath);
}

boolean IniConfig::ReadBooleanEntry(LPCTSTR key, boolean defaultValue)
{
    return ReadIntEntry(key, defaultValue ? 1 : 0) != 0;
}

void IniConfig::Save()
{
    WriteIntEntry(L"WindowWidth", mWindowWidth);
    WriteIntEntry(L"WindowHeight", mWindowHeight);
    WriteIntEntry(L"PosX", mPosX);
    WriteIntEntry(L"PosY", mPosY);
    WriteIntEntry(L"ConfigDlgPosX", mConfigDlgPosX);
    WriteIntEntry(L"ConfigDlgPosY", mConfigDlgPosY);

    NormalizeFps();
    WriteIntEntry(L"FPS", mFps);

    WriteIntEntry(L"TrafficMax", mTrafficMax);

    WriteIntEntry(L"DebugMode", mDebugMode ? 1 : 0);
    WriteIntEntry(L"AlwaysOnTop", mAlwaysOnTop ? 1 : 0);
    WriteIntEntry(L"DrawBorder", mDrawBorder ? 1 : 0);

    NormalizeColumnCount();
    WriteIntEntry(L"ColumnCount", mColumnCount);

    // TODO mMeterConfigsをシリアライズして保存すること
}

void IniConfig::WriteIntEntry(LPCTSTR key, int value)
{
    CString v;
    v.Format(L"%d", value);
    WritePrivateProfileString(szAppName, key, v, mInifilePath);
}

void IniConfig::NormalizeFps()
{
    if (mFps < FPS_MIN) {
        mFps = FPS_MIN;
    }
    else if (mFps > FPS_MAX) {
        mFps = FPS_MAX;
    }
}

void IniConfig::NormalizeColumnCount() {
    if (mColumnCount < COLUMN_COUNT_MIN) {
        mColumnCount = COLUMN_COUNT_MIN;
    }
    else if (mColumnCount > COLUMN_COUNT_MAX) {
        mColumnCount = COLUMN_COUNT_MAX;
    }
}
