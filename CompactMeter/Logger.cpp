﻿#include "stdafx.h"
#include "Logger.h"
#include "MyUtil.h"

std::wstring Logger::fullpath = L"";
FILE* Logger::fp = NULL;

void Logger::Setup(LPCTSTR filename)
{
    CString directoryPath = MyUtil::GetModuleDirectoryPath();

    fullpath = directoryPath + L"\\" + filename;

    _wfopen_s(&fp, fullpath.c_str(), L"a, ccs=UTF-8");
}

void Logger::Close()
{
    if (fp != NULL) {

        fclose(fp);
    }
}

void Logger::d(LPCWSTR format, ...)
{
    if( fp == NULL ) {
        return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    CString s;
    va_list arglist;
    va_start(arglist, format);
    s.FormatV(format, arglist);
    va_end(arglist);


    // "2018-12-12 06:39:14.147 hoge\n"
    CString value;
    value.Format(L"%d-%02d-%02d %02d:%02d:%02d.%03d %s\n",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        (LPCWSTR)s);

    fwprintf(fp, L"%s", (LPCWSTR)value);

#ifdef _DEBUG_CONSOLE
    CStringA ansi(value);
    printf("%s", (LPCSTR)ansi);
#endif

    fflush(fp);
}
