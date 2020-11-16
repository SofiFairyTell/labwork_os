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
		/*���������� ������� ���� � ������������� ������������� �������� ��������� � �������� ������� ��� ��������� � ��������� ������*/
	
		hObject = CreateSemaphore(NULL, 1, 2, TEXT("Semaphor"));

		if (hObject != NULL)
		{
			// ��������� ��������� ������ ��� �������� �������� ���������
			StringCchPrintf(szCmdLine, _countof(szCmdLine), TEXT("%s semaphore-duplicate %d %p"), argv[0], (int)GetCurrentProcessId(), hObject);
		} // if

		if (hObject != NULL)
		{
			// ������� ������ ���������
			HANDLE* ProcArray = new HANDLE[n_processes];

			STARTUPINFO si = { sizeof(STARTUPINFO) };
			PROCESS_INFORMATION procinfo = { 0 };

			for (int i = 0; i < n_processes; ++i)
			{
				// ��������� ����� �������
				BOOL bRet = CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &procinfo);

				if (FALSE != bRet)
				{
					/*�������� ���������� � ������ � ������� ���������� ������*/
					ProcArray[i] = procinfo.hProcess;
					CloseHandle(procinfo.hThread);
				} 
				else
				{
					ProcArray[i] = NULL;
				} 
			} 
			
			WaitForMultipleObjects(n_processes, ProcArray, TRUE, INFINITE);// ������� ���������� ���� �������� ���������

			for (int i = 0; i < n_processes; ++i) 
				CloseHandle(ProcArray[i]);
		
			delete[] ProcArray;
		
			CloseHandle(hObject);// ��������� ����������
		} 
	} 
	else if (argc > 1) // (!) ����� ��������� ��������
	{
		std::cout << "��������� ��������� ������:" << std::endl;
		HANDLE hSemaphore = NULL; // ���������� ��������
		if ((4 == argc) && (_tcsicmp(argv[1], TEXT("semaphore-duplicate")) == 0))
			{
				// �������� �� ��������� ������ ������������� ������������� ��������
				UINT parentPID = (UINT)_ttoi(argv[2]);
				
				HANDLE hObject = (HANDLE)_tcstoui64(argv[3], NULL, 16);//�������� ����������� �������� � 16-� �������
			
				DuplicateHandle(parentPID, hObject, &hSemaphore, SEMAPHORE_ALL_ACCESS);// ��������� ���������� ���������� ��������
			} 

			if (NULL != hSemaphore)
			{
				std::cout<<"> ���� (�������):"<<std::endl;

				for (int i = 0; i < 3; ++i)
				{
					WaitForSingleObject(hSemaphore, INFINITE);//������ ���� ����������� �������
					int j;
					for (j = 0; j < 10; ++j)
					{
						std::cout << j + 1;
						Sleep(500);	//�������� ������ ����			
					} 
					if (j == 10) std::cout << std::endl;
					ReleaseSemaphore(hSemaphore, 1, NULL);// ����� ����������� �������� �������� �������� ����� �������� 
				} 
				
				CloseHandle(hSemaphore);// ��������� ���������� ��������
			} // if
	}
} 
 
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
