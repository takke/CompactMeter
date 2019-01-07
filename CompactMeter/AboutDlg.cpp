#include "stdafx.h"
#include "AboutDlg.h"
#include "resource.h"
#include "Logger.h"
#include "prebuild.h"

extern WCHAR g_szAppTitle[];

inline bool GetModuleFileVersion(WORD version[4])
{
    TCHAR             fileName[MAX_PATH + 1];
    ::GetModuleFileName(NULL, fileName, sizeof(fileName));

    DWORD size = ::GetFileVersionInfoSize(fileName, NULL);

    std::vector<BYTE> versionBuffer;
    versionBuffer.resize(size);

    VS_FIXEDFILEINFO* pFileInfo;
    UINT              queryLen;
    if (::GetFileVersionInfo(fileName, NULL, size, &versionBuffer[0])) {
        ::VerQueryValue(&versionBuffer[0], _T("\\"), (void**)&pFileInfo, &queryLen);

        version[0] = HIWORD(pFileInfo->dwFileVersionMS);
        version[1] = LOWORD(pFileInfo->dwFileVersionMS);
        version[2] = HIWORD(pFileInfo->dwFileVersionLS);
        version[3] = LOWORD(pFileInfo->dwFileVersionLS);

        return true;
    }

    return false;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            CString appname;
            appname.Format(L"%s ", g_szAppTitle);

            WORD version[4];
            if (GetModuleFileVersion(version)) {
                // バージョン情報は [3] を使用しない
                appname.AppendFormat(L"Version %d.%d.%d", version[0], version[1], version[2]);

                // AppVeyor のビルド番号を追加
#ifdef APPVEYOR_BUILD_NUMBER_INT
                appname.AppendFormat(L" (Build #%d)", APPVEYOR_BUILD_NUMBER_INT);
#endif
            }

            SetDlgItemText(hDlg, IDC_APPNAME_STATIC, appname);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case NM_CLICK:
            PNMLINK pNMLink = (PNMLINK)lParam;
            Logger::d(L"NM_CLICK: %s", pNMLink->item.szUrl);
            ShellExecuteW(NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
            return 0L;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
