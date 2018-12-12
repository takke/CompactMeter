// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、
// または、参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#include <atlstr.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// Windows ヘッダー ファイル
#include <windows.h>

#include <Gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")

#include <pdh.h>    
#pragma comment(lib, "pdh.lib")

// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>

// プログラムに必要な追加ヘッダーをここで参照してください
