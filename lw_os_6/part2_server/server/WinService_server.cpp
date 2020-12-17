#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <locale.h>
#include <strsafe.h>
#include <fstream>
#include <iostream>

//extern LPCTSTR service_name; // ��� ������
//extern LPCTSTR SvcDisplayName; // ������������ ��� ������

LPCTSTR service_name = TEXT("DemoService"); //// ���������� ��� �������, ������������ SCM
LPCTSTR SvcDisplayName = TEXT("DemoService");// ������� ��� ������� � ������ ����������

SERVICE_STATUS service_status; // ������� ��������� ������
SERVICE_STATUS_HANDLE hServiceStatus; // ���������� ��������� ������

std::ofstream  out;   // �������� ���� ��� ��������� ������ �������
int  nCount;     // �������

// ------------------------------------------------------------------------------------------------

BOOL OnSvcInit(DWORD dwArgc, LPTSTR *lpszArgv);// ��� ������� ���������� ��� ������� ������

void OnSvcStop(void);// ��� ������� ���������� ��� ��������� ������

DWORD SvcMain(DWORD dwArgc, LPTSTR *lpszArgv);// � ���� ������� ���������� �������� ����������

// ------------------------------------------------------------------------------------------------
DWORD WINAPI SvcHandler(DWORD fdwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	if (SERVICE_CONTROL_STOP == fdwControl || SERVICE_CONTROL_SHUTDOWN == fdwControl)
	{
		OnSvcStop(); // ������������� ������
		service_status.dwCurrentState = SERVICE_STOP_PENDING; // ����� ��������� ������
	}
	
	SetServiceStatus(hServiceStatus, &service_status);// �������� ������� ��������� ������
	return NO_ERROR;
} 

// ------------------------------------------------------------------------------------------------
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// ������������ ���������� ����������� ������ ��� �������
	hServiceStatus = RegisterServiceCtrlHandlerEx(service_name, SvcHandler, NULL);

	if (!hServiceStatus)
	{
		out.open("C:\\ServiceFile.log");
		out << "Register service control handler failed.";
		out.close();

		return;
	}

	if (NULL != hServiceStatus)
	{
		// ��������� ���������
		service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		service_status.dwCurrentState = SERVICE_START_PENDING;
		service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
		service_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		service_status.dwServiceSpecificExitCode = 0;
		service_status.dwCheckPoint = 0;
		service_status.dwWaitHint = 0;

		// ������ ��������� ��������� ������
		SetServiceStatus(hServiceStatus, &service_status);

		// /// //

		if (OnSvcInit(dwArgc, lpszArgv) != FALSE)
		{
			// ���������� ������ ��� ���������� 
			service_status.dwCurrentState = SERVICE_RUNNING; // ����� ���������
		
			SetServiceStatus(hServiceStatus, &service_status);// �������� ������� ��������� ������

			DWORD dwExitCode = SvcMain(dwArgc, lpszArgv);

			if (dwExitCode != 0) // ���������� ��� ������
			{
				service_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
				service_status.dwServiceSpecificExitCode = dwExitCode;
			} 
			else
			{
				service_status.dwWin32ExitCode = NO_ERROR;
			} 
		} 

		service_status.dwCurrentState = SERVICE_STOPPED; // ����� ���������
		SetServiceStatus(hServiceStatus, &service_status);// ������ �������� ��������� ������
	} 
} 

// ------------------------------------------------------------------------------------------------
int _tmain(int argc, LPTSTR argv[])
{
	_tsetlocale(LC_ALL, TEXT(""));

	if (argc < 2)
	{
		_tprintf(TEXT("> �� ������ ��������.\n"));
		return 0; // ��������� ������ ����������
	} // if


	if (_tcscmp(argv[1], TEXT("/runservice")) == 0) // ������ ������ ������
	{
		//  �������������� ��������� ��������
		SERVICE_TABLE_ENTRY service_table[] =
		{
			{(LPTSTR)service_name, ServiceMain},// ��� ������� � ������� �������
			{NULL, NULL} // ������ �������� ���
		};
//��������� � ��������� �� ���������, ������ ��� ��������� �������� ������ �����������, � �� ����� ����� �� ��������� 
		if (!StartServiceCtrlDispatcher(service_table))
		{
			out.open("C:\\ServiceFile.log");
			out << "Start service control dispatcher failed.";
			out.close();
			return 0;
		}
		else
		{
			out.open("C:\\ServiceFile.log");
			out << "Start service control dispatcher okey.";
			out.close();
		}
	}
	
	if (_tcscmp(argv[1], TEXT("/create")) == 0) // �������� ������
	{
			// ����������� � ���������� ��������
			SC_HANDLE hServiceControlManager = OpenSCManager
			(NULL, //��������� ������
				NULL, //�������� ���� ������ ��������
				SC_MANAGER_CREATE_SERVICE //�������� �������� �������
			);
			if (hServiceControlManager == NULL)
			{
				std::cout << "Open service control manager failed." << std::endl
					<< "The last error code: " << GetLastError() << std::endl
					<< "Press any key to exit." << std::endl;
				std::cin.get();

				return -1;
			}

			TCHAR CmdLine[MAX_PATH + 13]; // ��������� ������

			// ���������� ���� � ��� ������������ �����
			GetModuleFileName(NULL, CmdLine, _countof(CmdLine));
			// ��������� �������� ��������� ������
			StringCchCat(CmdLine, _countof(CmdLine), TEXT(" /runservice"));

			// ������� ������
			SC_HANDLE hService = CreateService(
				hServiceControlManager, //���������� ��������� ��������
				service_name, //��������� ��� �������
				SvcDisplayName, //������� ��� ������������
				0,//��� ������ ���� �������� �������
				SERVICE_WIN32_OWN_PROCESS, // ������ �������� ���������
				SERVICE_DEMAND_START, // ������ ����������� "�������"
				SERVICE_ERROR_NORMAL, CmdLine, NULL, NULL, NULL, NULL, NULL);

			if (hService == NULL)
			{
				std::cout << "Create service failed." << std::endl
					<< "The last error code: " << GetLastError() << std::endl
					<< "Press any key to exit." << std::endl;
				std::cin.get();

				// ��������� ���������� ��������� ��������
				CloseServiceHandle(hServiceControlManager);

				return 0;
			}
			else
			{
				std::cout << "Service is installed." << std::endl;

				std::cin.get();

				// ��������� �����������
				CloseServiceHandle(hService);
				CloseServiceHandle(hServiceControlManager);

				return 0;
			}

		}

	if (_tcscmp(argv[1], TEXT("/delete")) == 0) // �������� ������
	{
		// ��������� SCM
		SC_HANDLE hServiceControlManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);//������ �������� - ���������� � ����������

		if (hServiceControlManager == NULL)
		{
			std::cout << "Open service control manager failed." << std::endl
				<< "The last error code: " << GetLastError() << std::endl
				<< "Press any key to continue." << std::endl;
			std::cin.get();
			return -1;
		}
		else
		{
			std::cout << "Service is opened." << std::endl;
			std::cin.get();
		}


		// ��������� ������ ��� ��������
		SC_HANDLE hService = OpenService(hServiceControlManager, service_name, SERVICE_ALL_ACCESS | DELETE);

		if (hService == NULL)
		{
			std::cout << "Open service failed." << std::endl
				<< "The last error code: " << GetLastError() << std::endl
				<< "Press any key to exit." << std::endl;
			std::cin.get();

			// ��������� ���������� ��������� ��������
			CloseServiceHandle(hServiceControlManager);
			return 0;
		}

		// �������� ��������� �������
		if (!QueryServiceStatus(hService, &service_status))
		{
			std::cout << "Query service status failed." << std::endl
				<< "The last error code: " << GetLastError() << std::endl
				<< "Press any key to exit." << std::endl;
			std::cin.get();

			// ��������� �����������
			CloseServiceHandle(hServiceControlManager);
			CloseServiceHandle(hService);

			return 0;
		}

		// ���� ������ ��������, �� ������������� ���
		if (service_status.dwCurrentState != SERVICE_STOPPED)
		{
			std::cout << "Service is working. It will be stoped" << std::endl;
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &service_status))
			{
				std::cout << "Control service failed." << std::endl
					<< "The last error code: " << GetLastError() << std::endl
					<< "Press any key to exit." << std::endl;
				std::cin.get();

				// ��������� �����������
				CloseServiceHandle(hServiceControlManager);
				CloseServiceHandle(hService);

				return 0;
			}
			// ����, ���� ������ �����������
			Sleep(500);
		}

		if (!DeleteService(hService))
		{
			std::cout << "Delete service failed." << std::endl
				<< "The last error code: " << GetLastError() << std::endl
				<< "Press any key to exit." << std::endl;
			std::cin.get();

			// ��������� �����������
			CloseServiceHandle(hServiceControlManager);
			CloseServiceHandle(hService);

			return 0;
		}
		else
		{
			std::cout << "The service is deleted." << std::endl
				<< "Press any key to exit." << std::endl;
			std::cin.get();
		}


		CloseServiceHandle(hService);
		CloseServiceHandle(hServiceControlManager); // ��������� ����������
		return 0; // ��������� ������ ����������
	} // if

	_tprintf(TEXT("> ����������� ��������.\n"));
}