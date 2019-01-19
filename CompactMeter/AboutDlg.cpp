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
            CString appname;
            MyUtil::GetAppNameWithVersion(appname);
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
