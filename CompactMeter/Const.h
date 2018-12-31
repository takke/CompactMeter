#pragma once

const long MB = 1'000'000;
const long GB = 1'000'000'000;

constexpr auto MAX_LOADSTRING = 100;

constexpr auto PI = 3.14159265f;

// タスクトレイ関連
#define WM_NOTIFYTASKTRAYICON   (WM_USER+100)
constexpr auto TRAYICON_ID = 0;

// 設定画面の設定値変更通知
#define WM_CONFIG_DLG_UPDATED   (WM_USER+200)
