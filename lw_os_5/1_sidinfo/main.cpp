#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>
#include <locale.h>
#include <NTSecAPI.h>//for las_handle
#include <winnt.h>
#include <sddl.h>//for ConvertSidToStringSidW
#include <LMCons.h>
#include <cwchar>
#include <iostream>
//#include <>

int _tmain();

/*������� � ����������*/
LSA_HANDLE OpenLocalPolicy(ACCESS_MASK AccessType);//�������� ����������� �������� ������������ ���������� ��

BOOL GetAccountSID_W(LPCWSTR AccountName, PSID *ppsid);//������ SID ��
BOOL GetAccountName_W(PSID psid, LPWSTR* AccountName);//���������� ��� �������� �� ��� SID
BOOL GetWellKnowSID(WELL_KNOWN_SID_TYPE WellKnowSidType, PSID sidDomain, PSID *sidWellKnow);//����������� ������ ���������� SID

void ListOfPrivilegesAndRights_User(LSA_HANDLE PolicyHandle, PSID sidUser);//����� ������ ���������� ������������
void CoutSID(PSID psid);//����� ������� �� �����
//UNICODE ������
int _tmain()
{
	_tsetlocale(LC_ALL, TEXT(" "));

	/*Pointer Security ID*/
	PSID sidDomain = NULL; //security ID ��� ���������� ��
	PSID sidUser = NULL; //security ID ��� ���������� ������������
	PSID sidWellKnow;//�
	LSA_HANDLE PolicyHandle = OpenLocalPolicy(POLICY_LOOKUP_NAMES);

	/*��������������� ����������*/
	DWORD count_char;//��� ����������� ��������
	BOOL RetRes = FALSE; //��� �����������
	DWORD dwError = NULL; //��� ������

	/*���������� ��� ������ ���� �� � ������������*/
	WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1] = TEXT("");
	WCHAR UserName[UNLEN + 1] = L"";
	
	/*������*/
	count_char = _countof(ComputerName);

	RetRes = GetComputerNameW(ComputerName, &count_char);//������ ��� ��

	if (RetRes != FALSE)
	{
		RetRes = GetAccountSID_W(ComputerName, &sidDomain);
		
		if ((RetRes != FALSE) && (sidDomain != NULL))
		{
			CoutSID(sidDomain);
		}
	}
	else
	{
		SetLastError(dwError);//����� ������?
		return 0; //��������� ������ ���������
	}

	count_char = _countof(UserName);

	RetRes = GetUserName(UserName, &count_char);//������ ��� ������������
	if (RetRes != FALSE)
	{
		RetRes = GetAccountSID_W(UserName, &sidUser);

		if ((RetRes != FALSE) && (sidUser != NULL))
		{
			CoutSID(sidUser);
			//����� ������ ����������

			if (PolicyHandle != NULL)
			{
				ListOfPrivilegesAndRights_User(PolicyHandle, sidUser);
			//	LocalFree(sidUser);//����������� ����?
			}

			
		}
	}

	constexpr WELL_KNOWN_SID_TYPE WellKnownType[] =
	{
		WinConsoleLogonSid,
		WinAuthenticatedUserSid,
		WinLocalSystemSid,
		WinAccountAdministratorSid,
		WinBuiltinIUsersSid,
		WinBuiltinNetworkConfigurationOperatorsSid
	};

	for (int i = 0; i < _countof(WellKnownType); ++i)
	{
		RetRes = GetWellKnowSID(WellKnownType[i], sidDomain, &sidWellKnow);
		
		if ((RetRes != FALSE) && (sidWellKnow != NULL))
		{
			CoutSID(sidDomain);
			//����� ������ ����������

			if (PolicyHandle != NULL)
			{
				ListOfPrivilegesAndRights_User(PolicyHandle, sidWellKnow);

			}

			//LocalFree(sidWellKnow);
		}
	}

  FreeSid(sidDomain);
  FreeSid(sidUser);
  FreeSid(sidWellKnow);
  LsaClose(PolicyHandle);

}


LSA_HANDLE OpenLocalPolicy(ACCESS_MASK AccessType)
{
	//POLICY_LOOKUP_NAMES - this access type is needed to translate between names and SIDs.
	LSA_HANDLE PolicyHandle;
	LSA_OBJECT_ATTRIBUTES ObjAtr;
	
	ZeroMemory(&ObjAtr, sizeof(ObjAtr));//������������� ��������� LSA_OBJECT_ATTRIBUTES
	
	NTSTATUS ntstatus = LsaOpenPolicy(NULL, &ObjAtr, AccessType, &PolicyHandle);//��������� ����������� �������� ������������

	SetLastError(LsaNtStatusToWinError(ntstatus)); //��� ������

	return LSA_SUCCESS(ntstatus) ? PolicyHandle : NULL; //���� ��������� �������, �� ������� PolicyHandle

}

/*����������� ������� ������������ SID*/
BOOL GetAccountSID_W(LPCWSTR AccountName, PSID *ppsid)
{
BOOL RetRes = FALSE;
SID_NAME_USE SidType;//���������� �������������� ����, ���� �������� ������������ ��� SID

/*���������� ��� ����������� ����� � SID*/
LPWSTR RefDomainName = NULL;
PSID psid = NULL;
DWORD cbSID = 0, cchRefDomainName = 0;

LookupAccountNameW(NULL, AccountName, NULL, &cbSID, NULL, &cchRefDomainName, NULL);//����������� �������� ������ ��� �����

if ((cbSID > 0) && (cchRefDomainName > 0))
{
	psid = (PSID)LocalAlloc(LMEM_FIXED, cbSID); //��������� ������ �� ��������� ���� ��������
	RefDomainName = (LPWSTR)LocalAlloc(LMEM_FIXED, cchRefDomainName * sizeof(WCHAR));// -||- ��� ����� ������
}

if ((psid != NULL) && (RefDomainName != NULL))
{
	RetRes = LookupAccountNameW(NULL, AccountName, psid, &cbSID, RefDomainName, &cchRefDomainName, &SidType);
}

if (RetRes != FALSE)
{
	*ppsid = psid;
}
else
{
	if (psid != NULL)
	{
		LocalFree(psid);//����������� ������
	}
}

if (RefDomainName != NULL)
{
	LocalFree(RefDomainName);//����������� ������
}

return RetRes;
}
BOOL GetAccountName_W(PSID psid, LPWSTR* AccountName)
{
	BOOL RetRes = FALSE;
	SID_NAME_USE SidType;//���������� �������������� ����, ���� �������� ������������ ��� SID
	/*���������� ��� ������*/
	LPWSTR Name = NULL;
	DWORD cch = 0, cchRefDomainName = 0;

	if (IsValidSid(psid) == FALSE)
	{
		return FALSE;
	}

	LookupAccountSid(NULL, psid, NULL, &cch, NULL, &cchRefDomainName, NULL);//��������� ������� �������
	DWORD cb = (cch + cchRefDomainName) * sizeof(WCHAR);
	if (cb > 0)
	{
		Name = (LPWSTR)LocalAlloc(LMEM_FIXED, cb);//��������� ������ �� ��������� ���� ��������
	}

	if (Name != NULL)
	{
		RetRes = LookupAccountSid(NULL, psid, Name + cchRefDomainName, &cch, Name, &cchRefDomainName, &SidType);
	}

	if (RetRes != FALSE)
	{
		if (SidTypeDomain != SidType)
		{
			if (cchRefDomainName > 0)
			{
				Name[cchRefDomainName] = '\\';
			}
			else
			{
				std::wcscpy(Name, Name + 1);//����������� ��� �������� � ������� ��������
			}
		}
		*AccountName = Name; //������ ����������� ��� � ���������
	}
	else
	{
		ConvertSidToStringSidW(psid, AccountName);//���� �� ���������� �������� ���, �� ������ SID
		if (Name != NULL)
		{
			LocalFree(Name);
		}
	}
	return RetRes;
}
BOOL GetWellKnowSID(WELL_KNOWN_SID_TYPE WellKnowSidType, PSID sidDomain, PSID *sidWellKnow)
{
	DWORD MaxSidSize = SECURITY_MAX_SID_SIZE;

	PSID localSID = (PSID)LocalAlloc(LMEM_FIXED, MaxSidSize);

	if (localSID == NULL)
	{
		return FALSE; //����������� SID �� �������. 
	}

	BOOL RetRes = CreateWellKnownSid(WellKnowSidType, sidDomain, localSID, &MaxSidSize);
	if (RetRes != FALSE)
	{
		*sidWellKnow = localSID;//��������� ��������� SID
	}
	else
	{
		LocalFree(localSID);
	}
	return RetRes;
}



void ListOfPrivilegesAndRights_User(LSA_HANDLE PolicyHandle, PSID sidUser)
{
	/*���������� ��� ������*/
	PLSA_UNICODE_STRING List_Rights; //������ ����
	WCHAR DisplayName[256];//������������� ���

	/*��������������� ��������*/
	DWORD count_char;//������� ���������� �������� � ������
	DWORD lpLanguageID;//ID �����?
	ULONG count_list;//�������� � ������ ����

	BOOL RetRes = FALSE;

	NTSTATUS ntstatus = LsaEnumerateAccountRights(PolicyHandle, sidUser, &List_Rights, &count_list);
	if (LSA_SUCCESS(ntstatus))
	{
		std::cout << "������ ���� ������� �������" << std::endl;
		for (ULONG i = 0; i < count_list; ++i)
		{
			LPCWSTR UserRight = List_Rights[i].Buffer;
			std::cout << "\t" << i + 1 << "  " << UserRight;

			count_char = _countof(DisplayName);//���������� ���������� �������� � ������

			RetRes = LookupPrivilegeDisplayName(NULL, UserRight, DisplayName, &count_char, &lpLanguageID);

			if (RetRes != FALSE)
			{
				std::cout << DisplayName << std::endl;
			}
			else
			{
				/*��������� ���� ������� �������*/
				constexpr LPCWSTR Right_array[20] =
				{
					SE_INTERACTIVE_LOGON_NAME, TEXT("��������� ���� � �������"),
					SE_DENY_INTERACTIVE_LOGON_NAME, TEXT("��������� ��������� ����"),
					SE_NETWORK_LOGON_NAME, TEXT("������ � ���������� �� ����"),
					SE_DENY_NETWORK_LOGON_NAME, TEXT("�������� � ������� � ����� ����������"),
					SE_BATCH_LOGON_NAME, TEXT("���� � �������� ��������� �������"),
					SE_DENY_BATCH_LOGON_NAME, TEXT("�������� �� ����� � �������� ��������� �������"),
					SE_SERVICE_LOGON_NAME, TEXT("���� � �������� ������"),
					SE_DENY_SERVICE_LOGON_NAME, TEXT("�������� �� ����� � �������� ������"),
					SE_REMOTE_INTERACTIVE_LOGON_NAME, TEXT("��������� ���� � ������� ����� ������ ��������� ������� ������"),
					SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME, TEXT("��������� ���� � ������� ����� ������ ��������� ������� ������")
				};

				for (int j = 0; j < _countof(Right_array); j+=2)
				{
					if ((wcscmp(Right_array[j], UserRight) == 0))
					{
						std::cout<<Right_array[j+1];
					}
				}
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		LsaFreeMemory(List_Rights);
	}
}



void CoutSID(PSID psid)
{
	LPWSTR lpSID = NULL, AccountName = NULL;//���������� ��� ������
	ConvertSidToStringSidW(psid, &lpSID);
	GetAccountName_W(psid, &AccountName);
	if ((AccountName != NULL) && (psid != NULL))
	{
		std::wcout << AccountName << "  " << psid << std::endl;
	}
	/*������� ������?*/
	FreeSid(psid);
	//delete AccountName;
}

