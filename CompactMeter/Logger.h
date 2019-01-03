#pragma once

class Logger
{
public:
    static void Setup(LPCTSTR filename);
    static void Close();

    static void d(LPCWSTR format, ...);

private:
    static std::wstring fullpath;
    static FILE* fp;

};

