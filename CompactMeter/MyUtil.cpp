#include "stdafx.h"
#include "MyUtil.h"

CString MyUtil::GetModuleDirectoryPath()
{
	TCHAR modulePath[MAX_PATH];
	GetModuleFileName(NULL, modulePath, MAX_PATH);

	TCHAR drive[MAX_PATH + 1], dir[MAX_PATH + 1], fname[MAX_PATH + 1], ext[MAX_PATH + 1];
	_wsplitpath_s(modulePath, drive, dir, fname, ext);

	CString directoryPath;
	directoryPath.Format(L"%s%s", drive, dir);
	return directoryPath;
}
