#include "stdafx.h"
#include "MyUtil.h"
#include "Logger.h"
#include "IniConfig.h"

extern IniConfig*  g_pIniConfig;

CString MyUtil::GetModuleDirectoryPath()
{
    TCHAR modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);

    TCHAR drive[MAX_PATH + 1], dir[MAX_PATH + 1], fname[MAX_PATH + 1], ext[MAX_PATH + 1];
    _wsplitpath_s(modulePath, drive, dir, fname, ext);

    CString directoryPath;
    directoryPath.Format(L"%s%s", drive, dir);
    return directoryPath;
}

int MyUtil::GetMeterCount()
{
    int meterCount = 0;

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    const int nProcessors = info.dwNumberOfProcessors;

    DWORD drives = ::GetLogicalDrives();

    for (const auto& mc : g_pIniConfig->mMeterConfigs) {
        if (!mc.enable) {
            continue;
        }

        switch (mc.id) {
        case METER_ID_CPU:
            meterCount++;
            break;
        case METER_ID_CORES:
            meterCount += (nProcessors / 4) + ((nProcessors % 4) == 0 ? 0 : 1);
            break;
        case METER_ID_MEMORY:
            meterCount++;
            break;
        case METER_ID_NETWORK:
            meterCount += 2;
            break;
        default:
            if (METER_ID_DRIVE_A <= mc.id && mc.id <= METER_ID_DRIVE_Z) {

                int driveId = mc.id - METER_ID_DRIVE_A;
                int mask = 1 << driveId;
                CStringW letter;
                letter.Format(L"%c:\\", 'A' + driveId);
                UINT driveType = GetDriveTypeW(letter);
//                Logger::d(L"%s mask %d -> %d, drives:0x%08X", letter, driveId, mask, drives);
                if (drives && mask && driveType == DRIVE_FIXED) {
                    meterCount += 2;
                }
            }
            break;
        }
    }

    return meterCount;
}

int MyUtil::CalcMeterWindowHeight(int width)
{
    // メーターの列数およびコア数から必要な box 数を算出する
    const int meterCount = MyUtil::GetMeterCount();

    const int columnCount = g_pIniConfig->mColumnCount;
    // 必要な縦方向メーター数
    const int requireVerticalCount = meterCount / columnCount + ((meterCount % columnCount) == 0 ? 0 : 1);

    const int boxSize = width / columnCount;

    Logger::d(L" meterCount=%d, vCount=%d, boxSize=%d", meterCount, requireVerticalCount, boxSize);

    return requireVerticalCount * boxSize + 2 + (g_pIniConfig->mDebugMode ? 200 : 0);
}
