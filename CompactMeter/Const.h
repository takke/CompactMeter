#pragma once

const long MB = 1000 * 1000;
const long GB = 1000 * 1000 * 1000;

#define MAX_LOADSTRING 100

#define PI 3.14159265f

// タスクトレイ関連
#define WM_NOTIFYTASKTRAYICON   (WM_USER+100)
constexpr auto TRAYICON_ID = 0;

// 設定画面の設定値変更通知
#define WM_CONFIG_DLG_UPDATED   (WM_USER+200)
