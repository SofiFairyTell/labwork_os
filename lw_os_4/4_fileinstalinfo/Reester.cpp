#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <locale.h>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


// ������� ��� ������ ������ ������������� ��������
void PrintInstallApplications(REGSAM samDesired);
// ������� ��� ������ ������ �������� �����������
void PrintAutorunApplications(HKEY hRootKey, REGSAM samDesired);

LSTATUS RegGetValueSZ(HKEY hKey, LPCTSTR lpValueName, LPTSTR lpszData, DWORD cch, LPDWORD lpcchNeeded);
// ������� ��� �����������, �������� �� ������������ ������� 64-��������� ������� Windows
BOOL IsWin64();

// ----------------------------------------------------------------------------------------------
int _tmain()
{
	_tsetlocale(LC_ALL, TEXT(""));

	if (IsWin64()) // 64-��������� ������ Windows
	{
		_tprintf(TEXT("������ ������������� ��������:\n\n"));

		// ������� ������ ������������� �������� (Win64)
		PrintInstallApplications(KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
		// ������� ������ ������������� �������� (Win32)
		PrintInstallApplications(KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_32KEY);

		_tprintf(TEXT("\n������ �������� �����������:\n\n"));

		// ������� ������ �������� ����������� (Win64)
		PrintAutorunApplications(HKEY_CURRENT_USER, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
		PrintAutorunApplications(HKEY_LOCAL_MACHINE, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
		// ������� ������ �������� ����������� (Win32)
		PrintAutorunApplications(HKEY_CURRENT_USER, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_32KEY);
		PrintAutorunApplications(HKEY_LOCAL_MACHINE, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_32KEY);
	} // if
	else
	{
		_tprintf(TEXT("������ ������������� ��������:\n\n"));

		// ������� ������ ������������� �������� (Win32)
		PrintInstallApplications(KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);

		_tprintf(TEXT("\n������ �������� �����������:\n\n"));

		// ������� ������ �������� ����������� (Win32)
		PrintAutorunApplications(HKEY_CURRENT_USER, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);
		PrintAutorunApplications(HKEY_LOCAL_MACHINE, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE);
	} // else
} // _tmain

// ----------------------------------------------------------------------------------------------
void PrintInstallApplications(REGSAM samDesired)
{
	HKEY hKey; // ���������� ����� ������

	// ��������� ���� �������
	LSTATUS lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), REG_OPTION_NON_VOLATILE, samDesired, &hKey);

	if (ERROR_SUCCESS == lStatus)
	{
		DWORD cSubKeys; // ����� ��������� ������
		DWORD cMaxName; // ������������ ������ ������� ��� ����� ���������� �����

		// ��������� ����� ��������� ������ � 
		// ������������ ������ ������� ��� ����� ���������� �����
		lStatus = RegQueryInfoKey(hKey, NULL, NULL, NULL, &cSubKeys, &cMaxName, NULL, NULL, NULL, NULL, NULL, NULL);

		if ((ERROR_SUCCESS == lStatus) && (cSubKeys > 0))
		{
			// ������� ������ ��� ����� ��� ����� ���������� �����
			LPTSTR szSubKeyName = new TCHAR[cMaxName + 1];

			for (DWORD dwIndex = 0; dwIndex < cSubKeys; ++dwIndex)
			{
				HKEY hSubKey = NULL; // ���������� ���������� ����� ������

				// ������� ��� ���������� ����� � �������� dwIndex
				DWORD cchName = cMaxName + 1;
				lStatus = RegEnumKeyEx(hKey, dwIndex, szSubKeyName, &cchName, NULL, NULL, NULL, NULL);

				if (ERROR_SUCCESS == lStatus)
				{
					// ��������� ��������� ���� �������
					lStatus = RegOpenKeyEx(hKey, szSubKeyName, REG_OPTION_NON_VOLATILE, samDesired, &hSubKey);
				} // if

				if (ERROR_SUCCESS == lStatus)
				{
					// ������� ������������ ����� �������� �� ��������� ����� �������
					DWORD cMaxValueLen = 0;
					lStatus = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cMaxValueLen, NULL, NULL);

					if ((ERROR_SUCCESS == lStatus) && (cMaxValueLen > 0))
					{
						// ������� ������ ��� ����� ��� ��������
						LPTSTR lpValueData = new TCHAR[cMaxValueLen];

						// ������� ��������� ��������
						lStatus = RegGetValueSZ(hSubKey, TEXT("DisplayName"), lpValueData, cMaxValueLen, NULL);

						if ((ERROR_SUCCESS == lStatus) && (_T('\0') != lpValueData[0]))
						{
							_tprintf(TEXT("-----\n%s\n"), lpValueData);

							// ������� ��������� ��������
							lStatus = RegGetValueSZ(hSubKey, TEXT("Publisher"), lpValueData, cMaxValueLen, NULL);

							if ((ERROR_SUCCESS == lStatus) && (_T('\0') != lpValueData[0]))
							{
								_tprintf(TEXT("%s\n"), lpValueData);
							} // if

							// ������� ��������� ��������
							lStatus = RegGetValueSZ(hSubKey, TEXT("DisplayVersion"), lpValueData, cMaxValueLen, NULL);

							if ((ERROR_SUCCESS == lStatus) && (_T('\0') != lpValueData[0]))
							{
								_tprintf(TEXT("%s\n"), lpValueData);
							} // if

							_tprintf(TEXT("\n"));
						} // if

						// ��������� ���������� ������
						delete[] lpValueData;
					} // if

					// ��������� ���������� ���������� ����� �������
					RegCloseKey(hSubKey), hSubKey = NULL;
				} // if
			} // for

			// ��������� ���������� ������
			delete[] szSubKeyName;
		} // if

		// ��������� ���������� ����� �������
		RegCloseKey(hKey), hKey = NULL;
	} // if
} // PrintInstallApplications

// ----------------------------------------------------------------------------------------------
void PrintAutorunApplications(HKEY hRootKey, REGSAM samDesired)
{
	HKEY hKey = NULL; // ���������� ����� ������

	// ��������� ���� �������
	LSTATUS lStatus = RegOpenKeyEx(hRootKey,
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), REG_OPTION_NON_VOLATILE, samDesired, &hKey);

	if (ERROR_SUCCESS == lStatus)
	{
		DWORD cValues; // ����� ���������� �����
		DWORD cMaxValueNameLen; // ������������ ������ ������ ��� ��� ����������
		DWORD cbMaxValueLen; // ������������ ������ ������ ��� �������� ����������

		// ��������� ����� ���������� ����� � 
		// ������������ ������ ������� ��� ��� � �������� ����������
		lStatus = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, &cMaxValueNameLen, &cbMaxValueLen, NULL, NULL);

		if ((ERROR_SUCCESS == lStatus) && (cValues > 0))
		{
			// ������� ������ ��� ������
			LPTSTR szValueName = new TCHAR[cMaxValueNameLen + 1];
			LPBYTE lpValueData = new BYTE[cbMaxValueLen];

			for (DWORD dwIndex = 0; dwIndex < cValues; ++dwIndex)
			{
				// ������� ���, ��� � �������� ��������� � �������� dwIndex
				DWORD cchValueName = cMaxValueNameLen + 1, dwType, cbData = cbMaxValueLen;
				lStatus = RegEnumValue(hKey, dwIndex, szValueName, &cchValueName, NULL, &dwType, lpValueData, &cbData);

				if ((ERROR_SUCCESS == lStatus) && (cchValueName > 0))
				{
					if ((REG_SZ == dwType) || (REG_EXPAND_SZ == dwType))
					{
						_tprintf(TEXT("-----\n%s\n"), szValueName);
						_tprintf(TEXT("%s\n\n"), (LPTSTR)lpValueData);
					} // if
				} // if
			} // for

			// ��������� ������, ���������� ��� ������
			delete[] lpValueData;
			delete[] szValueName;
		} // if

		// ��������� ���������� ����� �������
		RegCloseKey(hKey), hKey = NULL;
	} // if
} // PrintAutorunApplications
//work wit regist
	LSTATUS RegGetValueSZ(HKEY hKey, LPCTSTR lpValueName, LPTSTR lpszData, DWORD cch, LPDWORD lpcchNeeded)
	{
		DWORD dwType;
		// ���������� ��� ����������� �������� ���������
		LSTATUS lStatus = RegQueryValueEx(hKey, lpValueName, NULL, &dwType, NULL, NULL);

		if (ERROR_SUCCESS == lStatus && REG_SZ == dwType)
		{
			// ��������� ������ ������ (� ������)
			DWORD cb = cch * sizeof(TCHAR);
			// �������� �������� ���������
			lStatus = RegQueryValueEx(hKey, lpValueName, NULL, NULL, (LPBYTE)lpszData, &cb);

			if (NULL != lpcchNeeded)
				*lpcchNeeded = cb / sizeof(TCHAR);
		} // if
		else if (ERROR_SUCCESS == lStatus)
		{
			lStatus = ERROR_UNSUPPORTED_TYPE; // �������� ��� ������
		} // if

		return lStatus;
	} // RegGetValueSZ
// ----------------------------------------------------------------------------------------------

	BOOL IsWin64()
	{
#ifdef _WIN64

		return TRUE;

#else // _WIN64

		// (!) ������� IsWow64Process �������������� �� �� ���� ������� Windows.

		typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
		LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

		if (NULL != fnIsWow64Process)
		{
			BOOL bIsWow64;
			return (fnIsWow64Process(GetCurrentProcess(), &bIsWow64) != FALSE) && (bIsWow64 != FALSE);
		} // if

		return FALSE;

#endif // !_WIN64
	}
	