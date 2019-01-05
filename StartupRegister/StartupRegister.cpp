#include "stdafx.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    LPCWSTR caption = L"CompactMeter";

    // 空の場合はキー削除、指定されていればそれをファイル名とみなして登録する
    wprintf(L"cmdline: [%s]\n", lpCmdLine);

    HKEY hKey;
    LSTATUS rval;

    rval = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (rval != ERROR_SUCCESS) {
        MessageBoxW(NULL, L"キーのオープンに失敗しました", caption, MB_OK | MB_ICONERROR);
        return 1;
    }
    wprintf(L"open: [0x%08x]\n", rval);

    LPCWSTR keyName = L"CompactMeter";
    if (wcslen(lpCmdLine) == 0) {
        // 登録解除
        rval = RegDeleteValue(hKey, keyName);
        wprintf(L"delete: [0x%08x]\n", rval);
        if (rval != ERROR_SUCCESS) {
            MessageBoxW(NULL, L"キーの削除に失敗しました", caption, MB_OK | MB_ICONERROR);
        }
        else {
            MessageBoxW(NULL, L"スタートアップを解除しました", caption, MB_OK);
        }
    }
    else {
        TCHAR fullpath[MAX_PATH];
        {
            TCHAR modulePath[MAX_PATH];
            GetModuleFileName(NULL, modulePath, MAX_PATH);

            TCHAR drive[MAX_PATH + 1], dir[MAX_PATH + 1], fname[MAX_PATH + 1], ext[MAX_PATH + 1];
            _wsplitpath_s(modulePath, drive, dir, fname, ext);

            swprintf_s(fullpath, L"%s%s%s", drive, dir, L"CompactMeter.exe");
        }

        wprintf(L"path: %s\n", fullpath);

        rval = RegSetValueExW(hKey, keyName, 0, REG_SZ, (LPBYTE)fullpath, wcslen(fullpath) * sizeof(TCHAR));
        wprintf(L"set: [0x%08x]\n", rval);
        if (rval != ERROR_SUCCESS) {
            MessageBoxW(NULL, L"キーの登録に失敗しました", caption, MB_OK | MB_ICONERROR);
        }
        else {
            MessageBoxW(NULL, L"スタートアップに登録しました", caption, MB_OK);
        }
    }

    RegCloseKey(hKey);

    return 0;
}
