#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>

#include <WindowsX.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <strsafe.h>
#include <time.h>//��� ctime
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>//for browserinfo
#include <fileapi.h>
#include <string>
#include <wchar.h>

using namespace std;
#pragma warning(disable : 4996) //��������� ������ deprecate. ���������, ����� ������������ ���������� ������
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "shlwapi.lib")


typedef BOOL(__stdcall *LPSEARCHFUNC)(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);
BOOL Copy(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);
BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR path, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam);
BOOL FileOperation(LPCTSTR lpszFileName, LPCTSTR lpTargetDirectory, LPSEARCHFUNC lpFunc);

//indir - ������ ����������
//outdir - ���� ����������

BOOL Copy(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam)
{
	LPCTSTR lpTargetDirectory = (LPCTSTR)lpvParam; // �������, � ������� ����� ����������� ����/�������

	TCHAR szNewFileName[MAX_PATH]; // ����� ��� �����/��������
	StringCchPrintf(szNewFileName, _countof(szNewFileName), TEXT("%s\\%s"), (LPCTSTR)lpTargetDirectory, PathFindFileName(lpszFileName));

	if (lpFileAttributeData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // ����� ����������� �������
	{
		// ������ ����� ������� (�������� ����������)
		BOOL bRet = CreateDirectoryEx(lpszFileName, szNewFileName, NULL);

		if ((FALSE != bRet) || (GetLastError() == ERROR_ALREADY_EXISTS)) // (!) ������: ���������� ������� �������, ��� ��� �� ��� ����������. 
		{
			// ��������� ����� ������ ��������
			bRet = FileSearch(TEXT("*"), lpszFileName, Copy, szNewFileName);
		} // if

		return bRet;
	} // if

	// �������� ����
	return CopyFile(lpszFileName, szNewFileName, FALSE);
}

BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR path, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;

	TCHAR szDir[MAX_PATH];

	HANDLE hFind = INVALID_HANDLE_VALUE;
	StringCchCopy(szDir, MAX_PATH, path);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));
	hFind = FindFirstFile(szDir, &ffd);

	BOOL bRet = TRUE;

	if (INVALID_HANDLE_VALUE == hFind)
	{
		//error
	}
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			bRet = lpSearchFunc(szDir, (LPWIN32_FILE_ATTRIBUTE_DATA)&ffd, lpvParam);
			if (FALSE == bRet) break; // ��������� �����
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
	return TRUE;
}

BOOL FileOperation(LPCTSTR lpszFileName, LPCTSTR lpTargetDirectory, LPSEARCHFUNC lpFunc)
{
	if (NULL != lpTargetDirectory)
	{
		DWORD dwFileAttributes = GetFileAttributes(lpTargetDirectory);

		if (INVALID_FILE_ATTRIBUTES == dwFileAttributes) // (!) ������
		{
			return FALSE;
		} // if
		else if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) // (!) �� �������� ���������
		{
			SetLastError(ERROR_PATH_NOT_FOUND);
			return FALSE;
		} // if
	} // if

	WIN32_FILE_ATTRIBUTE_DATA fad;

	// ������� ������������ ���������� �����/��������
	BOOL bRet = GetFileAttributesEx(lpszFileName, GetFileExInfoStandard, &fad);

	if (FALSE != bRet)
	{
		// ��������� �������� � ������/���������
		bRet = lpFunc(lpszFileName, &fad, (LPVOID)lpTargetDirectory);
	} // if

	return bRet;
} // FileOperation


int _tmain(int argc, LPTSTR argv[])
{
	_tsetlocale(LC_ALL, TEXT(""));

	// if ((4 == argc) && (_tcscmp(argv[1], TEXT("/copy")) == 0)) // ����������� ������ � ���������
	//{
	//	_tprintf(TEXT("> ����������� \"%s\" � ������� \"%s\\\"\n"), argv[2], argv[3]);
	//	// �������� ����/�������
		BOOL bRet = FileOperation(L"C:\\test\\tests.txt", L"C:\\test\\libr", Copy);

		if (FALSE != bRet) _tprintf(TEXT("> �������!\n"));
		else _tprintf(TEXT("> ������: %d\n"), GetLastError());
	//} // if

} // _tmain



