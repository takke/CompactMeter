#include "stdafx.h"
#include "ConfigDlg.h"
#include "Logger.h"
#include "resource.h"
#include "MyInifileUtil.h"

extern HWND g_hConfigDlgWnd;
extern MyInifileUtil* g_pMyInifile;

INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        Logger::d(L"ConfigDlg, init");

        //--------------------------------------------------
        // 画面初期化
        //--------------------------------------------------

        // FPS
        SetDlgItemInt(hDlg, IDC_FPS_EDIT, g_pMyInifile->mFps, FALSE);



        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            Logger::d(L"ConfigDlg, close");
            EndDialog(hDlg, LOWORD(wParam));
            g_hConfigDlgWnd = NULL;
            return (INT_PTR)TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (wParam) {
        case IDC_FPS_SPIN:
            {
                LPNMUPDOWN lpnmUpdown = (LPNMUPDOWN)lParam;
                if (lpnmUpdown->hdr.code == UDN_DELTAPOS) {

                    int n = GetDlgItemInt(hDlg, IDC_FPS_EDIT, NULL, FALSE);

                    if (lpnmUpdown->iDelta > 0) {
                        Logger::d(L"down");
                        n--;
                    }
                    else {
                        Logger::d(L"up");
                        n++;
                    }

                    g_pMyInifile->mFps = n;
                    g_pMyInifile->NormalizeFps();

                    SetDlgItemInt(hDlg, IDC_FPS_EDIT, g_pMyInifile->mFps, FALSE);
                    g_pMyInifile->Save();
                }

            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}

