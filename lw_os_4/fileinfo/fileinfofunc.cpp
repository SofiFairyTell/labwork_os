#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <strsafe.h>
#include <time.h>//��� ctime
#include <fileapi.h>
#include "resource.h"

#define IDC_EDIT_TEXT        2001

#pragma warning(disable : 4996) //��������� ������ deprecate. ���������, ����� ������������ ���������� ������
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ������� ��������� �������� ����
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/*����������� ��������� WM_CREATE WM_DESTROY WM_SIZE WM_COMMAND */

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

/*������� �������*/
BOOL GetFileTimeFormat(const LPFILETIME lpFileTime, LPTSTR lpszFileTime, DWORD cchFileTime);

/*������� �����*/
void StringCchPrintFileSize(LPTSTR lpszBuffer, DWORD cch, LARGE_INTEGER size);


TCHAR FileName[MAX_PATH] = TEXT(""); // ��� �������������� ���������� �����
HANDLE hFile = INVALID_HANDLE_VALUE; // ���������� �������������� ���������� �����


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpszCmdLine, int nCmdShow)
{
					
	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = MainWindowProc; // ������� ���������
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 2);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcex.lpszClassName = TEXT("MainWindowClass"); // ��� ������
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	LoadLibrary(TEXT("ComCtl32.dll"));//��� ��������� ������ �����������				
	
	if (0 == RegisterClassEx(&wcex)) // ������������ �����
	{
		return -1; // ��������� ������ ����������
	}
	RECT wr = { 0, 0, 500, 500 };    // set the size, but not the position

	// ������� ������� ���� �� ������ ������ �������� ������
	HWND hWnd = CreateWindowEx(0, TEXT("MainWindowClass"), TEXT("Process"), WS_OVERLAPPEDWINDOW^WS_THICKFRAME^WS_MINIMIZEBOX^WS_MAXIMIZEBOX, 300,300,
		wr.right - wr.left,   wr.bottom - wr.top, NULL, NULL, hInstance, NULL);

	if (NULL == hWnd)
	{
		return -1; // ��������� ������ ����������
	}

	ShowWindow(hWnd, nCmdShow); // ���������� ������� ����




	MSG  msg;
	BOOL Ret;

	for (;;)
	{

		// ��������� ��������� �� �������
		Ret = GetMessage(&msg, NULL, 0, 0);
		if (Ret == FALSE)
		{
			break; // �������� WM_QUIT, ����� �� �����
		}
		else if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	return (int)msg.wParam;
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
	case WM_SIZE:
	{
		HWND hwndCtl = GetDlgItem(hwnd, IDC_EDIT_TEXT);
		MoveWindow(hwndCtl, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE); // �������� ������� ���� �����
	}

	break;
	case WM_DESTROY:
	{
		PostQuitMessage(0); // ���������� ��������� WM_QUIT
	}break;
	case WM_CLOSE:

		DestroyWindow(hwnd); // ���������� ����
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}



BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCRStr)
{
	HWND hwndLV = CreateWindowEx(0, TEXT("SysListView32"), NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS , 30, 40, 400, 150, hwnd, (HMENU)IDC_LIST1, lpCRStr->hInstance, NULL);

	// ����� ����������� 
	ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// ��������� ��� ������� � ������ ���������

	LVCOLUMN lvColumns[] = {
		{ LVCF_WIDTH | LVCF_TEXT, 0, 200, (LPTSTR)TEXT("��������") },
		{ LVCF_WIDTH | LVCF_TEXT, 0, 200, (LPTSTR)TEXT("��������") },
	};

	for (int i = 0; i < _countof(lvColumns); ++i)
	{
		// ��������� �������
		ListView_InsertColumn(hwndLV, i, &lvColumns[i]);
	} 
	

	return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	static HWND hEdit = GetDlgItem(hwnd, IDC_EDIT_TEXT);

	switch (id)
	{
	case ID_OPEN_FILE: // �������
		{
			OPENFILENAME openfile = { sizeof(OPENFILENAME) };

			openfile.hInstance = GetWindowInstance(hwnd);
			openfile.lpstrFilter = TEXT("��������� ��������� (*.txt)\0*.txt\0");
			openfile.lpstrFile = FileName;
			openfile.nMaxFile = _countof(FileName);
			openfile.lpstrTitle = TEXT("�������");
			openfile.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST;
			openfile.lpstrDefExt = TEXT("txt");

			if (GetOpenFileName(&openfile) != FALSE)
			{
			HANDLE  hFile;
			BY_HANDLE_FILE_INFORMATION  bhfi;  // ���������� � �����

			TCHAR TimeBuffer[100], Buffer[100];


			// ��������� ���� ��� ������
			hFile = CreateFile(
				FileName,   // ��� �����
				0,                     // ��������� ���������� � �����
				0,                     // ����������� ������ � �����
				NULL,                  // ������ ��� 
				OPEN_EXISTING,         // ��������� ������������ ����
				FILE_ATTRIBUTE_NORMAL, // ������� ����
				NULL                   // ������� ���
			);
			// ��������� �� �������� ��������
			if (hFile == INVALID_HANDLE_VALUE)
			{
				GetLastError();
				break;
			}
			// �������� ���������� � �����
			//get info about file
			if (!GetFileInformationByHandle(hFile, &bhfi))
			{
				GetLastError();
				break;
			}
			//��������� ���������� � ������� �����
			//get info about size of file
			LARGE_INTEGER LI_Size;
			if (!GetFileSizeEx(hFile, &LI_Size))
			{
				//��������� ������
			}

			StringCchPrintFileSize(Buffer, _countof(Buffer), LI_Size);
					   		

			if (bhfi.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			{
				
			}







			/*��������, �� �������*/

			//���������� ��������� ��������� � ������ ���������

			HWND hwndLV = GetDlgItem(hwnd, IDC_LIST1);
			// ��������� ����� ������� � ������ ���������
			ListView_DeleteAllItems(hwndLV);
			LVITEM lvItem = {LVIF_TEXT | LVIF_PARAM};
			lvItem.iItem = ListView_GetItemCount(hwndLV);

			//������

			lvItem.pszText = (LPWSTR)(L"������:");
			lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
			if ((lvItem.iItem != -1))
			{
				ListView_SetItemText(hwndLV, lvItem.iItem, 1, Buffer);
			}


			//������ ��������


			lvItem.pszText = (LPWSTR)(L"����� ���������:");
			lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
			if ((lvItem.iItem != -1))
			{
				GetFileTimeFormat(&bhfi.ftCreationTime, TimeBuffer, _countof(TimeBuffer));//����� ��������
				ListView_SetItemText(hwndLV, lvItem.iItem, 1, TimeBuffer);
			}

			//������ ��������

			lvItem.pszText = (LPWSTR)(L"����� ���������� ���������:");
			lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
			if ((lvItem.iItem != -1))
			{
				GetFileTimeFormat(&bhfi.ftLastAccessTime, TimeBuffer, _countof(TimeBuffer));//����� ���������� ���������
				ListView_SetItemText(hwndLV, lvItem.iItem, 1, TimeBuffer);
			}

			//������ ��������

			lvItem.pszText = (LPWSTR)(L"����� ��������:");
			lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
			if ((lvItem.iItem != -1))
			{
				GetFileTimeFormat(&bhfi.ftLastWriteTime, TimeBuffer, _countof(TimeBuffer));//����� ���������
				ListView_SetItemText(hwndLV, lvItem.iItem, 1, TimeBuffer);
			}


			//������������

			lvItem.pszText = (LPWSTR)(L"������������:");
			lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
			if ((lvItem.iItem != -1))
			{
				ListView_SetItemText(hwndLV, lvItem.iItem, 1, FileName);
			}

			//���

			lvItem.pszText = (LPWSTR)(L"���:");
			lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
			if ((lvItem.iItem != -1))
			{
				ListView_SetItemText(hwndLV, lvItem.iItem, 1, FileName);
			}

			}


			// ��������� ���������� ����� */
			CloseHandle(hFile);






/*


			/*��������, �� �������
			if (GetOpenFileName(&openfile) != FALSE)
				{						
					HWND hwndLV = GetDlgItem(hwnd, IDC_LIST1);
					// ��������� ����� ������� � ������ ���������
					ListView_DeleteAllItems(hwndLV);	
					
					//
					LVITEM lvItem = { LVIF_TEXT | LVIF_PARAM };
					lvItem.iItem = ListView_GetItemCount(hwndLV);
					lvItem.pszText = (LPTSTR)"������� 1";
					// ��������� ����� ������� � ������ ���������
					lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);
						
					if ((lvItem.iItem != -1))
					{
						ListView_SetItemText(hwndLV, lvItem.iItem, 1, FileName);
					} 
			}
			else
				{
					MessageBox(NULL, TEXT("�� ������� ������� ��������� ����."), NULL, MB_OK | MB_ICONERROR);
					FileName[0] = _T('\0');
				}
*/			
}
	break;

	case ID_EXIT:
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		break;

	}
} 
BOOL GetFileTimeFormat(const LPFILETIME lpFileTime, LPTSTR lpszFileTime, DWORD cchFileTime)
{
	SYSTEMTIME st;

	// ����������� ���� � ����� �� FILETIME � SYSTEMTIME
	BOOL bRet = FileTimeToSystemTime(lpFileTime, &st);

	// �������� ���� � ����� � �������� �������� �����
	if (FALSE != bRet)
		bRet = SystemTimeToTzSpecificLocalTime(NULL, &st, &st);

	if (FALSE != bRet)
	{
		// ��������� ���� � �������������� ������
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, lpszFileTime, cchFileTime);

		// ������� ����� � �������������� ������

		StringCchCat(lpszFileTime, cchFileTime, TEXT(", "));
		DWORD len = _tcslen(lpszFileTime);

		if (len < cchFileTime)
			GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &st, NULL, lpszFileTime + len, cchFileTime - len);
	} // if

	return bRet;
} // GetFileTimeFormat

void StringCchPrintFileSize(LPTSTR lpszBuffer, DWORD cch, LARGE_INTEGER size)
{
	if (size.QuadPart >= 0x40000000ULL)
	{
		StringCchPrintf(lpszBuffer, cch, TEXT("%.1f ��"), (size.QuadPart / (float)0x40000000ULL));
	} // if
	else if (size.QuadPart >= 0x100000ULL)
	{
		StringCchPrintf(lpszBuffer, cch, TEXT("%.1f ��"), (size.QuadPart / (float)0x100000ULL));
	} // if
	else if (size.QuadPart >= 0x0400ULL)
	{
		StringCchPrintf(lpszBuffer, cch, TEXT("%.1f ��"), (size.QuadPart / (float)0x0400ULL));
	} // if
	else
	{
		StringCchPrintf(lpszBuffer, cch, TEXT("%u ����"), size.LowPart);
	} // else

	size_t len = _tcslen(lpszBuffer);

	if (len < cch)
	{
		StringCchPrintf((lpszBuffer + len), (cch - len), TEXT(" (%llu ����)"), size.QuadPart);
	} // if
} // StringCchPrintFileSize