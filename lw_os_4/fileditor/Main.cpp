#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"


#define IDC_EDIT_TEXT        2001
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ������� ��������� �������� ����
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/*����������� ��������� WM_CREATE WM_DESTROY WM_SIZE WM_COMMAND */

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnDestroy(HWND hwnd);
void OnSize(HWND hwnd, UINT state, int cx, int cy);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void OnClose(HWND hwnd);

/*������ � ������� �������������*/
void LoadProfile(LPCTSTR lpFileName);//�������� ����������
void SaveProfile(LPCTSTR lpFileName);//���������� ����������

/*����������� �������� ����/������*/
BOOL OpenFileAsync(HWND hwndCtl); //�������� � ������
BOOL SaveFileAsync(HWND hwndCtl, BOOL fSaveAs = FALSE);// �������� � ������

/*����������� �������� ������/������*/
BOOL ReadAsync(HANDLE hFile, LPVOID lpBuffer, DWORD dwOffset, DWORD dwSize, LPOVERLAPPED lpOverlapped);//������
BOOL WriteAsync(HANDLE hFile, LPCVOID lpBuffer, DWORD dwOffset, DWORD dwSize, LPOVERLAPPED lpOverlapped);//������

/*������� �������� � �������� ����/������*/

BOOL FinishIo(LPOVERLAPPED lpOverlapped);
BOOL TryFinishIo(LPOVERLAPPED lpOverlapped);

// �������, ������� ���������� � ����� ��������� ���������,
// ���� � ������� ��� ���������
// ��� �������������
void OnIdle(HWND hwnd);

/*����������*/
POINT wndPos; // ��������� ����
SIZE wndSize; // ������ ����

TCHAR FileName[MAX_PATH] = TEXT(""); // ��� �������������� ���������� �����
HANDLE hFile = INVALID_HANDLE_VALUE; // ���������� �������������� ���������� �����

LOGFONT logFont; // ��������� ������
HFONT hFont = NULL; // ���������� ������

LPSTR lpszBufferText = NULL; // ��������� �� ����� ��� ������/������ ���������� �����
OVERLAPPED _oRead = { 0 }, _oWrite = { 0 };

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpszCmdLine, int nCmdShow)
{
	LoadLibrary(TEXT("ComCtl32.dll"));//��� ��������� ������ �����������								
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

	if (0 == RegisterClassEx(&wcex)) // ������������ �����
	{
		return -1; // ��������� ������ ����������
	}

	// ������� ������� ���� �� ������ ������ �������� ������
	HWND hWnd = CreateWindowEx(0, TEXT("MainWindowClass"), TEXT("Process"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (NULL == hWnd)
	{
		return -1; // ��������� ������ ����������
	}

	ShowWindow(hWnd, nCmdShow); // ���������� ������� ����
	
	TCHAR szIniFileName[MAX_PATH];

	{
		GetModuleFileName(NULL, szIniFileName, MAX_PATH);

		LPTSTR str = _tcsrchr(szIniFileName, TEXT('.'));
		if (NULL != str) str[0] = TEXT('\0');

		StringCchCat(szIniFileName, MAX_PATH, TEXT(".ini"));
	}

	// ��������� ��������� ���������� �� ����� �������������
	LoadProfile(szIniFileName);
	   	  
	MSG  msg;
	BOOL bRet;

	for (;;)
	{
		// ���������� ������� ��������� � �������
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			OnIdle(hWnd);
		} 

		// ��������� ��������� �� �������
		bRet = GetMessage(&msg, NULL, 0, 0);

		if (bRet == -1)
		{
			/* ��������� ������ � �������� ����� �� ����� */
		} // if
		else if (FALSE == bRet)
		{
			break; // �������� WM_QUIT, ����� �� �����
		} 
		else if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
	} 

	// ��������� ��������� ���������� � ���� �������������
	SaveProfile(szIniFileName);
 
	return (int)msg.wParam;
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

	case WM_SIZE:
	{
			HWND hwndCtl = GetDlgItem(hwnd, IDC_EDIT_TEXT);
			// �������� ������� ���� �����
			MoveWindow(hwndCtl, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
	}

		break;
	case WM_DESTROY:
	{
		if (INVALID_HANDLE_VALUE != hFile)
		{
			FinishIo(&_oWrite);	// ������� ���������� �������� �����/������
			CloseHandle(hFile), hFile = INVALID_HANDLE_VALUE;// ��������� ���������� �����
		} 
	
		if (NULL != hFont)
			DeleteObject(hFont), hFont = NULL;// ������� ��������� �����
		PostQuitMessage(0); // ���������� ��������� WM_QUIT
	}break;
	case WM_CLOSE:
		RECT rect;
		GetWindowRect(hwnd, &rect);

		wndPos.x = rect.left;
		wndPos.y = rect.top;

		wndSize.cx = rect.right - rect.left;
		wndSize.cy = rect.bottom - rect.top;

		DestroyWindow(hwnd); // ���������� ����
		break;
	} 
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
} 



















void OnIdle(HWND hwnd)
{
	if (NULL != lpszBufferText)
	{
		if (TryFinishIo(&_oRead) != FALSE) // ����������� ������ ������ �� ����� ���������
		{
			if (ERROR_SUCCESS == _oRead.Internal) // ������ ��������� �������
			{
				WORD bom = *(LPWORD)lpszBufferText; // ������ ������������������ ������

				if (0xFEFF == bom) // Unicode-����
				{
					LPWSTR lpszText = (LPWSTR)(lpszBufferText + sizeof(WORD)); // Unicode-������

					// ��������� ����� Unicode-������
					DWORD cch = (_oRead.InternalHigh - sizeof(WORD)) / sizeof(WCHAR);

					// ����� ����-������ � ����� ������
					lpszText[cch] = L'\0';
					// �������� Unicode-������ � ���� �����
					SetDlgItemTextW(hwnd, IDC_EDIT_TEXT, lpszText);
				} // if
				else // ANSI-����
				{
					// ����� ����-������ � ����� ������
					lpszBufferText[_oRead.InternalHigh] = '\0';
					// �������� ANSI-������ � ���� �����
					SetDlgItemTextA(hwnd, IDC_EDIT_TEXT, lpszBufferText);
				} // else
			} // if

			// ����������� ���������� ������
			delete[] lpszBufferText, lpszBufferText = NULL;
		} // if
		else if (TryFinishIo(&_oWrite) != FALSE) // ����������� ������ ������ � ���� ���������
		{
			if (ERROR_SUCCESS == _oWrite.Internal) // ������ ��������� �������
			{
				// �������� ������������ ������� �������� ������ � ����
				// �� ��������� ��� ��������
				FlushFileBuffers(hFile);
			} // if

			// ����������� ���������� ������
			delete[] lpszBufferText, lpszBufferText = NULL;
		} // if
	} // if
} // OnIdle



BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	// ������ ���� ����� ��� �������������� ������
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |WS_BORDER| ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_LEFT| ES_WANTRETURN;
	HWND hwndCtl = CreateWindowEx(0, TEXT("Edit"), TEXT(""), dwStyle, 0, 0, 0, 0, hwnd, (HMENU)IDC_EDIT_TEXT, lpCreateStruct->hInstance, NULL);
	
	// ������ ����������� �� ������ ������
	Edit_LimitText(hwndCtl, (DWORD)-1);

	// ������ �����
	hFont = CreateFontIndirect(&logFont);

	if (NULL != hFont)
	{
		// ������������� ����� ��� ���� �����
		SendMessage(hwndCtl, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
	} 

	// ��������� ��������� ������������� ��������� ���
	if (OpenFileAsync(hwndCtl) != FALSE)
	{
		// ����� ��������� �������� ����
		SetWindowText(hwnd, FileName);
	} 
	else
	{
		// ������� ��� �������������� ���������� �����
		FileName[0] = _T('\0');
		// ����� ��������� �������� ����
		SetWindowText(hwnd, TEXT("����������"));
	} 

	return TRUE;
} 






void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	static HWND hEdit = GetDlgItem(hwnd, IDC_EDIT_TEXT);

	switch (id)
	{
	case ID_NEW_FILE: // �������
	{
		if (INVALID_HANDLE_VALUE != hFile)
		{
			// ������� ���������� �������� �����/������
			FinishIo(&_oWrite);
			// ��������� ���������� �����
			CloseHandle(hFile), hFile = INVALID_HANDLE_VALUE;
		} // if

		// ������� ����� �� ���� �����
		Edit_SetText(hEdit, NULL);

		// ������� ��� �������������� ���������� �����
		FileName[0] = _T('\0');
		// ����� ��������� �������� ����
		SetWindowText(hwnd, TEXT("����������"));
	}
	break;

	case ID_OPEN: // �������
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
			if (OpenFileAsync(hEdit) != FALSE) // ��������� ����
			{
				// ����� ��������� �������� ����
				SetWindowText(hwnd, FileName);
			} // if
			else
			{
				MessageBox(NULL, TEXT("�� ������� ������� ��������� ����."), NULL, MB_OK | MB_ICONERROR);

				// ������� ��� �������������� ���������� �����
				FileName[0] = _T('\0');
				// ����� ��������� �������� ����
				SetWindowText(hwnd, TEXT("����������"));
			} // else
		} // if
	}
	break;

	case ID_SAVE: // ���������
		if (SaveFileAsync(hEdit) != FALSE) // ��������� ����
		{
			break;
		} 
	case ID_SAVE_AS: // ��������� ���
	{
		OPENFILENAME savefile = { sizeof(OPENFILENAME) };

		savefile.hInstance = GetWindowInstance(hwnd);
		savefile.lpstrFilter = TEXT("��������� ��������� (*.txt)\0*.txt\0");
		savefile.lpstrFile = FileName;
		savefile.nMaxFile = _countof(FileName);
		savefile.lpstrTitle = TEXT("��������� ���");
		savefile.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
		savefile.lpstrDefExt = TEXT("txt");

		if (GetSaveFileName(&savefile) != FALSE)
		{
			if (SaveFileAsync(hEdit, TRUE) != FALSE) // ������������� ����
			{		
				SetWindowText(hwnd, FileName);// ����� ��������� �������� ����
			}
			else
			{
				MessageBox(NULL, TEXT("�� ������� ��������� ��������� ����."), NULL, MB_OK | MB_ICONERROR);
			}
		} 
	}
	break;

	case ID_EXIT:
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		break;

	case ID_UNDO: // ��������
	{
		// �������� �������� ��������� � ���� �����
		Edit_Undo(hEdit);
		// ������� ����� ��������� � ���� �����
		SetFocus(hEdit);
	}
	break;

	case ID_SELECT_ALL: // �������� ���
	{
		Edit_SetSel(hEdit, 0, -1);// �������� ����� � ���� �����
		// ������� ����� ��������� � ���� �����
		SetFocus(hEdit);
	}
	break;

	case ID_FONT_EDIT: // �����
	{
		CHOOSEFONT choosef = { sizeof(CHOOSEFONT) };

		choosef.hInstance = GetWindowInstance(hwnd); // ��������� ���������� ���������� ����������
		choosef.hwndOwner = hwnd; // ��������� ���������� ���� ���������

		LOGFONT lf; //��� ����������� ����������� ������
		ZeroMemory(&lf, sizeof(lf));

		choosef.lpLogFont = &lf; // ��������� ���������, ������� ����� �������������� ��� �������� ������

		if (ChooseFont(&choosef) != FALSE)
		{
			// ������ ����� �����
			HFONT hNewFont = CreateFontIndirect(choosef.lpLogFont);

			if (NULL != hNewFont)
			{
				// ������� ��������� ����� �����
				if (NULL != hFont) DeleteObject(hFont);
				// ������������� ����� ��� ���� �����
				hFont = hNewFont;
				SendDlgItemMessage(hwnd, IDC_EDIT_TEXT, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

				// �������� ��������� ������
				CopyMemory(&logFont, choosef.lpLogFont, sizeof(LOGFONT));
			} 
		} 
	}
	break;
	case IDM_EDCUT:
		SendMessage(hEdit, WM_CUT, 0, 0);
		break;

	case IDM_EDCOPY:
		SendMessage(hEdit, WM_COPY, 0, 0);
		break;

	case IDM_EDPASTE:
		SendMessage(hEdit, WM_PASTE, 0, 0);
		break;
	} 
} // OnCommand

void LoadProfile(LPCTSTR lpFileName)
{
	// ��������� ��������� � ������ ����

	wndPos.x = GetPrivateProfileInt(TEXT("Window"), TEXT("X"), CW_USEDEFAULT, lpFileName);
	wndPos.y = GetPrivateProfileInt(TEXT("Window"), TEXT("Y"), 0, lpFileName);

	wndSize.cx = GetPrivateProfileInt(TEXT("Window"), TEXT("Width"), CW_USEDEFAULT, lpFileName);
	wndSize.cy = GetPrivateProfileInt(TEXT("Window"), TEXT("Height"), 600, lpFileName);

	// ��������� ��� ���������� �������������� ���������� �����

	GetPrivateProfileString(TEXT("File"), TEXT("Filename"), NULL, FileName, MAX_PATH, lpFileName);

	// ��������� ��������� ������

	logFont.lfHeight = (LONG)GetPrivateProfileInt(TEXT("Font"), TEXT("lfHeight"), 0, lpFileName);
	logFont.lfWidth = (LONG)GetPrivateProfileInt(TEXT("Font"), TEXT("lfWidth"), 0, lpFileName);
	logFont.lfEscapement = (LONG)GetPrivateProfileInt(TEXT("Font"), TEXT("lfEscapement"), 0, lpFileName);
	logFont.lfOrientation = (LONG)GetPrivateProfileInt(TEXT("Font"), TEXT("lfOrientation"), 0, lpFileName);
	logFont.lfWeight = (LONG)GetPrivateProfileInt(TEXT("Font"), TEXT("lfWeight"), 0, lpFileName);
	logFont.lfItalic = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfItalic"), 0, lpFileName);
	logFont.lfUnderline = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfUnderline"), 0, lpFileName);
	logFont.lfStrikeOut = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfStrikeOut"), 0, lpFileName);
	logFont.lfCharSet = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfCharSet"), 0, lpFileName);
	logFont.lfOutPrecision = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfOutPrecision"), 0, lpFileName);
	logFont.lfClipPrecision = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfClipPrecision"), 0, lpFileName);
	logFont.lfQuality = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfQuality"), 0, lpFileName);
	logFont.lfPitchAndFamily = (BYTE)GetPrivateProfileInt(TEXT("Font"), TEXT("lfPitchAndFamily"), 0, lpFileName);

	GetPrivateProfileString(TEXT("Font"), TEXT("lfFaceName"), NULL, logFont.lfFaceName, LF_FACESIZE, lpFileName);
} // LoadProfile

void SaveProfile(LPCTSTR lpFileName)
{
	TCHAR szString[10];

	// ��������� ��������� � ������ ����

	StringCchPrintf(szString, 10, TEXT("%d"), wndPos.x);
	WritePrivateProfileString(TEXT("Window"), TEXT("X"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), wndPos.y);
	WritePrivateProfileString(TEXT("Window"), TEXT("Y"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), wndSize.cx);
	WritePrivateProfileString(TEXT("Window"), TEXT("Width"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), wndSize.cy);
	WritePrivateProfileString(TEXT("Window"), TEXT("Height"), szString, lpFileName);

	// ��������� ��� ���������� �������������� ���������� �����

	WritePrivateProfileString(TEXT("File"), TEXT("Filename"), FileName, lpFileName);

	// ��������� ��������� ������

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfHeight);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfHeight"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfWidth);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfWidth"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfEscapement);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfEscapement"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfOrientation);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfOrientation"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfWeight);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfWeight"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfItalic);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfItalic"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfUnderline);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfUnderline"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfStrikeOut);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfStrikeOut"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfCharSet);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfCharSet"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfOutPrecision);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfOutPrecision"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfClipPrecision);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfClipPrecision"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfQuality);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfQuality"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), logFont.lfPitchAndFamily);
	WritePrivateProfileString(TEXT("Font"), TEXT("lfPitchAndFamily"), szString, lpFileName);

	WritePrivateProfileString(TEXT("Font"), TEXT("lfFaceName"), logFont.lfFaceName, lpFileName);
} // SaveProfile

BOOL OpenFileAsync(HWND hwndCtl)
{
	// ��������� ������������ ���� ��� ������ � ������
	HANDLE hExistingFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (INVALID_HANDLE_VALUE == hExistingFile) // �� ������� ������� ����
	{
		return FALSE;
	} // if

	// ������� ����� �� ���� �����
	Edit_SetText(hwndCtl, NULL);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		// ������� ���������� �������� �����/������
		FinishIo(&_oWrite);
		// ��������� ���������� �����
		CloseHandle(hFile);
	} // if

	hFile = hExistingFile;

	// ���������� ������ ����� 
	LARGE_INTEGER size;
	BOOL bRet = GetFileSizeEx(hFile, &size);

	if ((FALSE != bRet) && (size.LowPart > 0))
	{
		// �������� ������ ��� ������, � ������� ����� ����������� ������ �� �����
		lpszBufferText = new CHAR[size.LowPart + 2];

		// �������� ����������� ������ ������ �� �����
		bRet = ReadAsync(hFile, lpszBufferText, 0, size.LowPart, &_oRead);

		if (FALSE == bRet) // �������� ������
		{
			// ����������� ���������� ������
			delete[] lpszBufferText, lpszBufferText = NULL;
		} // if
	} // if

	return bRet;
} // OpenFileAsync

BOOL SaveFileAsync(HWND hwndCtl, BOOL fSaveAs)
{
	if (FALSE != fSaveAs)
	{
		// ������ � ��������� ���� ��� ������ � ������
		HANDLE hNewFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);

		if (INVALID_HANDLE_VALUE == hNewFile) // �� ������� ������� ����
		{
			return FALSE;
		} // if

		if (INVALID_HANDLE_VALUE != hFile)
		{
			// ������� ���������� �������� �����/������
			FinishIo(&_oWrite);
			// ��������� ���������� �����
			CloseHandle(hFile);
		} // if

		hFile = hNewFile;
	} // if
	else if (INVALID_HANDLE_VALUE != hFile)
	{
		// ������� ���������� �������� �����/������
		FinishIo(&_oWrite);
	} // if
	else
	{
		// ������ � ��������� ���� ��� ������ � ������
		hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);

		if (INVALID_HANDLE_VALUE == hFile) // �� ������� ������� ����
		{
			return FALSE;
		} // if
	} // else

	// ���������� ������ ������
	LARGE_INTEGER size;
	size.QuadPart = GetWindowTextLengthA(hwndCtl);

	// �������� ��������� ��������� �����
	BOOL bRet = SetFilePointerEx(hFile, size, NULL, FILE_BEGIN);
	// ������������� ����� �����
	if (FALSE != bRet)
		bRet = SetEndOfFile(hFile);

	if ((FALSE != bRet) && (size.LowPart > 0))
	{
		// �������� ������ ��� ������, �� �������� ����� ������������ ������ � ����
		lpszBufferText = new CHAR[size.LowPart + 1];
		// �������� ANSI-������ �� ���� ����� � ������
		GetWindowTextA(hwndCtl, lpszBufferText, size.LowPart + 1);

		// �������� ����������� ������ ������ � ����
		bRet = WriteAsync(hFile, lpszBufferText, 0, size.LowPart, &_oWrite);

		if (FALSE == bRet) // �������� ������
		{
			// ����������� ���������� ������
			delete[] lpszBufferText, lpszBufferText = NULL;
		} // if
	} // if

	return bRet;
} // SaveFileAsync

/*Asynch work*/
// ----------------------------------------------------------------------------------------------
BOOL ReadAsync(HANDLE hFile, LPVOID lpBuffer, DWORD dwOffset, DWORD dwSize, LPOVERLAPPED lpOverlapped)
{
	// �������������� ��������� OVERLAPPED ...

	ZeroMemory(lpOverlapped, sizeof(OVERLAPPED));

	lpOverlapped->Offset = dwOffset;
	lpOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// �������� ����������� �������� ������ ������ �� �����
	BOOL bRet = ReadFile(hFile, lpBuffer, dwSize, NULL, lpOverlapped);

	if (FALSE == bRet && ERROR_IO_PENDING != GetLastError())
	{
		CloseHandle(lpOverlapped->hEvent), lpOverlapped->hEvent = NULL;
		return FALSE;
	} // if

	return TRUE;
} // ReadAsync

// ----------------------------------------------------------------------------------------------
BOOL WriteAsync(HANDLE hFile, LPCVOID lpBuffer, DWORD dwOffset, DWORD dwSize, LPOVERLAPPED lpOverlapped)
{
	// �������������� ��������� OVERLAPPED ...

	ZeroMemory(lpOverlapped, sizeof(OVERLAPPED));

	lpOverlapped->Offset = dwOffset;
	lpOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// �������� ����������� �������� ������ ������ � ����
	BOOL bRet = WriteFile(hFile, lpBuffer, dwSize, NULL, lpOverlapped);

	if (FALSE == bRet && ERROR_IO_PENDING != GetLastError())
	{
		CloseHandle(lpOverlapped->hEvent), lpOverlapped->hEvent = NULL;
		return FALSE;
	} // if

	return TRUE;
} // WriteAsync

// ----------------------------------------------------------------------------------------------
BOOL FinishIo(LPOVERLAPPED lpOverlapped)
{
	if (NULL != lpOverlapped->hEvent)
	{
		// ������� ���������� �������� �����/������
		DWORD dwResult = WaitForSingleObject(lpOverlapped->hEvent, INFINITE);

		if (WAIT_OBJECT_0 == dwResult) // �������� ���������
		{
			CloseHandle(lpOverlapped->hEvent), lpOverlapped->hEvent = NULL;
			return TRUE;
		} // if
	} // if

	return FALSE;
} // FinishIo

// ----------------------------------------------------------------------------------------------
BOOL TryFinishIo(LPOVERLAPPED lpOverlapped)
{
	if (NULL != lpOverlapped->hEvent)
	{
		// ���������� ��������� �������� �����/������
		DWORD dwResult = WaitForSingleObject(lpOverlapped->hEvent, 0);

		if (WAIT_OBJECT_0 == dwResult) // �������� ���������
		{
			CloseHandle(lpOverlapped->hEvent), lpOverlapped->hEvent = NULL;
			return TRUE;
		} // if
	} // if

	return FALSE;
} // TryFinishIo

