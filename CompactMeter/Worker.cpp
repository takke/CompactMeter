#include "stdafx.h"
#include "Worker.h"
#include "Logger.h"
#include "MyInifileUtil.h"

extern boolean g_dragging;
extern MyInifileUtil* g_pMyInifile;

CWorker::CWorker(void)
{
    myMutex = CreateMutex(NULL, TRUE, NULL);
    myExitFlag = false;
}


CWorker::~CWorker(void)
{
    CloseHandle(myMutex);
}

void CWorker::SetParams(HWND hWnd)
{
    this->hWnd = hWnd;
}

DWORD WINAPI CWorker::ThreadFunc(LPVOID lpParameter)
{
    return ((CWorker*)lpParameter)->ExecThread();
}

DWORD WINAPI CWorker::ExecThread()
{
    criticalSection.Init();
    Logger::d(L"Worker start");

    // CPU 使用率の計測準備
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    const int nProcessors = info.dwNumberOfProcessors;
    Logger::d(L" processors: %d", nProcessors);

    std::vector<PDH_HQUERY>   hQuery;
    std::vector<PDH_HCOUNTER> hCounter;
    PDH_FMT_COUNTERVALUE      fntValue;
    if( !InitProcessors(hQuery, nProcessors, hCounter) ) {
        // 初期化失敗
        return S_OK;
    }

    while (true) {

        if (!g_dragging) {

            criticalSection.Lock();

            // Network
            CollectTraffic();

            // CPU 使用率計測
            CollectCpuUsage(nProcessors, hQuery, hCounter, fntValue);

            criticalSection.Unlock();

            InvalidateRect(hWnd, NULL, FALSE);
        }

        Sleep(1000 / g_pMyInifile->mFps);


        // 終了チェック
        WaitForSingleObject(myMutex, 0);
        bool currentFlag = myExitFlag;
        ReleaseMutex(myMutex);
        if (currentFlag) {
            break;
        }
    }

    Logger::d(L"Worker end");

    // CPU 使用率計測用データ解放
    for (int i = 0; i < nProcessors + 1; i++) {
        PdhCloseQuery(hQuery[i]);
    }

    return S_OK;
}

void CWorker::CollectCpuUsage(const int &nProcessors, std::vector<PDH_HQUERY> &hQuery, std::vector<PDH_HQUERY> &hCounter, PDH_FMT_COUNTERVALUE &fntValue)
{
    CpuUsage usage;
    usage.usages.resize(nProcessors+1);

    for (int i = 0; i < nProcessors + 1; i++) {
        PdhCollectQueryData(hQuery[i]);
        PdhGetFormattedCounterValue(hCounter[i], PDH_FMT_LONG, NULL, &fntValue);
        usage.usages[i] = (float)fntValue.longValue;
    }
    cpuUsages.push_back(usage);

    while (cpuUsages.size() > (size_t)g_pMyInifile->mFps) {
        cpuUsages.erase(cpuUsages.begin());
    }
}

boolean CWorker::InitProcessors(std::vector<PDH_HQUERY> &hQuery, const int &nProcessors, std::vector<PDH_HQUERY> &hCounter)
{
    hQuery.resize(nProcessors + 1);
    hCounter.resize(nProcessors + 1);

    // 各コア
    for (int i = 0; i < nProcessors + 1; i++) {
        if (PdhOpenQuery(NULL, 0, &hQuery[i]) != ERROR_SUCCESS) {
            Logger::d(L"クエリーをオープンできません。%d", i);
            return false;
        }
    }

    // ALL
    PdhAddCounter(hQuery[0], L"\\Processor(_Total)\\% Processor Time", 0, &hCounter[0]);
    PdhCollectQueryData(hQuery[0]);
    for (int i = 0; i < nProcessors; i++) {
        CString query;
        query.Format(L"\\Processor(%d)\\%% Processor Time", i);
        PdhAddCounter(hQuery[i + 1], (LPCWSTR)query, 0, &hCounter[i + 1]);
        PdhCollectQueryData(hQuery[i + 1]);
    }

    return true;
}

void CWorker::CollectTraffic()
{
    PMIB_IF_TABLE2 ifTable2;

    Traffic t;
    t.in = 0;
    t.out = 0;
    t.tick = GetTickCount();

    if (GetIfTable2(&ifTable2) == NO_ERROR) {
        if (ifTable2->NumEntries > 0) {
//          printf("----\n");
//          printf("Number of Adapters: %ld\n\n", ifTable2->NumEntries);
            for (int i = ifTable2->NumEntries - 1; i >= 0; i--) {

                MIB_IF_ROW2* ifrow2 = &ifTable2->Table[i];
//              wprintf(L"[%d] %s\n", i, ifrow2->Description);


                if (!ifrow2->InterfaceAndOperStatusFlags.FilterInterface &&
                    (ifrow2->Type == IF_TYPE_ETHERNET_CSMACD ||
                     ifrow2->Type == IF_TYPE_IEEE80211 ||
                     ifrow2->Type == IF_TYPE_PPP)
                    )
                {
                    GetIfEntry2(ifrow2);
                    t.in += ifrow2->InOctets;
                    t.out += ifrow2->OutOctets;
                }
            }

            traffics.push_back(t);
//          printf("=> in[%lld], out[%lld]\n", t.in, t.out);
        }
        FreeMibTable(ifTable2);
    }
    else {
        printf("no adapters");
    }

    while (traffics.size() > (size_t)g_pMyInifile->mFps) {
        traffics.erase(traffics.begin());
    }

}

void CWorker::Terminate()
{
    Logger::d(L"Worker terminate");

    WaitForSingleObject(myMutex, 0);
    myExitFlag = true;
    ReleaseMutex(myMutex);
}

int CWorker::GetCpuUsage(CpuUsage* out)
{
    int n = cpuUsages.size();
    if (n <= 0) {
        return 0;
    }

    int nCore = cpuUsages.begin()->usages.size();

    // 初期化
    out->usages.resize(nCore);
    for (int c = 0; c < nCore; c++) {
        out->usages[c] = 0;
    }

    // 平均値を計算する
    for (auto it = cpuUsages.begin(); it != cpuUsages.end(); it++) {
        for (int c = 0; c < nCore; c++) {
            out->usages[c] += it->usages[c];
        }
    }
    for (int c = 0; c < nCore; c++) {
        out->usages[c] /= n;
    }

    return nCore-1;
}
