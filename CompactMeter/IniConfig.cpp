﻿#include "stdafx.h"
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
    Logger::d(L"ini file load: start");

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

    mDebugMode = ReadBoolEntry(L"DebugMode", false);
    mAlwaysOnTop = ReadBoolEntry(L"AlwaysOnTop", true);
    mDrawBorder = ReadBoolEntry(L"DrawBorder", true);

    mColumnCount = ReadIntEntry(L"ColumnCount", 2);
    NormalizeColumnCount();

    // JSONからデシリアライズする
    CString s;
    ReadStringEntry(L"MeterConfigs", L"", s);
    s.Replace(L"\\n", L"\n");
    if (!s.IsEmpty()) {
        mMeterConfigs.clear();
        Logger::d(L"MeterConfigs -> %s", s);
        CStringA sa(s);
        std::stringstream ss;
        ss << sa;
        try {
            cereal::JSONInputArchive i_archive(ss);
            i_archive(cereal::make_nvp("meters", mMeterConfigs));
        }
        catch (cereal::Exception& ex) {
            Logger::d(L"json load error: %s", CString(ex.what()));
        }
    }

    // 過不足があれば追加する
    {
        std::vector<MeterConfig> defaultMeterConfigs;
        defaultMeterConfigs.clear();
        defaultMeterConfigs.push_back(MeterConfig(METER_ID_CPU));
        defaultMeterConfigs.push_back(MeterConfig(METER_ID_MEMORY));
        defaultMeterConfigs.push_back(MeterConfig(METER_ID_CORES));
        defaultMeterConfigs.push_back(MeterConfig(METER_ID_NETWORK));

        // TODO ドライブはID別にすること
        defaultMeterConfigs.push_back(MeterConfig(METER_ID_DRIVES));

        std::set<MeterId> ids, ids0;
        for (auto& v : mMeterConfigs) {
            ids.insert(v.id);
        }
        for (auto& v : defaultMeterConfigs) {
            ids0.insert(v.id);
        }

        // 足りないものを追加する(新規追加された機能の分)
        for (auto& v : defaultMeterConfigs) {
            if (ids.find(v.id) == ids.end()) {
                Logger::d(L"見つからないので追加: %d(%s)", v.id, v.getName());
                mMeterConfigs.push_back(v);
            }
        }

        // 余分なものを削除する
        for (size_t i = 0; i < mMeterConfigs.size(); ) {
            auto& v = mMeterConfigs[i];
            if (ids0.find(v.id) == ids0.end()) {
                Logger::d(L"余分なものなので削除する: %d(%s)", v.id, v.getName());
                mMeterConfigs.erase(mMeterConfigs.begin() + i);
            }
            else {
                i++;
            }
        }

    }

    Logger::d(L"ini file load: done");
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

    // mMeterConfigsをシリアライズして保存する
    std::stringstream ss;
    {
        cereal::JSONOutputArchive o_archive(ss);
        o_archive(cereal::make_nvp("meters", mMeterConfigs));
    }
    CString s(ss.str().c_str());
//    Logger::d(L"json:%s", (LPCWSTR)s);
    s.Replace(L"\n", L"\\n");
    WriteStringEntry(L"MeterConfigs", s);
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

void IniConfig::WriteStringEntry(LPCWSTR key, LPCWSTR v)
{
    WritePrivateProfileString(szAppName, key, v, mInifilePath);
}

void IniConfig::WriteIntEntry(LPCTSTR key, int value)
{
    CString v;
    v.Format(L"%d", value);
    WritePrivateProfileString(szAppName, key, v, mInifilePath);
}

void IniConfig::ReadStringEntry(const LPCWSTR &key, const LPCWSTR &szDefault, ATL::CString &s)
{
    wchar_t buf[1024];
    int n = GetPrivateProfileString(szAppName, key, szDefault, buf, 1024, mInifilePath);
    if (n >= 1) {
        s = buf;
    }
    else {
        s = szDefault;
    }
}

int IniConfig::ReadIntEntry(LPCTSTR key, int defaultValue)
{
    return GetPrivateProfileInt(szAppName, key, defaultValue, mInifilePath);
}

bool IniConfig::ReadBoolEntry(LPCTSTR key, bool defaultValue)
{
    return ReadIntEntry(key, defaultValue ? 1 : 0) != 0;
}

