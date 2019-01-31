#pragma once

extern WCHAR g_szAppTitle[];

class MyUtil
{
public:
    static CString GetModuleDirectoryPath();

    static int GetMeterCount();
    static int CalcMeterWindowHeight(int width);

    inline static COLORREF direct2DColorToRGB(COLORREF c) {
        return RGB((c & (0xff << 16U)) >> 16U, (c & (0xff << 8U)) >> 8U, c & 0xff);
    }

    inline static bool GetModuleFileVersion(WORD version[4])
    {
        TCHAR             fileName[MAX_PATH + 1];
        ::GetModuleFileName(nullptr, fileName, sizeof(fileName));

        const DWORD size = ::GetFileVersionInfoSize(fileName, nullptr);

        std::vector<BYTE> versionBuffer;
        versionBuffer.resize(size);

        VS_FIXEDFILEINFO* pFileInfo;
        UINT              queryLen;
        if (::GetFileVersionInfo(fileName, NULL, size, &versionBuffer[0])) {
            ::VerQueryValue(&versionBuffer[0], _T("\\"), reinterpret_cast<void**>(&pFileInfo), &queryLen);

            version[0] = HIWORD(pFileInfo->dwFileVersionMS);
            version[1] = LOWORD(pFileInfo->dwFileVersionMS);
            version[2] = HIWORD(pFileInfo->dwFileVersionLS);
            version[3] = LOWORD(pFileInfo->dwFileVersionLS);

            return true;
        }

        return false;
    }

    inline static void GetAppNameWithVersion(CString& appName) {

        appName.Format(L"%s ", g_szAppTitle);

        WORD version[4];
        if (GetModuleFileVersion(version)) {
            // バージョン情報は [3] を使用しない
            appName.AppendFormat(L"Version %d.%d.%d", version[0], version[1], version[2]);

            // AppVeyor のビルド番号を追加
#ifdef APPVEYOR_BUILD_NUMBER_INT
            appName.AppendFormat(L" (Build #%d)", APPVEYOR_BUILD_NUMBER_INT);
#endif
        }

    }
};

