#include "stdafx.h"
#include "Worker.h"
#include "Logger.h"
#include "IniConfig.h"
#include "MeterDrawer.h"

extern boolean g_dragging;
extern IniConfig* g_pIniConfig;
extern MeterDrawer g_meterDrawer;

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

    //--------------------------------------------------
    // CPU 使用率の取得準備
    //--------------------------------------------------
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    const int nProcessors = info.dwNumberOfProcessors;
    Logger::d(L" processors: %d", nProcessors);

    std::vector<PDH_HQUERY>   hCpuQuery;
    std::vector<PDH_HCOUNTER> hCpuCounter;
    PDH_FMT_COUNTERVALUE      fntValue;
    if (!InitProcessors(hCpuQuery, nProcessors, hCpuCounter)) {
        // 初期化失敗
        return S_OK;
    }

    //--------------------------------------------------
    // ドライブ使用量の取得準備
    //--------------------------------------------------
    std::vector<PDH_HQUERY>   hDriveReadQuery;
    std::vector<PDH_HCOUNTER> hDriveReadCounter;
    std::vector<PDH_HQUERY>   hDriveWriteQuery;
    std::vector<PDH_HCOUNTER> hDriveWriteCounter;
    int nDrives = 0;
    if (!InitDrives(hDriveWriteQuery, hDriveWriteCounter, hDriveReadQuery, hDriveReadCounter, nDrives)) {
        // 初期化失敗
        return S_OK;
    }

    while (true) {

        DWORD start = GetTickCount();

        if (!g_dragging) {

            criticalSection.Lock();
            m_stopWatch.Start();

            // Network 通信量取得
            CollectTraffic();

            // CPU 使用率取得
            CollectCpuUsage(nProcessors, hCpuQuery, hCpuCounter, fntValue);

            // Drive 使用量取得
            CollectDriveUsage(nDrives, hDriveReadQuery, hDriveReadCounter, fntValue, hDriveWriteQuery, hDriveWriteCounter);

            m_stopWatch.Stop();
            criticalSection.Unlock();

            // 描画
//            InvalidateRect(hWnd, NULL, FALSE);
            HDC hdc = ::GetDC(hWnd);
            g_meterDrawer.DrawToDC(hdc, hWnd, this);
            ::ReleaseDC(hWnd, hdc);
        }
        DWORD elapsed = GetTickCount() - start;

        int sleep = 1000 / g_pIniConfig->mFps - elapsed;

        if (sleep < 5) {
            sleep = 5;
        }
        Sleep(sleep);


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
        PdhCloseQuery(hCpuQuery[i]);
    }
    for (int i = 0; i < 26; i++) {
        if (hDriveReadQuery[i] != NULL) {
            PdhCloseQuery(hDriveReadQuery[i]);
            PdhCloseQuery(hDriveWriteQuery[i]);
        }
    }

    return S_OK;
}

boolean CWorker::InitProcessors(std::vector<PDH_HQUERY> &hQuery, const int &nProcessors, std::vector<PDH_HQUERY> &hCounter)
{
    hQuery.resize(nProcessors + 1);
    hCounter.resize(nProcessors + 1);

    // 各コア
    for (int i = 0; i < nProcessors + 1; i++) {
        if (PdhOpenQuery(NULL, 0, &hQuery[i]) != ERROR_SUCCESS) {
            Logger::d(L"CPUクエリーをオープンできません。%d", i);
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

boolean CWorker::InitDrives(std::vector<PDH_HQUERY> &hDriveWriteQuery, std::vector<PDH_HQUERY> &hDriveWriteCounter, std::vector<PDH_HQUERY> &hDriveReadQuery, std::vector<PDH_HQUERY> &hDriveReadCounter, int &nDrives)
{
    hDriveWriteQuery.resize(26);
    hDriveWriteCounter.resize(26);
    hDriveReadQuery.resize(26);
    hDriveReadCounter.resize(26);

    // 各ドライブの種別、存在確認およびオープン
    DWORD drives = ::GetLogicalDrives();
    for (int i = 0, mask = 1; i < 26; i++, mask <<= 1) {

        if (!(drives & mask)) {
            hDriveReadQuery[i] = NULL;
            hDriveWriteQuery[i] = NULL;
            hDriveReadCounter[i] = NULL;
            hDriveWriteCounter[i] = NULL;
            continue;
        }

        CStringA letter;
        letter.Format("%c:\\", 'A' + i);
        UINT driveType = GetDriveTypeA(letter);
        LPCWSTR driveTypeText;
        switch (driveType) {
        case DRIVE_REMOVABLE:
            driveTypeText = L"Removable";   // リムーバブル
            break;
        case DRIVE_FIXED:
            driveTypeText = L"Fixed";       // HDD/SSD
            break;
        case DRIVE_REMOTE:
            driveTypeText = L"Remote";      // ネットワークドライブ
            break;
        case DRIVE_CDROM:
            driveTypeText = L"CDROM";
            break;
        default:
            driveTypeText = L"Unknown";
            break;
        }

        switch (driveType) {
        case DRIVE_FIXED:   // HDD/SSD
            break;
        default:
            Logger::d(L"Drive %c skip (%s)", 'A' + i, driveTypeText);
            hDriveReadQuery[i] = NULL;
            hDriveWriteQuery[i] = NULL;
            hDriveReadCounter[i] = NULL;
            hDriveWriteCounter[i] = NULL;
            continue;
        }

        // OpenQuery
        if (PdhOpenQuery(NULL, 0, &hDriveReadQuery[i]) != ERROR_SUCCESS) {
            Logger::d(L"DriveReadクエリーをオープンできません。%d", i);
            return false;
        }
        if (PdhOpenQuery(NULL, 0, &hDriveWriteQuery[i]) != ERROR_SUCCESS) {
            Logger::d(L"DriveWriteクエリーをオープンできません。%d", i);
            return false;
        }

        Logger::d(L"Drive %c prepared (%s)", 'A' + i, driveTypeText);
        nDrives++;
    }

    for (int i = 0; i < 26; i++) {

        if (hDriveReadQuery[i] != NULL) {
            CString query;
            query.Format(L"\\LogicalDisk(%c:)\\Disk Read Bytes/sec", 'A' + i);
            PdhAddCounter(hDriveReadQuery[i], (LPCWSTR)query, 0, &hDriveReadCounter[i]);
            PdhCollectQueryData(hDriveReadQuery[i]);

            Logger::d(L" add counter %c [%s]", 'A' + i, query);
        }

        if (hDriveWriteQuery[i] != NULL) {
            CString query;
            query.Format(L"\\LogicalDisk(%c:)\\Disk Write Bytes/sec", 'A' + i);
            PdhAddCounter(hDriveWriteQuery[i], (LPCWSTR)query, 0, &hDriveWriteCounter[i]);
            PdhCollectQueryData(hDriveWriteQuery[i]);

            Logger::d(L" add counter %c [%s]", 'A' + i, query);
        }
    }

    return true;
}

void CWorker::CollectCpuUsage(const int &nProcessors, std::vector<PDH_HQUERY> &hQuery, std::vector<PDH_HQUERY> &hCounter, PDH_FMT_COUNTERVALUE &fntValue)
{
    CpuUsage usage;
    usage.usages.resize(nProcessors + 1);

    for (int i = 0; i < nProcessors + 1; i++) {
        PdhCollectQueryData(hQuery[i]);
        PdhGetFormattedCounterValue(hCounter[i], PDH_FMT_LONG, NULL, &fntValue);
        usage.usages[i] = (float)fntValue.longValue;
    }
    cpuUsages.push_back(usage);

    while (cpuUsages.size() > (size_t)g_pIniConfig->mFps) {
        cpuUsages.erase(cpuUsages.begin());
    }
}

void CWorker::CollectDriveUsage(int nDrives, std::vector<PDH_HQUERY> &hDriveReadQuery, std::vector<PDH_HCOUNTER> &hDriveReadCounter,
    PDH_FMT_COUNTERVALUE &fntValue, std::vector<PDH_HQUERY> &hDriveWriteQuery, std::vector<PDH_HCOUNTER> &hDriveWriteCounter)
{
    // 0.1秒に1回程度で十分
    static DWORD sLastTick = 0;
    DWORD now = ::GetTickCount();
    const DWORD interval = 100; // ms
    if (now - sLastTick < interval) {
        // interval[ms] 経過していないので skip
        return;
    }
    sLastTick = now;


    DriveUsage usage;
    usage.readUsages.resize(nDrives);
    usage.writeUsages.resize(nDrives);
    usage.letters.resize(nDrives);

    int i = 0;
    for (int id = 0; id < 26; id++) {
        if (hDriveReadQuery[id] != NULL) {

            // Read
            PdhCollectQueryData(hDriveReadQuery[id]);
            PdhGetFormattedCounterValue(hDriveReadCounter[id], PDH_FMT_LONG, NULL, &fntValue);
            usage.readUsages[i] = fntValue.longValue;

            // Write
            PdhCollectQueryData(hDriveWriteQuery[id]);
            PdhGetFormattedCounterValue(hDriveWriteCounter[id], PDH_FMT_LONG, NULL, &fntValue);
            usage.writeUsages[i] = fntValue.longValue;

            usage.letters[i] = 'A' + id;

            //if (usage.readUsages[i] != 0 || usage.writeUsages[i] != 0) {
            //    Logger::d(L" collect drive %c : %ld, %ld", usage.letters[i], usage.readUsages[i], usage.writeUsages[i]);
            //}

            i++;
        }
    }
//    Logger::d(L" collect drives %d", i);
    driveUsages.push_back(usage);

    // N秒分のデータを保持する
    while (driveUsages.size() > 1000 / interval) {
        driveUsages.erase(driveUsages.begin());
    }

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

    while (traffics.size() > (size_t)g_pIniConfig->mFps) {
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
    int n = (int)cpuUsages.size();
    if (n <= 0) {
        return 0;
    }

    int nCore = (int)cpuUsages.begin()->usages.size();

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

void CWorker::GetDriveUsages(DriveUsage * out)
{
    int n = (int)driveUsages.size();

    // 0以外の最新のデータを集めて格納する
    out->letters = driveUsages.back().letters;

    int nDrive = (int)out->letters.size();

    out->readUsages.resize(nDrive);
    out->writeUsages.resize(nDrive);

    for (int i = 0; i < nDrive; i++) {

        // drive[i] について0以外のデータを最新から探す
        out->readUsages[i] = 0;
        for (auto it = driveUsages.rbegin(); it != driveUsages.rend(); it++) {
            if (it->readUsages[i] != 0) {
                out->readUsages[i] = it->readUsages[i];
                break;
            }
        }

        out->writeUsages[i] = 0;
        for (auto it = driveUsages.rbegin(); it != driveUsages.rend(); it++) {
            if (it->writeUsages[i] != 0) {
                out->writeUsages[i] = it->writeUsages[i];
                break;
            }
        }
    }
}
