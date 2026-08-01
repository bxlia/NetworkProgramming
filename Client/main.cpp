#include<iostream>
#include<fstream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>

//#include<system_error>
//#include<string>

#include<FormatLastError.h>

// LINK2019 (LNK - Linker, Компоновщик) возникает в том случае, когда компоновщик видит прототип функции (SYMBOL), НО НЕ может сопоставить с ним реализацию функции.
// Это может бытьь из-за того, что список принимаемых параметров в прототипе и реализации отличается либо же реализация вообще нет.
// WS2_32.lib
#pragma comment(lib, "WS2_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "FormatLastError.lib")
#endif // _DEBUG

#ifdef NDEBUG
#ifdef _M_IX86
#pragma comment(lib, "FormatLastErrorx86.lib")
#elif _M_X64
#pragma comment(lib, "FormatLastErrorx64.lib")
#endif WIN32
#endif // NDEBUG

using std::cin;
using std::cout;
using std::endl;

#define MTU		1500 //Maximum Transfer Unit - максимальный блок данныхб который можно передать по сети для сетей семейства Ethernet MTU составляет 1500 Byte

BOOL finish = FALSE;
VOID Receive(SOCKET connect_socket);

void main()
{
	setlocale(LC_ALL, "");

	INT iResult;	//эта переменная будет хранить результаты работы функций
	DWORD dwError;
	CHAR szError[256] = {};

	//0) Инициализация WinSOCK:
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult)
	{
		cout << "WSAStartup failed with error: " << iResult << endl;
		return;
	}

	//1) Задаем параметры подключения:

	CONST INT IP_SIZE = 16;
	CHAR sz_server_ip[IP_SIZE] = {};
	std::ifstream fin("Client.ini");
	if (fin.is_open())
	{
		fin >> sz_server_ip;
		cout << "Server IP: " << sz_server_ip << endl;
		fin.close();

	}
	else
	{
		cout << "Error: 'Client.ini' not found" << endl;
		return;
	}

	addrinfo hints;
	addrinfo* target;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			//	TCP/IP
	hints.ai_socktype = SOCK_STREAM;	//SOCK_STREAM и IPPROTO_TCP говорят о том, что мы будет подключаться по протоколу TCP.
	hints.ai_protocol = IPPROTO_TCP;

	//iResult = getaddrinfo((PCSTR)INADDR_ANY, "27015", &hints, &target);	//127.0.0.1:27015
	iResult = getaddrinfo(sz_server_ip, "27015", &hints, &target);
	if (iResult)
	{
		cout << "getaddrinfo() failed with error " << iResult << endl;
		WSACleanup();
		return;
	}

	//2) Создаем сокет, при помощи которого будем подлючаться к Серверу:
	SOCKET connect_socket = socket(target->ai_family, target->ai_socktype, target->ai_protocol);
	dwError = WSAGetLastError();
	if (connect_socket == INVALID_SOCKET)
	{
		cout << FormatLastError(dwError, szError) << endl;
		cout << "Socket creation failed with error " << WSAGetLastError() << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//3) Подключаемся к Серверу:
	iResult = connect(connect_socket, target->ai_addr, target->ai_addrlen);
	dwError = WSAGetLastError();
	if (iResult)
	{
		cout << "Unable to connect to Server. Error: " << WSAGetLastError() << endl;
		cout << FormatLastError(dwError, szError) << endl;
		closesocket(connect_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}
	cout << "connect_socket: " << connect_socket << endl;
	freeaddrinfo(target);

	HANDLE hReceiveThread = CreateThread
	(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)Receive,
		(LPVOID)connect_socket,
		NULL,
		0
	);

	//4) Отправка данных на Сервер:
	CHAR send_buffer[MTU] = "Hello Server! How are you?";
	do
	{
		iResult = send(connect_socket, send_buffer, strlen(send_buffer), 0);
		dwError = WSAGetLastError();
		if (iResult == SOCKET_ERROR)
		{
			cout << "Send failed with error: " << WSAGetLastError() << endl;
			cout << FormatLastError(dwError, szError) << endl;
			closesocket(connect_socket);
			WSACleanup();
			return;
		}
		cout << "Sent " << iResult << " Bytes" << endl;

		//5) Получение данных от Сервера:
		//Receive(connect_socket);


		ZeroMemory(send_buffer, strlen(send_buffer));
		cout << "Введите сообщение: ";
		//cin >> send_buffer;
		SetConsoleCP(1251);
		cin.getline(send_buffer, MTU);
		SetConsoleCP(866);
	} while (strcmp(send_buffer, "exit"));
	finish = TRUE;
	WaitForSingleObject(hReceiveThread, INFINITE);
	CloseHandle(hReceiveThread);

	//6) Завершаем сеанс работы с Сервером и освобождаем ресурсы:
	iResult = shutdown(connect_socket, SD_BOTH);	//закрываем соединение с Сервером в обоих направлениях
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout
		<< "Shutdown failed with error: " << WSAGetLastError() << endl
		<< FormatLastError(dwError, szError);
	closesocket(connect_socket);
	WSACleanup();
}

VOID Receive(SOCKET connect_socket)
{
	cout << "connect_socket: " << connect_socket << endl;
	//cin.get();
	DWORD dwError = 0;
	CHAR szError[256] = {};
	INT iResult = 0;
	CHAR recv_buffer[MTU] = {};
	do
	{
		iResult = recv(connect_socket, recv_buffer, MTU, NULL);
		dwError = WSAGetLastError();
		if (iResult > 0)cout << iResult << "Byte received. Message: " << recv_buffer << endl;
		else if (iResult == 0)cout << "Nothing received." << endl;
		else cout
			<< "Receive failed with error: " << WSAGetLastError() << endl
			<< FormatLastError(dwError, szError);
		if (WSAGetLastError() == 10053)break;
	} while (!finish);
}