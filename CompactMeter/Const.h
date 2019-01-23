#pragma once

const long MB = 1'000'000;
const long GB = 1'000'000'000;

constexpr auto MAX_LOADSTRING = 100;

constexpr auto PI = 3.14159265f;

// タスクトレイ関連
#define WM_NOTIFYTASKTRAYICON   (WM_USER+100)
constexpr auto TRAYICON_ID = 0;

// 画面のサイズ更新要求
#define WM_UPDATE_METER_WINDOW_SIZE (WM_USER+101)

// 設定画面の設定値変更通知
#define WM_CONFIG_DLG_UPDATED   (WM_USER+200)

// メイン画面のサイズ
const auto MAIN_WINDOW_MIN_WIDTH = 200;
const auto MAIN_WINDOW_MIN_HEIGHT = 200;
