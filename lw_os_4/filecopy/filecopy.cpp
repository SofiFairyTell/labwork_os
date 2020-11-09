#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <locale.h>
#include <strsafe.h>
#include <iostream>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

typedef BOOL(__stdcall *LPSEARCHFUNC)(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);

// �������, ������������ ��� ����������� ������ � ���������
BOOL __stdcall FileCopy(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);

// �������, ����������� �������� � ������ ��� ���������
BOOL FileOperation(LPCTSTR lpszFileName, LPCTSTR lpTargetDirectory, LPSEARCHFUNC lpFunc);

// �������, ������� ��������� ����� ������ ��������
BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR lpszDirName, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam);

int _tmain(int argc, LPTSTR argv[])
{
	setlocale(LC_ALL, "");
	if ((4 == argc) && (_tcscmp(argv[1], TEXT("/copy")) == 0)) // ����������� ������ � ���������
	{
		std::cout << "> �����������" << argv[2] << " � " << argv[3] << "\n";

		BOOL bRet = FileOperation(argv[2], argv[3], FileCopy);

		if (FALSE != bRet)
			std::cout << "> �������" << "\n";
		else
			std::cout << "> ��� ������"<< GetLastError()<<"\n";
	} 
	else
	{
		std::cout<<"������ �����: /copy <��� �����> <�������>\n\n\ \t<��� �����> - ��� ����������� ����� ��� ��������\n\n\t<�������> - �������, � ������� ����� ����������� ���� ��� �������\n\n";
	} 
} 


BOOL __stdcall FileCopy(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam)
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
			bRet = FileSearch(TEXT("*"), lpszFileName, FileCopy, szNewFileName);
		} // if

		return bRet;
	} // if

	// �������� ����
	return CopyFile(lpszFileName, szNewFileName, FALSE);
} // FileCopy

// ----------------------------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------------------------
BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR lpszDirName, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam)
{
	WIN32_FIND_DATA fd;
	TCHAR szFileName[MAX_PATH];

	// ��������� ������ ������
	StringCchPrintf(szFileName, MAX_PATH, TEXT("%s\\%s"), lpszDirName, lpszFileName);

	// �������� �����
	HANDLE hFindFile = FindFirstFile(szFileName, &fd);
	if (INVALID_HANDLE_VALUE == hFindFile) return FALSE;

	BOOL bRet = TRUE;

	for (BOOL bFindNext = TRUE; FALSE != bFindNext; bFindNext = FindNextFile(hFindFile, &fd))
	{
		if (_tcscmp(fd.cFileName, TEXT(".")) == 0 || _tcscmp(fd.cFileName, TEXT("..")) == 0)
		{
			/* ���������� ������� � ������������ ������� */
			continue;
		} // if

		// ��������� ������ ���� � �����
		StringCchPrintf(szFileName, MAX_PATH, TEXT("%s\\%s"), lpszDirName, fd.cFileName);

		bRet = lpSearchFunc(szFileName, (LPWIN32_FILE_ATTRIBUTE_DATA)&fd, lpvParam);
		if (FALSE == bRet) break; // ��������� �����
	} // for

	FindClose(hFindFile); // ��������� �����
	return bRet;
} // FileSearch