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


void MyInifileUtil::load()
{
	mWindowWidth = GetPrivateProfileInt(szAppName, L"WindowWidth", 700, mInifilePath);
	mWindowHeight = GetPrivateProfileInt(szAppName, L"WindowHeight", 700, mInifilePath);
	mPosX = GetPrivateProfileInt(szAppName, L"PosX", 0, mInifilePath);
	mPosY = GetPrivateProfileInt(szAppName, L"PosY", 0, mInifilePath);

	Logger::d(L"ini file loaded");
}


void MyInifileUtil::save()
{
//	wprintf(L"path=%s\n", (LPCTSTR)path);
	CString v;

	v.Format(L"%d", mWindowWidth);
	WritePrivateProfileString(szAppName, L"WindowWidth", v, mInifilePath);

	v.Format(L"%d", mWindowHeight);
	WritePrivateProfileString(szAppName, L"WindowHeight", v, mInifilePath);
	
	v.Format(L"%d", mPosX);
	WritePrivateProfileString(szAppName, L"PosX", v, mInifilePath);

	v.Format(L"%d", mPosY);
	WritePrivateProfileString(szAppName, L"PosY", v, mInifilePath);
}
