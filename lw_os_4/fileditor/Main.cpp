




#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <strsafe.h>
#include <richedit.h> //why ?
#include "resource.h"


#define IDC_EDIT_TEXT        2001
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ������� ��������� �������� ����
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/*����������� ��������� WM_CREATE WM_DESTROY WM_SIZE WM_COMMAND */

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

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
POINT WindowPosition; // ��������� ����
SIZE WindowSize; // ������ ����

TCHAR FileName[MAX_PATH] = TEXT(""); // ��� �������������� ���������� �����
HANDLE hFile = INVALID_HANDLE_VALUE; // ���������� �������������� ���������� �����

CHARFORMAT cf;//��������� ��������
PARAFORMAT pf;//��������� �������
LOGFONT logFont; // ��������� ������
HFONT hFont = NULL; // ���������� ������

LPSTR lpszBufferText = NULL; // ��������� �� ����� ��� ������/������ ���������� �����
OVERLAPPED _oRead = { 0 }, _oWrite = { 0 };//why?

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpszCmdLine, int nCmdShow)
{
	
	HINSTANCE relib = LoadLibrary(TEXT("riched32.dll"));    //load the dll don't forget this   
											//and don't forget to free it (see wm_destroy) 
	if (relib == NULL)
		MessageBox(NULL, TEXT("Failed to load riched32.dll!"), TEXT("Error"), MB_ICONEXCLAMATION);
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
	
	LoadLibrary(TEXT("ComCtl32.dll"));//��� ��������� ������ �����������
	// ������� ������� ���� �� ������ ������ �������� ������
		TCHAR InitFN[MAX_PATH];//��� ����� �������������

	{
		GetModuleFileName(NULL, InitFN, MAX_PATH);

		LPTSTR str = _tcsrchr(InitFN, TEXT('.'));
		if (NULL != str) str[0] = TEXT('\0');

		StringCchCat(InitFN, MAX_PATH, TEXT(".ini"));
	}

	// ��������� ��������� ���������� �� ����� �������������
	LoadProfile(InitFN);
	
	HWND hWnd = CreateWindowEx(0, TEXT("MainWindowClass"), TEXT("Process"), WS_OVERLAPPEDWINDOW,
		WindowPosition.x, WindowPosition.y, WindowSize.cx, WindowSize.cy, NULL, NULL, hInstance, NULL);

	if (NULL == hWnd)
	{
		return -1; // ��������� ������ ����������
	}

	ShowWindow(hWnd, nCmdShow); // ���������� ������� ����
	

	   	  
	MSG  msg;
	BOOL Ret;

	for (;;)
	{
		// ���������� ������� ��������� � �������
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			OnIdle(hWnd);
		} 

		// ��������� ��������� �� �������
		Ret = GetMessage(&msg, NULL, 0, 0);
		if ( Ret == FALSE )
		{
			break; // �������� WM_QUIT, ����� �� �����
		} 
		else if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
	} 
	SaveProfile(InitFN);//���������� ����������
 
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
			MoveWindow(hwndCtl, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE); // �������� ������� ���� �����
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

		WindowPosition.x = rect.left;
		WindowPosition.y = rect.top;

		WindowSize.cx = rect.right - rect.left;
		WindowSize.cy = rect.bottom - rect.top;

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
	DWORD Styles = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_MULTILINE | ES_SAVESEL| ES_SUNKEN;

	// ������� ����� ���������� Rich Edit
	HWND hwndCtl = CreateWindowEx(0, TEXT( "RICHEDIT"), TEXT(""), Styles, 0, 0, 100, 100, hwnd, (HMENU)IDC_EDIT_TEXT, lpCreateStruct->hInstance, NULL);
	
	if (hwndCtl == NULL)
		return FALSE;
	
	// �������� ����� ����� ������ ���������� Rich Edit
	SetFocus(hwndCtl);


	// ������ ����������� �� ������ ������
	//Edit_LimitText(hwndCtl, (DWORD)-1); //If it will be return RichEdit will be block for edit and entering text

	logFont.lfCharSet = DEFAULT_CHARSET; //�������� �� ���������
	logFont.lfPitchAndFamily = DEFAULT_PITCH; //�������� �� ���������
	wcscpy_s(logFont.lfFaceName, L"Times New Roman"); //�������� � ������ �������� ������
	logFont.lfHeight = 20; //������
	logFont.lfWidth = 10; //������
	logFont.lfWeight = 40; //�������
	logFont.lfEscapement = 0; //����� ��� ��������
	
	// ������ �����
	hFont = CreateFontIndirect(&logFont);
	static HWND hEdit = GetDlgItem(hwnd, IDC_EDIT_TEXT);
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
		SendMessage(hEdit, EM_SETPARAFORMAT, 0, (LPARAM)&pf);//������������ ������
	} 
	else
	{
		FileName[0] = _T('\0');// ������� ��� �������������� ���������� �����	
		SetWindowText(hwnd, TEXT("����������"));// ����� ��������� �������� ����
	} 
	
	return TRUE;
} 






void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	static HWND hEdit = GetDlgItem(hwnd, IDC_EDIT_TEXT);
	CHOOSEFONT choosef = { sizeof(CHOOSEFONT) };
	HDC hDC;


	switch (id)
	{
	case ID_NEW_FILE: // �������
	{
		if (INVALID_HANDLE_VALUE != hFile)
		{
			
			FinishIo(&_oWrite);// ������� ���������� �������� �����/������
			CloseHandle(hFile), hFile = INVALID_HANDLE_VALUE;
		} 	
		Edit_SetText(hEdit, NULL);// ������� ����� �� ���� �����

		FileName[0] = _T('\0');
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
				SetWindowText(hwnd, FileName);// ����� ��������� �������� ����
			} 
			else
			{
				MessageBox(NULL, TEXT("�� ������� ������� ��������� ����."), NULL, MB_OK | MB_ICONERROR);
				FileName[0] = _T('\0');
				SetWindowText(hwnd, TEXT("����������"));
			} 
		}
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
		
		Edit_Undo(hEdit);// �������� �������� ��������� � ���� �����	
		SetFocus(hEdit);// ������� ����� ��������� � ���� �����
	}
	break;

	case ID_SELECT_ALL: // �������� ���
	{
		Edit_SetSel(hEdit, 0, -1);// �������� ����� � ���� �����
		SetFocus(hEdit);// ������� ����� ��������� � ���� �����
	}
	break;
#pragma region Font

	case ID_BOLD_FONT:
	{
		cf.cbSize = sizeof(cf);

		// ���������� ������ ��������
		SendMessage(hEdit, EM_GETCHARFORMAT, TRUE, (LPARAM)&cf);

		// �������� ��� ���� dwEffects, � ������� ��������
		// ����� �������� ������� ��� bold (������ ����������)
		cf.dwMask = CFM_BOLD;

		// ����������� ���, ������������ ������ ����������
		cf.dwEffects ^= CFE_BOLD;

		// �������� ������ ��������
		SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		break;
	}

	// ������������� ��� �������� ���������
	// ���������� ��������
	case ID_ITALIC_FONT:
	{
		cf.cbSize = sizeof(cf);
		SendMessage(hEdit, EM_GETCHARFORMAT,
			TRUE, (LPARAM)&cf);

		cf.dwMask = CFM_ITALIC;
		cf.dwEffects ^= CFE_ITALIC;
		SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		break;
	}


	case ID_FONT_EDIT: // �����
	{
		// ���������� ������� ������ ��������
		SendMessage(hEdit, EM_GETCHARFORMAT, TRUE, (LPARAM)&cf);
	
		memset(&choosef, 0, sizeof(choosef));
		memset(&logFont, 0, sizeof(logFont));

		choosef.hInstance = GetWindowInstance(hwnd); // ��������� ���������� ���������� ����������
		choosef.hwndOwner = hwnd; // ��������� ���������� ���� ���������
			   		 
		// ���� ���� ������ ��������� �������� ��� ������
			// �������,��������� ����� � ���������������� ����������
		logFont.lfItalic = (BOOL)(cf.dwEffects & CFE_ITALIC);
		logFont.lfUnderline = (BOOL)(cf.dwEffects & CFE_UNDERLINE);

		// ����������� ������ �� TWIPS-�� � �������.
		// ������������� ������������� ����, ����� 
		// ����������� �������������� � ��������������
		// ���������� �������� ������ ��������
		logFont.lfHeight = -cf.yHeight / 20;

		// ����� ��������, �������� �� ���������
		logFont.lfCharSet = ANSI_CHARSET;

		// �������� ��������, �������� �� ���������
		logFont.lfQuality = DEFAULT_QUALITY;

		// �������� ��������� �������
		logFont.lfPitchAndFamily = cf.bPitchAndFamily;

		// �������� ���������� ������
		lstrcpy(logFont.lfFaceName, cf.szFaceName);

		// ������������� ��� ������ � ����������� �� ����,
		// ���� ������������ ��������� ������ ������� 
		// ��� ���
		if (cf.dwEffects & CFE_BOLD)
			logFont.lfWeight = FW_BOLD;
		else
			logFont.lfWeight = FW_NORMAL;
		hDC = GetDC(hwnd);
		// ��������� ��������� ��� ������� ������ ������
		choosef.lStructSize = sizeof(choosef);
		choosef.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
		choosef.hDC = hDC;
		choosef.hwndOwner = hwnd;
		choosef.lpLogFont = &logFont;
		choosef.rgbColors = RGB(0, 0, 0);
		choosef.nFontType = SCREEN_FONTTYPE;

		// ������� �� ����� ���������� ������ ���
		// ������ ������
		if (ChooseFont(&choosef))
		{
			// ����� ������������ ��� ���� ���� dwEffects
			cf.dwMask = CFM_BOLD | CFM_FACE | CFM_ITALIC |
				CFM_UNDERLINE | CFM_SIZE | CFM_OFFSET;

			// �������������� � TWIPS-�
			cf.yHeight = -logFont.lfHeight * 20;

			// ������������� ���� dwEffects 
			cf.dwEffects = 0;
			if (logFont.lfUnderline)
				cf.dwEffects |= CFE_UNDERLINE;

			if (logFont.lfWeight == FW_BOLD)
				cf.dwEffects |= CFE_BOLD;

			if (logFont.lfItalic)
				cf.dwEffects |= CFE_ITALIC;

			// ������������� ��������� ������
			cf.bPitchAndFamily = logFont.lfPitchAndFamily;

			// ������������� �������� ���������� ������
			lstrcpy(cf.szFaceName, logFont.lfFaceName);

			// �������� ��������� ���������� ��������
			SendMessage(hEdit, EM_SETCHARFORMAT,
				SCF_SELECTION, (LPARAM)&cf);
		}

		// ����������� �������� �����������
		ReleaseDC(hwnd, hDC);

	}
	break;

#pragma endregion	
case IDM_EDCUT:
		SendMessage(hEdit, WM_CUT, 0, 0);
		break;

	case IDM_EDCOPY:
		SendMessage(hEdit, WM_COPY, 0, 0);
		break;

	case IDM_EDPASTE:
		SendMessage(hEdit, WM_PASTE, 0, 0);
		break;

	// ������������� ������������ ��������� �� ������ �������
	// ���� ������ ���������� Rich Edit
	case ID_FORMAT_PARAGRAPH_RIGHT:
		{
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_ALIGNMENT;
		pf.wAlignment = PFA_RIGHT;
		SendMessage(hEdit, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
		break;
		}

	// ��������� ��������� �������� ���������
	case ID_FORMAT_PARAGRAPH_CENTER:
	{
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_ALIGNMENT;
		pf.wAlignment = PFA_CENTER;
		SendMessage(hEdit, EM_SETPARAFORMAT, 0, (LPARAM)&pf);

		break;
	}
	// ������������� ������������ ��������� �� ����� �������
// ���� ������ ���������� Rich Edit
	case ID_FORMAT_PARAGRAPH_LEFT:
		{
			pf.cbSize = sizeof(pf);
			pf.dwMask = PFM_ALIGNMENT;
			pf.wAlignment = PFA_LEFT;

			// �������� ��� ������������ �������� ���������
			SendMessage(hEdit, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
			break;
		}
	} 

    

} // OnCommand

void LoadProfile(LPCTSTR lpFileName)
{
	// ��������� ��������� � ������ ����

	WindowPosition.x = GetPrivateProfileInt(TEXT("Window"), TEXT("X"), CW_USEDEFAULT, lpFileName);
	WindowPosition.y = GetPrivateProfileInt(TEXT("Window"), TEXT("Y"), 0, lpFileName);

	WindowSize.cx = GetPrivateProfileInt(TEXT("Window"), TEXT("Width"), CW_USEDEFAULT, lpFileName);
	WindowSize.cy = GetPrivateProfileInt(TEXT("Window"), TEXT("Height"), 600, lpFileName);
	
	//�������� ���� ������������
	/*��������*/

	if (GetPrivateProfileInt(TEXT("Paraformat"), TEXT("wAlignment"), 0, lpFileName)==3)
	{
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_ALIGNMENT;
		pf.wAlignment = PFA_CENTER;
	}
	if (GetPrivateProfileInt(TEXT("Paraformat"), TEXT("wAlignment"), 0, lpFileName) == 2)
	{
			pf.cbSize = sizeof(pf);
			pf.dwMask = PFM_ALIGNMENT;
			pf.wAlignment = PFA_RIGHT;
	}
	if (GetPrivateProfileInt(TEXT("Paraformat"), TEXT("wAlignment"), 0, lpFileName) == 1)
	{
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_ALIGNMENT;
		pf.wAlignment = PFA_LEFT;
	}
		

	/*HOW DO THIS in another way????*/
	
	/*
	pf.dwMask = GetPrivateProfileInt(TEXT("Paraformat"), TEXT("dwMask"), 0, lpFileName);
	pf.wAlignment = GetPrivateProfileInt(TEXT("Paraformat"), TEXT("wAlignment"), 0, lpFileName);
	*/

	/*
	switch ((GetPrivateProfileInt(TEXT("Paraformat"), TEXT("wAlignment"), 0, lpFileName)))
	{
		pf.cbSize = sizeof(pf);
		pf.dwMask = PFM_ALIGNMENT;
		case 1:
			pf.wAlignment = PFA_LEFT;
			break;
		case 2:
			pf.wAlignment = PFA_RIGHT;
			break;
		case 3:
			pf.wAlignment = PFA_CENTER;
			break;
	}
	*/
	// ��������� ��� ���������� �������������� ���������� �����

	GetPrivateProfileString(TEXT("File"), TEXT("Filename"), NULL, FileName, MAX_PATH, lpFileName);


} 

void SaveProfile(LPCTSTR lpFileName)
{
	TCHAR szString[10];

	// ��������� ��������� � ������ ����

	StringCchPrintf(szString, 10, TEXT("%d"), WindowPosition.x);
	WritePrivateProfileString(TEXT("Window"), TEXT("X"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), WindowPosition.y);
	WritePrivateProfileString(TEXT("Window"), TEXT("Y"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), WindowSize.cx);
	WritePrivateProfileString(TEXT("Window"), TEXT("Width"), szString, lpFileName);

	StringCchPrintf(szString, 10, TEXT("%d"), WindowSize.cy);
	WritePrivateProfileString(TEXT("Window"), TEXT("Height"), szString, lpFileName);
	
	StringCchPrintf(szString, 10, TEXT("%d"), pf.dwMask);
	WritePrivateProfileString(TEXT("Paraformat"), TEXT("dwMask"), szString, lpFileName);
	
	StringCchPrintf(szString, 10, TEXT("%d"), pf.wAlignment);
	WritePrivateProfileString(TEXT("Paraformat"), TEXT("wAlignment"), szString, lpFileName);


	// ��������� ��� ���������� �������������� ���������� �����

	WritePrivateProfileString(TEXT("File"), TEXT("Filename"), FileName, lpFileName);

	

} 

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

