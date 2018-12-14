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
	mWindowWidth = GetPrivateProfileInt(szAppName, L"WindowWidth", 300, mInifilePath);
	mWindowHeight = GetPrivateProfileInt(szAppName, L"WindowHeight", 600, mInifilePath);

	mPosX = GetPrivateProfileInt(szAppName, L"PosX", 0, mInifilePath);
	mPosY = GetPrivateProfileInt(szAppName, L"PosY", 0, mInifilePath);

	mDebugMode = GetPrivateProfileInt(szAppName, L"DebugMode", 0, mInifilePath) != 0;
	mAlwaysOnTop = GetPrivateProfileInt(szAppName, L"AlwaysOnTop", 1, mInifilePath) != 0;

	Logger::d(L"ini file loaded");
}


void MyInifileUtil::Save()
{
	WriteIntEntry(L"WindowWidth", mWindowWidth);
	WriteIntEntry(L"WindowHeight", mWindowHeight);
	WriteIntEntry(L"PosX", mPosX);
	WriteIntEntry(L"PosY", mPosY);
	WriteIntEntry(L"DebugMode", mDebugMode ? 1 : 0);
	WriteIntEntry(L"AlwaysOnTop", mAlwaysOnTop ? 1 : 0);
}


void MyInifileUtil::WriteIntEntry(LPCTSTR key, int value)
{
	CString v;
	v.Format(L"%d", value);
	WritePrivateProfileString(szAppName, key, v, mInifilePath);
}
