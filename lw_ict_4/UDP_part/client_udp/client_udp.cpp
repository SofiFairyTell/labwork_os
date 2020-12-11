#include <iostream>
#include <WS2tcpip.h>
#include <Windows.h> 
#include <Windowsx.h>
#include <CommCtrl.h> 
#include <tchar.h> 
#include <Psapi.h> 
#include <strsafe.h> 
#include <process.h>
// Include the Winsock library (lib) file
#pragma comment (lib, "ws2_32.lib")

// Saves us from typing std::cout << etc. etc. etc.
using namespace std;
#pragma pack(1)
struct SLP_msg
{
	int filelen;		 //����� ���������
	int numberfrag;		//����� ���������
	WCHAR username[20]; //��� �����������
	WCHAR text[10];		//����� ���������
};
#pragma pack()

void wmain(int argc, char* argv[]) // We can pass in a command line option!! 
{
	////////////////////////////////////////////////////////////
	// INITIALIZE WINSOCK
	////////////////////////////////////////////////////////////

	// Structure to store the WinSock version. This is filled in
	// on the call to WSAStartup()
	WSADATA data;

	// To start WinSock, the required version must be passed to
	// WSAStartup(). This server is going to use WinSock version
	// 2 so I create a word that will store 2 and 2 in hex i.e.
	// 0x0202
	WORD version = MAKEWORD(2, 2);

	// Start WinSock
	int wsOk = WSAStartup(version, &data);
	if (wsOk != 0)
	{
		// Not ok! Get out quickly
		cout << "Can't start Winsock! " << wsOk;
		return;
	}

	////////////////////////////////////////////////////////////
	// CONNECT TO THE SERVER
	////////////////////////////////////////////////////////////

	// Create a hint structure for the server
	sockaddr_in server;
	server.sin_family = AF_INET; // AF_INET = IPv4 addresses
	server.sin_port = htons(54000); // Little to big endian conversion
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr); // Convert from string to byte array

	// Socket creation, note that the socket type is datagram
	SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

	// Write out to that socket
	string s(argv[1]); //username
	string text(argv[2]); //���������

	WCHAR usernames[20] = L"";
	WCHAR text[255] = L"";
	

	SLP_msg msg; //����� ��� �������� ���������
	memset(msg.text, NULL, 10);
	const CHAR * username = s.c_str();
	const CHAR * text_c = text.c_str();

	StringCchPrintf(msg.username, 20, (WCHAR*)username); //�������� ����� � ���������
	StringCchPrintf(msg.text, 20,(WCHAR*)text_c); //�������� ������ � ���������

	int n = 0;
	int lentext = text.length;//���������� ����� ������ 
	msg.filelen = lentext;
	int num; //����� ������?? ����������??
	int fragment = (lentext / 2); //����� 10 ����, ������ �� 5 ����?

	if (lentext % 2 == 0)
	{
		num = 1;
	}
	else num = 0;
	int sendOk;
	for (int i = 0; i < lentext; i += 2)
	{
		WCHAR frag[2] = L"";
		for (int j = 0; j < 2; j++)
		{
			frag[j] = text[j + i];
		}
		msg.numberfrag = num;
		num++;
		StringCchPrintf(msg.text, 6, frag);
		sendOk = sendto(out, (const char*)&msg, sizeof(msg), NULL, (sockaddr*)&server, sizeof(server));
		ZeroMemory(msg.text, 10);
	}

	//int sendOk = sendto(out, s.c_str(), s.size() + 1, 0, (sockaddr*)&server, sizeof(server));

	if (sendOk == SOCKET_ERROR)
	{
		cout << "That didn't work! " << WSAGetLastError() << endl;
	}

	// Close the socket
	closesocket(out);

	// Close down Winsock
	WSACleanup();
}