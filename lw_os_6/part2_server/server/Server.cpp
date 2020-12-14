#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <process.h>

// ������ ��� ������
LPCTSTR gSvcName = TEXT("SampleWinService");
// ������ ������������ ��� ������
LPCTSTR gSvcDisplayName = TEXT("������ ������ Windows");


HANDLE hPipe = INVALID_HANDLE_VALUE; // ���������� ������


HANDLE hStopper = NULL; // ������� ��� ���������� ������ ������

HANDLE hThreads[3]; // ����������� ��������� �������

// ������� ������, ��� �������������� ������� �� ������
unsigned __stdcall ThreadFuncPipe(void *lpParameter);

// ------------------------------------------------------------------------------------------------
BOOL OnSvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	SECURITY_DESCRIPTOR sd; // ���������� ������������

	// �������������� ���������� ������������
	BOOL bRet = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

	if (FALSE != bRet)
		bRet = SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

	if (FALSE != bRet)
	{
		SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
		sa.lpSecurityDescriptor = &sd;

		// /// //

		// ������ �����
		hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\SamplePipe"),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // ���������, ��� ����� �������� ��� ������ � ������ ������
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // ��������� ����� ������ ������
			PIPE_UNLIMITED_INSTANCES, 0, 0, 0, &sa);

		if (INVALID_HANDLE_VALUE == hPipe)
		{
			// �� ������� ������� �����
			return FALSE;
		} // if
	
	} // if

	return bRet;
} // OnSvcInit

void OnSvcStop(void)
{
	// ��������� ������ ��������� �������
	SetEvent(hStopper);
} // OnSvcStop
// ------------------------------------------------------------------------------------------------
DWORD SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// ������ ������� ��� ���������� ������ ������
	hStopper = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

	// ������ �����, � ������� ����� ������������ ������� �� ������
	hThreads[0] = (HANDLE)_beginthreadex(NULL, 0, ThreadFuncPipe, NULL, 0, NULL);


	// ������� ���������� ��������� �������
	WaitForMultipleObjects(_countof(hThreads), hThreads, TRUE, INFINITE);

	// ��������� ���������� �������
	CloseHandle(hStopper);

	return 0;
} // SvcMain

// ------------------------------------------------------------------------------------------------

// ������ ���������
constexpr LPCTSTR szStudents[] = {
	TEXT("������ ������� ���������� ��-31"),
	TEXT("�������� ��������� ��������� ��-31"),
	TEXT("������� ����� ������������ ��-31"),
	TEXT("�������� ���� ��������� ��-31"),
	TEXT("�������� ���� ������������� ��-31"),
	TEXT("������� ��������� ������������ ��-31"),
	TEXT("������� ��������� �������� ��-31"),
	TEXT("������ �������� ����������� ��-31"),
	TEXT("����� ����� ������� ��-31"),
	TEXT("������� ������� �������� ��-31"),
	TEXT("��������� ��� ���������� ��-31"),
	TEXT("�������� ��������� ��������� ��-31"),
	TEXT("��������� ��������� ��������� ��-31"),
	TEXT("�������� ������� ������������ ��-31"),
	TEXT("���� ����� ��������� ��-31"),
	TEXT("�������� ����� ����������� ��-31"),
	TEXT("�������� ����� ������������ ��-31"),
	TEXT("�������� ������ ���������� ��-31"),
	TEXT("������� ������ ��������� ��-31"),
	TEXT("������ ������ ��������� ��-31"),
	TEXT("������� �������� ���������� ��-31"),
	TEXT("��������� ����� ������������� ��-31"),
	TEXT("��������� ������� ��������� ��-31"),
	TEXT("�������� ����� �������� ��-31"),
	TEXT("��������� ���� �������� ��-31"),
	TEXT("��������� �������� ���������� ��-31"),
	TEXT("����������� ����� �������� ��-31"),
	TEXT("���������� ������� ������� ��-31"),
	TEXT("�������� ������ ���������� ��-31"),
	TEXT("�������� ����� ������������� ��-31"),
	TEXT("��������� ������� �������� ��-31"),
	TEXT("�������� ����� ������������ ��-31"),
	TEXT("������ ������ ������������ ��-31")
};

#pragma pack(push, 1)

struct REQUEST
{
	DWORD dwProcessId;
	DWORD dwIndex;
}; // struct REQUEST

#pragma pack(pop)


// ------------------------------------------------------------------------------------------------
unsigned __stdcall ThreadFuncPipe(void *lpParameter)
{
	// ������ ������� ��� ������������ �������� � �������
	HANDLE hPipeEvent = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);

	// ������ �������
	HANDLE hEvents[2] = { hStopper, hPipeEvent };

	for (;;)
	{
		// �������������� ��������� OVERLAPPED ...
		OVERLAPPED oConnect = { 0 };
		oConnect.hEvent = hPipeEvent;
		// ������� �������-������ � �������� � ��� ����������
		ConnectNamedPipe(hPipe, &oConnect);

		// ������� ���� �� ���� �������
		DWORD dwResult = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

		if ((WAIT_OBJECT_0 == dwResult) || (ERROR_SUCCESS != oConnect.Internal))
		{
			break; // (!) ������� �� �����
		} // if

		for (;;)
		{
			REQUEST Request; // ������
			DWORD nBytes;

			// ������ ������ �� ������
			BOOL bRet = ReadFile(hPipe, &Request, sizeof(Request), &nBytes, NULL);
			if (FALSE == bRet) break; // (!) ������: ������� �� �����

			TCHAR szResponse[100] = TEXT(""); // �����

			if (Request.dwIndex < _countof(szStudents))
			{
				StringCchCopy(szResponse, _countof(szResponse), szStudents[Request.dwIndex]);
			} // if

			// ������ ������ � �������� �����
			WriteFile(hPipe, szResponse, sizeof(szResponse), &nBytes, NULL);
		} // for

		DisconnectNamedPipe(hPipe); // ��������� ����������
	} // for

	// ��������� ���������� �������
	CloseHandle(hPipeEvent);

	// ��������� ���������� ������
	CloseHandle(hPipe);

	return 0;
} // ThreadFuncPipe

