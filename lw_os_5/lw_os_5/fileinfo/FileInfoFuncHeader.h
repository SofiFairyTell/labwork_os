#pragma once
#include <Windows.h>
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
#include "resource.h"
#include <AccCtrl.h>
#include <aclapi.h>//GetExplicitEntriesFromAclA function (aclapi.h)
#include <sddl.h>//for ConvertSidToStringSidW




#pragma warning(disable : 4996) //��������� ������ deprecate. ���������, ����� ������������ ���������� ������
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "shlwapi.lib")
// ������� ��������� �������� ����
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/*����������� ��������� WM_CREATE WM_DESTROY WM_SIZE WM_COMMAND */

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

/*������� �������*/
BOOL GetFileTimeFormat(const LPFILETIME lpFileTime, LPTSTR lpszFileTime, DWORD cchFileTime);

/*������� �����*/
void ConvertFileSize(LPTSTR lpszBuffer, DWORD cch, LARGE_INTEGER size);
void ConvertDirectSize(LPTSTR lpszBuffer, DWORD cch, ULARGE_INTEGER size);

/*������� ������ �����*/
BOOL __stdcall CalculateSize(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);
typedef BOOL(__stdcall *LPSEARCHFUNC)(LPCTSTR lpszFileName, const LPWIN32_FILE_ATTRIBUTE_DATA lpFileAttributeData, LPVOID lpvParam);
BOOL FileSearch(LPCTSTR lpszFileName, LPCTSTR path, LPSEARCHFUNC lpSearchFunc, LPVOID lpvParam);


/*������� � ListView*/
BOOL ListViewInit(LPTSTR path, HWND hwnd);
/*Var*/
RECT rect = { 0 }; // ������ � ��������� ����

HANDLE hFile = INVALID_HANDLE_VALUE; // ���������� �������������� ���������� �����
LPCTSTR lpszFileName = NULL; // ��������� �� ��� �����/��������
DWORD cchPath = 0; // ����� ���� � �����/��������
HKEY hKey = NULL; // ���������� ����� ������

/**/
PSECURITY_DESCRIPTOR Sec_Descriptor;//���������� ������������
TCHAR FileName[MAX_PATH] = TEXT(""); // ���� �� �������������� �����/�����

/*�������*/
BOOL GetFileSecurityDescriptor(LPCWSTR lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR *ppSD);
BOOL GetItemFromDACL(PSECURITY_DESCRIPTOR Sec_Descriptor, PULONG pcCountOfEntries, PEXPLICIT_ACCESS *pListOfEntries);
BOOL GetAccountName_W(PSID psid, LPWSTR* AccountName);
BOOL GetAccountSID_W(LPCWSTR AccountName, PSID *ppsid); //������ SID �� ��� USER
BOOL GetOwnerName_W(PSECURITY_DESCRIPTOR Sec_Descriptor, LPWSTR *OwnerName);
BOOL SetFileSecurityInfo(LPCTSTR FileName, LPWSTR NewOwner, ULONG CountOfEntries, PEXPLICIT_ACCESS pListOfEntries, BOOL bMergeEntries);