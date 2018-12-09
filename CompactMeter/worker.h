#pragma once

#include "stdafx.h"

struct Traffic {
	ULONGLONG in;
	ULONGLONG out;
	DWORD tick;
};

class CWorker
{
public:
	CWorker(void);
	~CWorker(void);

	void SetParams(HWND hWnd);
	static DWORD WINAPI ThreadFunc(LPVOID lpParameter);
	void Terminate();

	// Network
	std::vector<Traffic> traffics;


private:
	HWND hWnd;


	bool myExitFlag; // 終了指示を保持するフラグ 
	HANDLE myMutex;  // 排他制御

	DWORD WINAPI ExecThread();
	void CollectTraffic();
};

