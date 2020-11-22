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

		_tprintf(TEXT("\n������ �������� �����������:\n\n"));

		// ������� ������ �������� ����������� (Win64)
		PrintAutorunApplications(HKEY_CURRENT_USER, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
		PrintAutorunApplications(HKEY_LOCAL_MACHINE, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	} 

} // _tmain

// ----------------------------------------------------------------------------------------------
void PrintInstallApplications(REGSAM samDesired)
{
	HKEY hKey; // ���������� ����� ������

	// ��������� ���� �������
	LSTATUS retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), REG_OPTION_NON_VOLATILE, samDesired, &hKey);

	if (ERROR_SUCCESS == retCode)
	{
		DWORD cSubKeys; // ����� ��������� ������
		DWORD cbMaxSubKey; // ��� ���������� ������� �����
		DWORD i; //��� ��������
		// ��������� ����� ��������� ������ � 
		// ������������ ������ ������� ��� ����� ���������� �����
		retCode = RegQueryInfoKey(hKey, NULL, NULL, NULL, &cSubKeys, &cbMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);

		if ((ERROR_SUCCESS == retCode) && (cSubKeys > 0))
		{
			printf("\nNumber of values: %u\n", cSubKeys);
			// ������� ������ ��� ����� ��� ����� ���������� �����
			LPTSTR szSubKeyName = new TCHAR[cbMaxSubKey + 1];

			for ( i = 0;  i< cSubKeys; ++i)
			{
				HKEY hSubKey = NULL; // ���������� ���������� ����� �������

				// ������� ��� ���������� ����� � �������� dwIndex
				DWORD cchValue = cbMaxSubKey + 1;

				if (ERROR_SUCCESS == RegEnumKeyEx(hKey, i, szSubKeyName, &cchValue, NULL, NULL, NULL, NULL))
				{
					//�������� �� ��������� ������
					if (ERROR_SUCCESS == RegOpenKeyEx(hKey, szSubKeyName, REG_OPTION_NON_VOLATILE, samDesired, &hSubKey))
					{
						// ������� ������������ ����� �������� �� ��������� ����� �������
						DWORD cMaxValueLen = 0;
						retCode = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cMaxValueLen, NULL, NULL);

						if ((ERROR_SUCCESS == retCode) && (cMaxValueLen > 0))
						{
							// ������� ������ ��� ����� ��� ��������
							LPTSTR lpValueData = new TCHAR[cMaxValueLen];

							// ������� ��������� ��������
							retCode = RegGetValueSZ(hSubKey, TEXT("DisplayName"), lpValueData, cMaxValueLen, NULL);

							if ((ERROR_SUCCESS == retCode) && (_T('\0') != lpValueData[0]))
							{
								_tprintf(TEXT("-----\n%s\n"), lpValueData);

								// ������� ��������� ��������
								retCode = RegGetValueSZ(hSubKey, TEXT("Publisher"), lpValueData, cMaxValueLen, NULL);

								if ((ERROR_SUCCESS == retCode) && (_T('\0') != lpValueData[0]))
								{
									_tprintf(TEXT("%s\n"), lpValueData);
								} // if

								// ������� ��������� ��������
								retCode = RegGetValueSZ(hSubKey, TEXT("DisplayVersion"), lpValueData, cMaxValueLen, NULL);



								if ((ERROR_SUCCESS == retCode) && (_T('\0') != lpValueData[0]))
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
	LSTATUS lStatus = RegOpenKeyEx(hRootKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), REG_OPTION_NON_VOLATILE, samDesired, &hKey);

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

	LSTATUS RegGetValueSZ(HKEY hKey, LPCTSTR section, LPTSTR key, DWORD cch, LPDWORD lpcchNeeded)
	{
		DWORD dwType;
		// ���������� ��� ����������� �������� ���������
		LSTATUS res = RegQueryValueEx(hKey, section, NULL, &dwType, NULL, NULL);

		if (ERROR_SUCCESS == res && REG_SZ == dwType)
		{
			// ��������� ������ ������ (� ������)
			DWORD cb = cch * sizeof(TCHAR);
			// �������� �������� ���������
			res = RegQueryValueEx(hKey, section, NULL, NULL, (LPBYTE)key, &cb);

			if (NULL != lpcchNeeded)
				*lpcchNeeded = cb / sizeof(TCHAR);
		} // if
		else if (ERROR_SUCCESS == res)
		{
			res = ERROR_UNSUPPORTED_TYPE; // �������� ��� ������
		} // if

		return res;
	} // RegGetValueSZ
// ----------------------------------------------------------------------------------------------

	/*LONG GetStringRegKey(HKEY hKey, LPCTSTR section, LPTSTR key)
	{
		
		WCHAR szBuffer[512];
		DWORD dwBufferSize = sizeof(szBuffer);
		ULONG nError;
		nError = RegQueryValueExW(hKey, section, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
		if (ERROR_SUCCESS == nError)
		{
			strValue = szBuffer;
		}
		return nError;
	}*/

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
	