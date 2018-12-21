#include "stdafx.h"
#include "ConfigDlg.h"
#include "Logger.h"
#include "resource.h"
#include "MyInifileUtil.h"

extern HWND g_hConfigDlgWnd;
extern MyInifileUtil* g_pMyInifile;

struct TRAFFIC_MAX_COMBO_DATA {
    LPCWSTR label;
    long value;
};
TRAFFIC_MAX_COMBO_DATA TRAFFIC_MAX_COMBO_VALUES[] = {
    { L"10MB", 10 * MB },
    { L"100MB", 100 * MB},
    { L"300MB", 300 * MB},
    { L"1GB", 1 * GB},
    { L"2GB", 2 * GB},
    { NULL, 0}
};

INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        Logger::d(L"ConfigDlg, init");

        //--------------------------------------------------
        // 初期値設定
        //--------------------------------------------------

        // FPS
        SetDlgItemInt(hDlg, IDC_FPS_EDIT, g_pMyInifile->mFps, FALSE);

        // コア別メーター
        CheckDlgButton(hDlg, IDC_SHOW_CORE_METER_CHECK, g_pMyInifile->mShowCoreMeters ? BST_CHECKED : BST_UNCHECKED);

        // 最大通信量
        HWND hTraffixMaxCombo = GetDlgItem(hDlg, IDC_TRAFFIC_MAX_COMBO);
        int iSelected = 0;
        for (int i = 0; TRAFFIC_MAX_COMBO_VALUES[i].label != NULL; i++) {
            SendMessage(hTraffixMaxCombo, CB_ADDSTRING, NULL, (WPARAM)TRAFFIC_MAX_COMBO_VALUES[i].label);

            if (g_pMyInifile->mTrafficMax == TRAFFIC_MAX_COMBO_VALUES[i].value) {
                iSelected = i;
            }
        }
        SendMessage(hTraffixMaxCombo, CB_SETCURSEL, iSelected, 0);

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            Logger::d(L"ConfigDlg, close");
            EndDialog(hDlg, LOWORD(wParam));
            g_hConfigDlgWnd = NULL;
            return (INT_PTR)TRUE;

        case IDC_TRAFFIC_MAX_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hTraffixMaxCombo = GetDlgItem(hDlg, IDC_TRAFFIC_MAX_COMBO);
                int iSelected = SendMessage(hTraffixMaxCombo, CB_GETCURSEL, 0, 0);

                g_pMyInifile->mTrafficMax = TRAFFIC_MAX_COMBO_VALUES[iSelected].value;

                Logger::d(L"selchange %d -> %d", iSelected, g_pMyInifile->mTrafficMax);

                g_pMyInifile->Save();

            }
            return (INT_PTR)TRUE;

        case IDC_SHOW_CORE_METER_CHECK:
            if (HIWORD(wParam) == BN_CLICKED) {
                g_pMyInifile->mShowCoreMeters = IsDlgButtonChecked(hDlg, IDC_SHOW_CORE_METER_CHECK) == BST_CHECKED;
                g_pMyInifile->Save();
            }
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
        break;

    case WM_MOVE:
        {
            // 移動 => INIに位置を保存
            RECT rect;
            GetWindowRect(hDlg, &rect);
            g_pMyInifile->mConfigDlgPosX = rect.left;
            g_pMyInifile->mConfigDlgPosY = rect.top;
            g_pMyInifile->Save();
        }
        break;
    }
    return (INT_PTR)FALSE;
}

