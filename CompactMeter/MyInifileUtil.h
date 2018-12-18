#pragma once
class MyInifileUtil
{
public:
    CString mInifilePath;
    int mWindowWidth = 0;
    int mWindowHeight = 0;
    int mPosX = 0;
    int mPosY = 0;
    boolean mDebugMode = false;
    boolean mAlwaysOnTop = true;
    boolean mDrawBorder = true;

    const wchar_t* szAppName = L"CompactMeter";

    MyInifileUtil();
    ~MyInifileUtil();
    void Load();
    int ReadIntEntry(LPCTSTR key, int defaultValue);
    boolean ReadBooleanEntry(LPCTSTR key, boolean defaultValue);
    void Save();
    void WriteIntEntry(LPCTSTR key, int value);
};

