#pragma once

class Logger
{
public:
	static void Setup();
	static void Close();

	static void d(LPCWSTR format, ...);

private:
	static std::wstring filename;
	static FILE* fp;

};

