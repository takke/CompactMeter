#include "stdafx.h"
#include "AboutDlg.h"
#include "resource.h"
#include "Logger.h"
#include "prebuild.h"
#include "MyUtil.h"

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            CString appName;
            MyUtil::GetAppNameWithVersion(appName);
            SetDlgItemText(hDlg, IDC_APPNAME_STATIC, appName);
        }
        return static_cast<INT_PTR>(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return static_cast<INT_PTR>(TRUE);
        }
        break;

    case WM_NOTIFY:
        switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
        case NM_CLICK:
            PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
            Logger::d(L"NM_CLICK: %s", pNMLink->item.szUrl);
            ShellExecuteW(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
            return 0L;
        }
        break;
    }
    return static_cast<INT_PTR>(FALSE);
}
