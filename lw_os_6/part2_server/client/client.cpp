#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <strsafe.h>
#include <locale.h>

#pragma pack(push, 1)

struct REQUEST
{
	DWORD PID;
	DWORD index;
}; 

#pragma pack(pop)

// ------------------------------------------------------------------------------------------------
int _tmain(int argc, LPCTSTR argv[])
{
	_tsetlocale(LC_ALL, TEXT(""));

	_tprintf(TEXT("> ��������� ��������� ������:\n"));
	_tprintf(TEXT("> \n"));

	for (int i = 0; i < argc; ++i)
	{
		_tprintf(TEXT("> %s\n"), argv[i]);
	} 

	_tprintf(TEXT("\n"));

	if (2 == argc)
	{
	
		if (_tcsicmp(argv[1], TEXT("/pipe")) == 0) // ����������� ������
		{
			REQUEST Request; // ������

			Request.PID = GetCurrentProcessId();
			Request.index = 0;

			DWORD nBytes;
			TCHAR Response[100]; // �����

			for (;;)
			{
				// ���������� ������ � ������� �����
				BOOL bRet = CallNamedPipe(TEXT("\\\\.\\pipe\\test_pipe"),
					&Request, sizeof(Request), Response, sizeof(Response), &nBytes, NMPWAIT_WAIT_FOREVER);

				if (FALSE != bRet)
				{
					if (_T('\0') == Response[0])
					{
						break; // ������� �� �����
					} 
					_tprintf(TEXT("> %d %s\n"), (Request.index + 1), Response);
					Sleep(200);
				} 
				else
				{
					_tprintf(TEXT("> ������: %d\n"), GetLastError());
					break;
				} 
				++Request.index;
			} 
		} 
		
	} 
} 