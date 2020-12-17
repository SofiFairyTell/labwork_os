#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <strsafe.h>
#include <process.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"user32.lib")
#pragma warning(disable: 4996)

#define IDC_EDIT_MESSAGES           2001
#define IDC_EDIT_TEXT               2002
#define IDC_EDIT_USERNAME			2003
#define IDC_RBUTTON_WM_SETTEXT      2004
#define IDC_RBUTTON_WM_COPYDATA     2005
#define IDC_RBUTTON_CLIPBOARD       2006
#define IDC_RBUTTON_PIPE            2007


#define MAX_MESSAGE_SIZE            255
#define MAX_USERNAME_SIZE			20

/*������*/
SOCKET sock = NULL;

HFONT hFont = NULL;

sockaddr_in sockSin = { 0 };
sockaddr_in sockSout = { 0 };
sockaddr_in sockSoin = { 0 };

void OnEnter(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
/*�����������*/
HWND hwnd = NULL; // ���������� �������� ����
HWND hDlg = NULL;
HWND hFindDlg = NULL;

HKEY hKey = NULL; // ���������� ��������� ����� ������� ��� ���������� ����
HANDLE hStopper = NULL; // ������� ��� ����������� ������ ThreadFuncClipboard � ���������� ����������
HANDLE hThread = NULL; // ����������� ��������� �������
unsigned __stdcall ThreadFunc(LPVOID lpParameter);

/*������� ���������*/
LRESULT CALLBACK MainWindowProcess(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL PreTranslateMessage(LPMSG lpMsg);//�������� ����� ��������� ��������� � ������� ���������

/*����������� WM_CREATE, WM_DESTROY, WM_SIZE*/

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
void OnDestroy(HWND hwnd);

void OnIdle(HWND hwnd);

#pragma pack(1)
struct SLP_msg
{
	int filelen;		 //����� ���������
	int numberfrag;		//����� ���������
	WCHAR username[20]; //��� �����������
	WCHAR text[10];		//����� ���������
};
#pragma pack()


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR CmdLine, int CmdShow)
{

	/*����������� �������� ������ � ��������� ������*/
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = MainWindowProcess; // ������� ���������
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 100, 256));
	wcex.lpszClassName = TEXT("MainWindowProcess"); // ��� ������
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (0 == RegisterClassEx(&wcex))
	{
		return -1;
	}
	/*---------------------------------------------*/
	LoadLibrary(TEXT("ComCtl32.dll"));//
	LoadLibrary(TEXT("Ws2_32.dll"));

	/*�������� �������� ����� � ��������� ������ */
	hwnd = CreateWindowEx(0, TEXT("MainWindowProcess"), TEXT("Chat"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 520, 520, NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		return -1;
	}
	/*--------------------------------------------------*/


	ShowWindow(hwnd, CmdShow); // ���������� ������� ����
	
	/*���� ��������� ���������*/
	MSG  msg;
	BOOL RetRes;
	for (;;)
	{
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			OnIdle(hwnd);
		}

		RetRes = GetMessage(&msg, NULL, 0, 0);

		if (RetRes == -1)
		{

		}
		else if (FALSE == RetRes)
		{
			break;
		}
		else if (PreTranslateMessage(&msg) == FALSE)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//UregisterApplication();// ������� ���������� � ���������� �� ���������� �������
	return (int)msg.wParam;
}
void OnIdle(HWND hwnd)
{
}

/*��������� �������� ����*/
LRESULT CALLBACK MainWindowProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hWnd, WM_COMMAND, OnEnter);
	} 
	return DefWindowProc(hWnd, msg, wParam, lParam);
} 
BOOL PreTranslateMessage(LPMSG lpMsg)
{
	return IsDialogMessage(hDlg, lpMsg) ||
		IsDialogMessage(hFindDlg, lpMsg);
}

/*���������� ���������*/
void OnEnter(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	/*����������*/
	WCHAR Message[MAX_MESSAGE_SIZE]; //���������
	WCHAR UserName[MAX_USERNAME_SIZE];//��� �����������
	WCHAR UserMessage[255] = _T("");

	SLP_msg msg; //����� ��� �������� ���������
	memset(msg.text, NULL, 10);
	if (VK_RETURN == codeNotify)
	{

 //}
	//DWORD symbols, symb_user;  //���������� �������� � ���������

	//if ((WM_KEYDOWN == Msg->message) && (VK_RETURN == Msg->wParam)) // ������ ������� Enter
	//{
		/*�������� ��� ������������ � ����� ���������*/
		HWND hwndCtl = GetDlgItem(hwnd, IDC_EDIT_TEXT);

		if (GetFocus() == hwndCtl) // ���� ����� �������� ������� ����������
		{
			/*����� ����� ���� ��������� ������������� �����*/
			/*CTRL+ENTER*/
			if (GetKeyState(VK_SHIFT) < 0) // ������ ������� SHIFT
			{
				Edit_ReplaceSel(hwndCtl, _T("\r\n"));
			} 
			else
			{
			GetDlgItemText(hwnd, IDC_EDIT_TEXT,Message,255);
			GetDlgItemText(hwnd, IDC_EDIT_USERNAME, UserName,20);

			StringCchCat(msg.username, _countof(UserName), UserName);
			StringCchPrintf(UserMessage, _countof(UserMessage),L"User: %s", Message); //��� ������������ +���������

			/*�������� ������?*/

			int n = 0, num; 
			int LenMessage; 
			msg.filelen = LenMessage = wcslen(Message);//����������� ����� ���������

			HWND hwndUser = GetDlgItem(hwnd, IDC_EDIT_USERNAME);
		
			int part = (LenMessage / 5);

			if (LenMessage % 5 == 0)
				num = 1;
			else
				num = 0;
				
				//symbols = Edit_GetText(hwndCtl, Message, _countof(Message));// �������� ����� �� ���� �����
				//symb_user = Edit_GetText(hwndUser, UserName, _countof(UserName));// �������� ����� �� ���� �����


			for (int i = 0; i < LenMessage; i += 5)
			{
				WCHAR frag[5] = L"";
				for (int j = 0; j < 5; j++)
				{
					frag[j] = Message[j + i];
				}
				msg.numberfrag = num;
				num++;
				StringCchPrintf(msg.text, 6, frag);
				n = sendto(sock, (const char*)&msg, sizeof(msg), NULL, (struct sockaddr*)&sockSin, sizeof(sockSin));
				ZeroMemory(msg.text, 10);
			}



				//if (symbols > 0)
				//{
				//	// ������� ���� �����
				//	Edit_SetText(hwndCtl, NULL);
				//	
				//	StringCchCat(UserName, _countof(UserName), _T(":"));
				//	StringCchCat(Message, _countof(Message), _T("\n\n"));

				//	StringCchCat(UserMessage, _countof(UserMessage), UserName);

				//	StringCchCat(UserMessage, _countof(UserMessage), Message);

				//	SendText(UserMessage, _tcslen(UserMessage), FALSE);
	
				//} 
			} 
		//	return TRUE;
		} 
	}
	//return FALSE;
} 

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	// ������ ������� ��� ����������� ������ ThreadFuncClipboard � ���������� ����������
	hStopper = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
	  

	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

	// ������ ���� ������ ���������
	CreateWindowEx(WS_EX_STATICEDGE, TEXT("Edit"), TEXT(""), dwStyle | ES_READONLY,
		10, 10, 490, 250, hwnd, (HMENU)IDC_EDIT_MESSAGES, lpCreateStruct->hInstance, NULL);
	
	//��� username
	CreateWindowEx(0, TEXT("Static"), TEXT("���: "), WS_CHILD | WS_VISIBLE | SS_SIMPLE,
		10, 270, 40, 40, hwnd, NULL, lpCreateStruct->hInstance, NULL);
	
	CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT(""), WS_CHILD | WS_VISIBLE ,
		50, 270, 450, 40, hwnd, (HMENU)IDC_EDIT_USERNAME, lpCreateStruct->hInstance, NULL);
	
	// ������ ���� �����
	HWND hwndCtl = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Edit"), TEXT(""), dwStyle,
		10, 320, 490, 140, hwnd, (HMENU)IDC_EDIT_TEXT, lpCreateStruct->hInstance, NULL);

	// ����� ����������� �� ���� ������
	Edit_LimitText(hwndCtl, MAX_MESSAGE_SIZE);

	/*������������� ������*/
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	sock = socket(AF_INET, SOCK_DGRAM, NULL); //�������� ������
	sockSout.sin_family = AF_INET; // ��������� ������� = IPv4
	sockSout.sin_port = htons(7581); // ����� ����� = 7581
	sockSout.sin_addr.s_addr = htonl(INADDR_ANY);
	err = bind(sock, (sockaddr*)&sockSout, sizeof(sockSout));

	hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, NULL, 0, NULL);


	return TRUE;
} 

void OnDestroy(HWND hwnd)
{
	/*����� �������� ���� ����������*/
	
	SetEvent(hStopper);//  ��������� ����� ThreadFuncClipboard
	WaitForSingleObject(hThread, INFINITE);//���� ��������� ��������� �������
	CloseHandle(hStopper);// ��������� ���������� ������� ��� ���������� ����������
	PostQuitMessage(0); // ���������� ��������� WM_QUIT
} 

//
//void StartChat(HWND hwnd, LPCTSTR Message)
//{
//	HWND hwndCtl = GetDlgItem(hwnd, IDC_EDIT_MESSAGES);
//	Edit_SetSel(hwndCtl, Edit_GetTextLength(hwndCtl), -1);// ������������� ������ � ����� ���� ������
//	Edit_ReplaceSel(hwndCtl, Message);// ��������� ����� � ���� ������
//} 
//
//
//BOOL RegisterApplication()
//{
//	/*����������*/
//	DWORD lpdwDisposition;	//����� ����������, ���� ����� ����������: ���� �� ���������� � ��� ������/ ���� ��� ������
//	DWORD PID;				//������������� ��������
//
//	LONG lStatus;		//��������� �������� �����
//	HKEY  SubKey_Handle = NULL;		// ���������� ���������� ����� �������, ������� ����� ��������� ������ �������� ����������
//	TCHAR SubKey_Name[20];			// ��� ���������� ����� 
//
//	// ������ � ��������� ���� ������� HKEY_CURRENT_USER\Software\\IT-311
//
//	lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\IT-311"),
//		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &lpdwDisposition);
//
//	if (lStatus == ERROR_SUCCESS)
//	{
//		PID = GetCurrentProcessId();// ������� ������������� �������� ��������
//
//		StringCchPrintf(SubKey_Name, _countof(SubKey_Name), TEXT("%d"), PID);// ��������� ��� ���������� ����� �������
//		
//		lStatus = RegCreateKey(hKey, SubKey_Name, &SubKey_Handle);// ������ ��������� ���� �������
//
//		if (ERROR_SUCCESS == lStatus)
//		{
//			RegSetValueEx(SubKey_Handle, NULL, 0, REG_DWORD, (LPBYTE)&PID, sizeof(DWORD));// ��������� ������������� �������� ��������		
//			RegSetValueEx(SubKey_Handle, TEXT("hwnd"), 0, REG_BINARY, (LPBYTE)&hwnd, sizeof(HWND));// ��������� �������� ���������� �������� ����	
//			RegCloseKey(SubKey_Handle), SubKey_Handle = NULL; // ��������� ��������� ���� �������
//			return TRUE;
//		} 
//		RegCloseKey(hKey), hKey = NULL;// ��������� ���� �������
//	} 
//	return FALSE;
//} 
//
//void UregisterApplication()
//{
//	/*����������*/
//	DWORD SubKeys_quant = 0; // ���������� ��������� ������	
//	TCHAR SubKey_Name[20]; // ��� ���������� ����� �������, ������� �������� ������ �������� ����������
//	LSTATUS lStatus; //��������� ����������� ���������� ������
//	if (hKey != NULL)
//	{
//		lStatus = RegQueryInfoKey(hKey, NULL, NULL, NULL, &SubKeys_quant, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
//
//		if ((lStatus == ERROR_SUCCESS) && (SubKeys_quant < 2))
//		{	
//			RegDeleteTree(hKey, NULL);// ������� ��� �����		
//			RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\IT-311"));// ������� ����
//		} 
//		else
//		{	
//			StringCchPrintf(SubKey_Name, _countof(SubKey_Name), TEXT("%d"), GetCurrentProcessId());	// ��������� ��� ���������� ����� �������
//			RegDeleteKey(hKey, SubKey_Name);	// ������� ��������� ����
//		} 
//		RegCloseKey(hKey), hKey = NULL;	// ��������� ���� �������
//	} 
//} 
//
//void SendText(LPCTSTR Message, DWORD Message_Size, BOOL DataCopy)
//{
//	/*-------�����������-------*/
//	
//	//�����������
//	HWND hRecvWnd; // ���������� ���� ����������
//	HKEY hSubKey = NULL; // ���������� ���������� ����� �������
//					 
//	//���  ���������� �����
//	TCHAR SubKey_Name[20]; // ��� ���������� ����� �������
//	DWORD SubKey_Name_sz; // ��� ����������� ������� ����� �����
//	DWORD SubKeys_quant = 0; // ���������� ��������� ������
//	LSTATUS lStatus; //��� ����������� � ���������� �������
//
//	DWORD lpcbData;//����� ���������� ��� ������� ������ ������
//
//	if ((Message != NULL) && (Message_Size > 0))
//	{
//		lStatus = RegQueryInfoKey(hKey, NULL, NULL, NULL, &SubKeys_quant, NULL, NULL, NULL, NULL, NULL, NULL, NULL);// ���������� ���������� ��������� ������
//
//		if ((ERROR_SUCCESS == lStatus) && (SubKeys_quant > 0))
//		{
//			//������ ������������ ������
//			for (DWORD i = 0; i < SubKeys_quant; ++i)
//			{		
//				SubKey_Name_sz = _countof(SubKey_Name);
//				lStatus = RegEnumKeyEx(hKey, i, SubKey_Name, &SubKey_Name_sz, NULL, NULL, NULL, NULL);//������� ��� ����� �� �������
//
//				if (lStatus == ERROR_SUCCESS)
//				{			
//					lStatus = RegOpenKeyEx(hKey, SubKey_Name, 0, KEY_QUERY_VALUE, &hSubKey);// ��������� ��������� ����
//				} 
//
//				if (lStatus == ERROR_SUCCESS)
//				{
//					
//					lpcbData = sizeof(HWND); 
//			
//					lStatus = RegQueryValueEx(hSubKey, TEXT("hwnd"), NULL, NULL, (LPBYTE)&hRecvWnd, &lpcbData);// �������� ���������� ���� ����������
//
//					if (lStatus == ERROR_SUCCESS)
//					{
//						SendMessage(hRecvWnd, WM_SETTEXT, 0, (LPARAM)Message);// ���������� ��������� WM_SETTEXT
//					}
//					RegCloseKey(hSubKey), hSubKey = NULL;// ��������� ��������� ���� �������
//				} 
//			} 
//		} 
//	} 
//} 
//

unsigned __stdcall ThreadFunc(LPVOID lpParameter)
{
	SLP_msg msgt = { 0 };

	int len = sizeof(sockSout);
	WCHAR text[255] = L"";
	WCHAR text1[255] = L"";
	for (;;)
	{
		int n = recvfrom(sock, (char*)&msgt, sizeof(msgt), NULL, (struct sockaddr*)&sockSout, &len);

		if (SOCKET_ERROR != n) // ��������� ������� ��������
		{
			int a = msgt.filelen / 5;
			if (msgt.filelen % 5 == 0)
				msgt.numberfrag--;
			for (int i = 0; i < 255; i++)
			{
				if (i == msgt.numberfrag * 5)
				{
					for (int j = 0; j < 5; j++)
						text[i + j] = msgt.text[j];
				}
			}
			if (msgt.filelen % 5 == 0)
				msgt.numberfrag++;
			if (a == msgt.numberfrag)
			{
				wcscat(text1, msgt.username);
				wcscat(text1, L": ");
				wcscat(text1, text);

				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)text1);
				//ListBox_AddString(hList, text1);
				memset(text, NULL, 255);
				memset(text1, NULL, 255);
			}
		}
	}

	return(0);
}
