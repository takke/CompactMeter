#pragma once
class MyInifileUtil
{
public:
    CString mInifilePath;
    int mWindowWidth = 0;
    int mWindowHeight = 0;
    int mPosX = 0;
    int mPosY = 0;
    int mFps = 30;
    const int FPS_MIN = 10;
    const int FPS_MAX = 60;
    boolean mDebugMode = false;
    boolean mAlwaysOnTop = true;
    boolean mDrawBorder = true;

private:
    const wchar_t* szAppName = L"CompactMeter";

public:
    MyInifileUtil();
    ~MyInifileUtil();
    void Load();
    void Save();
    void NormalizeFps();

private:
    int ReadIntEntry(LPCTSTR key, int defaultValue);
    boolean ReadBooleanEntry(LPCTSTR key, boolean defaultValue);
    void WriteIntEntry(LPCTSTR key, int value);
};

