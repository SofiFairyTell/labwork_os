#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <locale.h>
#include <iostream>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


// ������� ��� ������ ������ ������������� ��������
void InstallAppList(REGSAM samDesired);
// ������� ��� ������ ������ �������� �����������
void AutorunAppList(HKEY hRootKey, REGSAM samDesired);

void SubFind(LSTATUS retCode, DWORD CMVlen, HKEY hSubKey);

LSTATUS RegGetValueSZ(HKEY hKey, LPCTSTR lpValueName, LPTSTR lpszData, DWORD cch, LPDWORD lpcchNeeded);
// ������� ��� �����������, �������� �� ������������ ������� 64-��������� ������� Windows
BOOL IsWin64();

// ----------------------------------------------------------------------------------------------
int _tmain()
{
	setlocale(LC_ALL, "");

	if (IsWin64()) // 64-��������� ������ Windows
	{
		std::cout<<"������ ������������� ��������:\n\n";

		// ������� ������ ������������� �������� (Win64)
		InstallAppList(KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);

		std::cout << "������ �������� ����������� HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run:\n\n";

		AutorunAppList(HKEY_LOCAL_MACHINE, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
		
		std::cout << "������ �������� ����������� HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run:\n\n";

		AutorunAppList(HKEY_CURRENT_USER, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
	} 

} 
// ----------------------------------------------------------------------------------------------
void InstallAppList(REGSAM AccessRights)
{
	HKEY hKey; // ���������� ����� �������

	// ��������� ���� �������
	LSTATUS retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), REG_OPTION_NON_VOLATILE, AccessRights, &hKey);

	if (ERROR_SUCCESS == retCode)
	{
		DWORD cSubKeys, cbMaxSubKey,i; // ��������� �����,�� ������������ ������,���������� ��� �������� �� ���

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
					//������ ����� �� ��������� ������ 
					if (ERROR_SUCCESS == RegOpenKeyEx(hKey, szSubKeyName, REG_OPTION_NON_VOLATILE, AccessRights, &hSubKey))
					{
						DWORD cMaxValueLen = 0;//��� ����������� ������������ ����� ��������
						retCode = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cMaxValueLen, NULL, NULL);
						SubFind(retCode, cMaxValueLen, hSubKey);//������������ ����������� �� ���������� �����			
						RegCloseKey(hSubKey), hSubKey = NULL;// ��������� ���������� ���������� ����� �������
					}
				}		
			} 
			delete[] szSubKeyName;
		} 
	
		RegCloseKey(hKey), hKey = NULL;// ��������� ���������� ����� �������
	} 
}

void SubFind(LSTATUS retCode, DWORD CMVlen, HKEY hSubKey)
{
	if ((ERROR_SUCCESS == retCode) && (CMVlen > 0))
	{
		// ������� ������ ��� ����� ��� ��������
		LPTSTR lpData = new TCHAR[CMVlen];

		// ������� ��������� ��������
		retCode = RegGetValueSZ(hSubKey, TEXT("DisplayName"), lpData, CMVlen, NULL);

		if ((ERROR_SUCCESS == retCode) && (_T('\0') != lpData[0]))
		{
			_tprintf(TEXT("-----\n%s\n"), lpData);

			// ������� ��������� ��������
			retCode = RegGetValueSZ(hSubKey, TEXT("Publisher"), lpData, CMVlen, NULL);

			if ((ERROR_SUCCESS == retCode) && (_T('\0') != lpData[0]))
			{
				_tprintf(TEXT("%s\n"), lpData);
			} 

			// ������� ��������� ��������
			retCode = RegGetValueSZ(hSubKey, TEXT("DisplayVersion"), lpData, CMVlen, NULL);

			if ((ERROR_SUCCESS == retCode) && (_T('\0') != lpData[0]))
			{
				_tprintf(TEXT("%s\n"), lpData);
			} // if
		
			std::cout <<  "\n";
		} 

		delete[] lpData;// ��������� ���������� ������
	}
}


void AutorunAppList(HKEY hRootKey, REGSAM AccessRights)
{
	HKEY hKey = NULL; // ���������� ����� ������

	// ��������� ���� �������
	LSTATUS lStatus = RegOpenKeyEx(hRootKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), REG_OPTION_NON_VOLATILE, AccessRights, &hKey);

	if (ERROR_SUCCESS == lStatus)
	{
		DWORD Val, MaxNameLen, MaxVallLen; // ����� ���������� �����, ����� ��� ����, ����� ��� ��������

		// ��������� ����� ���������� ����� � 
		// ������������ ������ ������� ��� ��� � �������� ����������
		lStatus = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &Val, &MaxNameLen, &MaxVallLen, NULL, NULL);

		if ((ERROR_SUCCESS == lStatus) && (Val > 0))
		{
			// ������� ������ ��� ������
			LPTSTR NameKey = new TCHAR[MaxNameLen + 1];
			LPBYTE lpData = new BYTE[MaxVallLen];

			for (DWORD i = 0; i < Val; ++i)
			{
					
				DWORD cchValueName = MaxNameLen + 1, dwType, cbData = MaxVallLen;
				lStatus = RegEnumValue(hKey, i, NameKey, &cchValueName, NULL, &dwType, lpData, &cbData);

				if ((ERROR_SUCCESS == lStatus) && (cchValueName > 0))
				{
					//reg_sz - ��� ������. ��� �������� UNICODE-������
					if ((REG_SZ == dwType) || (REG_EXPAND_SZ == dwType))
					{
						_tprintf(TEXT("-----\n%s\n"), NameKey);//��� ���������
						_tprintf(TEXT("%s\n\n"), (LPTSTR)lpData);//�������� ���������
					} 
				}
			}
					
			delete[] lpData;//������������ ������
			delete[] NameKey;//������������ ������
		} 

		RegCloseKey(hKey), hKey = NULL;		// ��������� ���������� ����� �������
	} 
} 

  //work wit regist

	LSTATUS RegGetValueSZ(HKEY hKey, LPCTSTR section, LPTSTR key, DWORD cch, LPDWORD lpcchNeeded)
	{
		DWORD dwType;
		LSTATUS res = RegQueryValueEx(hKey, section, NULL, &dwType, NULL, NULL);
			if (res != ERROR_SUCCESS)
			{
					res = ERROR_UNSUPPORTED_TYPE; // �������� ��� ������
			}
			else
				if (ERROR_SUCCESS == res)
				{
					DWORD cb = cch * sizeof(TCHAR);
						// �������� �������� ���������
					res = RegQueryValueEx(hKey, section, NULL, NULL, (LPBYTE)key, &cb);
					if (NULL != lpcchNeeded) *lpcchNeeded = cb / sizeof(TCHAR);
				}
		return res;
	} 


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
	