/*��������� ��������� ����� ����������. 
���� ���������� �� ������. 
��������� ������� �� 10. ��������� ����������, ����� 3 ���� �� 10
������ ���� ��� ������������� - ������� (������������)*/
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>
#include <conio.h>
#include <locale.h>
#include <iostream>

// ���r���, ����������� ����������
BOOL DuplicateHandle(DWORD dwProcessId, HANDLE hSourceHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess);

// ------------------------------------------------------------------------------------------------
int _tmain(int argc, LPCTSTR argv[])
{
	setlocale(LC_ALL, "");

	if (1 == argc) // (!) ����� ������������� ��������
	{
		int n_processes;
		std::cout << "������� ���������� ���������: ";
		std::cin >> n_processes;
		std::cout << std::endl;
		HANDLE hObject = NULL;
		TCHAR szCmdLine[MAX_PATH + 40];
		// �������� ����������� ������� ���� � ������������� ������������� ��������
		// ����� ���������� � �������� ������� ��� ��������� � ��������� ������

		hObject = CreateSemaphoreEx(NULL, 1, 1, NULL, 0, SEMAPHORE_ALL_ACCESS);
		if (hObject != NULL)
		{
			// ��������� ��������� ������ ��� �������� �������� ���������
			StringCchPrintf(szCmdLine, _countof(szCmdLine), TEXT("%s semaphore-duplicate %d %p"), argv[0], (int)GetCurrentProcessId(), hObject);
		} // if

		if (hObject != NULL)
		{
			// ������� ������ ���������
			HANDLE* processes = new HANDLE[n_processes];

			// ��������� �������� ��������...

			STARTUPINFO si = { sizeof(STARTUPINFO) };
			PROCESS_INFORMATION pi = { 0 };

			for (int i = 0; i < n_processes; ++i)
			{
				// ��������� ����� �������
				BOOL bRet = CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

				if (FALSE != bRet)
				{
					// ���������� ���������� ������ �������� � ������
					processes[i] = pi.hProcess;
					// ��������� ���������� ������ ������ ��������
					CloseHandle(pi.hThread);
				} // if
				else
				{
					processes[i] = NULL;
				} // else
			} // for

			// ������� ���������� ���� �������� ���������
			WaitForMultipleObjects(n_processes, processes, TRUE, INFINITE);

			// ��������� ����������� �������� ���������
			for (int i = 0; i < n_processes; ++i) CloseHandle(processes[i]);
		
			delete[] processes;	// ������� ������ ���������
		
			CloseHandle(hObject);// ��������� ����������
		} // if
	} // if
	else if (argc > 1) // (!) ����� ��������� ��������
	{
		std::cout << "��������� ��������� ������:" << std::endl;
		if(_tcsnicmp(argv[1], TEXT("semaphore-"), 10) == 0)
		{
			HANDLE hSemaphore = NULL; // ���������� ��������
			if ((4 == argc) && (_tcsicmp(argv[1], TEXT("semaphore-duplicate")) == 0))
			{
				// �������� �� ��������� ������ ������������� ������������� ��������
				DWORD dwProcessId = (DWORD)_ttoi(argv[2]);
				// �������� �� ��������� ������ ���������� ��������
				HANDLE hObject = (HANDLE)_tcstoui64(argv[3], NULL, 16);

				// ��������� ���������� ���������� ��������
				DuplicateHandle(dwProcessId, hObject, &hSemaphore, SEMAPHORE_ALL_ACCESS);
			} 

			if (NULL != hSemaphore)
			{
				_tprintf(TEXT("> \n"));
				_tprintf(TEXT("> ���� (�������):\n"));

				for (int i = 0; i < 3; ++i)
				{
					// ������� ������������ ��������,
					// � ����� ����������� ���
					WaitForSingleObject(hSemaphore, INFINITE);

					for (int j = 0; j < 10; ++j)
					{
						_tprintf(TEXT("> %d\n"), j + 1);
						Sleep(500);
					} // for

					ReleaseSemaphore(hSemaphore, 1, NULL);// ����� ����������� �������� �������� �������� ����� ��������
				} // for

				// ��������� ���������� ��������
				CloseHandle(hSemaphore);
			} // if
		} // if
	}
	
	} // _tmain
 
// ------------------------------------------------------------------------------------------------
BOOL DuplicateHandle(DWORD dwProcessId, HANDLE hSourceHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess)
{
	BOOL bRet = FALSE;

	// ��������� �������
	HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);

	if (NULL != hProcess)
	{
		// ��������� ���������� ����������
		bRet = DuplicateHandle(hProcess, hSourceHandle, GetCurrentProcess(), lpTargetHandle, dwDesiredAccess, FALSE, 0);

		// ��������� ���������� ��������
		CloseHandle(hProcess);
	} // if

	return bRet;
} // DuplicateHandle
