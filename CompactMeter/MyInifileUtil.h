#pragma once
class MyInifileUtil
{
public:
	CString mInifilePath;
	int mWindowWidth = 0;
	int mWindowHeight = 0;
	int mPosX = 0;
	int mPosY = 0;

	const wchar_t* szAppName = L"CompactMeter";

	MyInifileUtil();
	~MyInifileUtil();
	void load();
	void save();
};

