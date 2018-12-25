#pragma once

#include "stdafx.h"
#include "StopWatch.h"

struct Traffic {
    ULONGLONG in;
    ULONGLONG out;
    DWORD tick;
};

struct CpuUsage {
    // index=0 が all
    // index=1 以降が各コアのCPU使用率
    std::vector<float> usages;
};

class CWorker
{
public:
    CComCriticalSection criticalSection;

    // Network
    std::list<Traffic> traffics;

    // CPU使用率
    std::list<CpuUsage> cpuUsages;

    StopWatch m_stopWatch;


public:
    CWorker(void);
    ~CWorker(void);

    void SetParams(HWND hWnd);
    static DWORD WINAPI ThreadFunc(LPVOID lpParameter);
    void Terminate();

    int GetCpuUsage(CpuUsage* out);

private:
    HWND hWnd;


    bool myExitFlag; // 終了指示を保持するフラグ 
    HANDLE myMutex;  // 排他制御

    DWORD WINAPI ExecThread();
    void CollectCpuUsage(const int &nProcessors, std::vector<PDH_HQUERY> &hQuery, std::vector<PDH_HQUERY> &hCounter, PDH_FMT_COUNTERVALUE &fntValue);
    boolean InitProcessors(std::vector<PDH_HQUERY> &hQuery, const int &nProcessors, std::vector<PDH_HQUERY> &hCounter);
    void CollectTraffic();
};

