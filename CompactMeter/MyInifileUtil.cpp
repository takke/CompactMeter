#include "stdafx.h"
#include "MyInifileUtil.h"
#include "MyUtil.h"
#include "Logger.h"


MyInifileUtil::MyInifileUtil()
{
    CString directoryPath = MyUtil::GetModuleDirectoryPath();
    mInifilePath.Format(L"%s\\%s", (LPCWSTR)directoryPath, L"CompactMeter.ini");
}


MyInifileUtil::~MyInifileUtil()
{
}


void MyInifileUtil::Load()
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

    mShowCoreMeters = ReadBooleanEntry(L"ShowCoreMeters", true);

    mTrafficMax = ReadIntEntry(L"TrafficMax", 300 * MB);

    mDebugMode = ReadBooleanEntry(L"DebugMode", false);
    mAlwaysOnTop = ReadBooleanEntry(L"AlwaysOnTop", true);
    mDrawBorder = ReadBooleanEntry(L"DrawBorder", true);

    Logger::d(L"ini file loaded");
}

int MyInifileUtil::ReadIntEntry(LPCTSTR key, int defaultValue)
{
    return GetPrivateProfileInt(szAppName, key, defaultValue, mInifilePath);
}


boolean MyInifileUtil::ReadBooleanEntry(LPCTSTR key, boolean defaultValue)
{
    return ReadIntEntry(key, defaultValue ? 1 : 0) != 0;
}


void MyInifileUtil::Save()
{
    WriteIntEntry(L"WindowWidth", mWindowWidth);
    WriteIntEntry(L"WindowHeight", mWindowHeight);
    WriteIntEntry(L"PosX", mPosX);
    WriteIntEntry(L"PosY", mPosY);
    WriteIntEntry(L"ConfigDlgPosX", mConfigDlgPosX);
    WriteIntEntry(L"ConfigDlgPosY", mConfigDlgPosY);

    NormalizeFps();
    WriteIntEntry(L"FPS", mFps);

    WriteIntEntry(L"ShowCoreMeters", mShowCoreMeters ? 1 : 0);

    WriteIntEntry(L"TrafficMax", mTrafficMax);

    WriteIntEntry(L"DebugMode", mDebugMode ? 1 : 0);
    WriteIntEntry(L"AlwaysOnTop", mAlwaysOnTop ? 1 : 0);
    WriteIntEntry(L"DrawBorder", mDrawBorder ? 1 : 0);
}


void MyInifileUtil::WriteIntEntry(LPCTSTR key, int value)
{
    CString v;
    v.Format(L"%d", value);
    WritePrivateProfileString(szAppName, key, v, mInifilePath);
}

void MyInifileUtil::NormalizeFps()
{
    if (mFps < FPS_MIN) {
        mFps = FPS_MIN;
    }
    else if (mFps > FPS_MAX) {
        mFps = FPS_MAX;
    }
}
