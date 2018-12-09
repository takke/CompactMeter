#include "stdafx.h"
#include "MyInifileUtil.h"


MyInifileUtil::MyInifileUtil()
{
	TCHAR modulePath[MAX_PATH];
	GetModuleFileName(NULL, modulePath, MAX_PATH);

	TCHAR drive[MAX_PATH + 1], dir[MAX_PATH + 1], fname[MAX_PATH + 1], ext[MAX_PATH + 1];
	_wsplitpath_s(modulePath, drive, dir, fname, ext);

	mInifilePath.Format(L"%s%s\\%s", drive, dir, L"CompactMeter.ini");
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
