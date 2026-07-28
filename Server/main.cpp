#ifndef win32_lean_and_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 

#include<Windows.h>
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>

#include<FormatLastError.h>
#include<Message.h>

using std::cin;
using std::cout;
using std::endl;

#define MTU 1500 //Maximum Transfel Unit
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "FormatLastError.lib")

#define MAX_CONNECTIONS 3
HANDLE g_hThreads[MAX_CONNECTIONS] = {};
DWORD g_dwThreadIDs[MAX_CONNECTIONS] = {};
SOCKET g_hSockets[MAX_CONNECTIONS] = {};
INT n = 0;

VOID ShowActiveClients();
VOID ClientHandler(SOCKET client_socket);

void main()
{
	setlocale(LC_ALL, " ");
	cout << "SERVER" << endl;

	INT iResult = 0;
	DWORD dwError = 0;
	CHAR szError[256] = {};

	//0) Инициализация WinSock:
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	//1) Параметры подключения:
	addrinfo hints;
	addrinfo* target;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // TCP/IP
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; // Соединение будет работать в режиме 'LISTENING'

	iResult = getaddrinfo(NULL, "27015", &hints, &target); // NULL - '0.0.0.0'. Сервер будет прослушивать порт '27015' на всех доступных IP-адресах, на всех сетевых картах, установленных на ПК.
	if (iResult) // '0' - это FALSE, TRUE - это все что НЕ '0'
	{
		cout << "getaddrinfo() failed with error: " << iResult << endl;
		cout << "Функция getaddrinfo() завершилась с ошибкой: " << iResult << endl;
		WSACleanup();
		return;
	}

	//2) Создаем SOCKET Сервера
	SOCKET listen_socket = socket(target->ai_family, target->ai_socktype, target->ai_protocol);
	dwError = WSAGetLastError();
	if (listen_socket == INVALID_SOCKET)
	{
		cout << "LISTEN SOCKET creation failed with error: " << WSAGetLastError() << endl;
		cout << "Создание функции LISTEN SOCKET завершилось с ошибкой: " << WSAGetLastError() << endl;
		cout << FormatLastError(dwError, szError) << endl;
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//3) BIND - привязываем серверный сокет к интерфейсу, который он будет слушать
	iResult = bind(listen_socket, target->ai_addr, target->ai_addrlen);
	dwError = WSAGetLastError();
	if (iResult)
	{
		cout << "BIND failed with error: " << WSAGetLastError() << endl;
		cout << "Привязка сокета к порту завершилась с ошибкой: " << WSAGetLastError() << endl;
		cout << FormatLastError(dwError, szError) << endl;
		closesocket(listen_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//4) Запускаем прослушивание порта
	if (listen(listen_socket, MAX_CONNECTIONS) == SOCKET_ERROR)
	{
		cout << FormatLastError(WSAGetLastError(), szError) << endl;
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		cout << "Прослушивать порт невозможно из-за ошибки: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(target);
		WSACleanup();
		return;
	}

	//5) Принимаем подключение от клиентов
	do
	{
		ShowActiveClients();
		sockaddr_in client_address;
		int client_address_len = sizeof(client_address);
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_address, &client_address_len);
		//cout << client_address.sa_data << endl;
		//cout << "Accept DONE" << endl;
		cout << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port);
		if (client_socket == INVALID_SOCKET)
		{
			cout << FormatLastError(WSAGetLastError(), szError) << endl;
			cout << "Accept failed with error: " << WSAGetLastError() << endl;
			cout << "Не удалось принять подключение от клиента: " << WSAGetLastError() << endl;
			closesocket(listen_socket);
			freeaddrinfo(target);
			WSACleanup();
			return;
		}if (n < MAX_CONNECTIONS)
		{

			//ClientHandler(client_socket);

			g_hSockets[n] = client_socket;
			g_hThreads[n] = CreateThread
			(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ClientHandler,
				(LPVOID)g_hSockets[n],
				NULL,
				g_dwThreadIDs + n
			);
			n++;
		}
		else
		{
			send(client_socket, DECLINE_MESSAGE, strlen(DECLINE_MESSAGE), NULL);
			if (iResult == SOCKET_ERROR) FormatLastError(WSAGetLastError(), szError);
			iResult = shutdown(client_socket, SD_BOTH);
			cout << " - DECLINE" << endl;
		}
	} while (true);
	//9) Освободить ресурсы
	closesocket(listen_socket);
	freeaddrinfo(target);
	WSACleanup();
}

VOID ShowActiveClients()
{
	Sleep(100);
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hConsole, &info);
	SetConsoleCursorPosition(hConsole, COORD{ 25, 0 });
	cout << "\t\t\t\t\t";
	SetConsoleCursorPosition(hConsole, COORD{ 25, 0 });
	cout << "Количество подключенных клиентов: " << n;
	SetConsoleCursorPosition(hConsole, info.dwCursorPosition);
}
INT GetClientPosition(DWORD dwID)
{
	for (int i = 0; i < MAX_CONNECTIONS; i++)
	{
		if (g_dwThreadIDs[i] == dwID)return i;
	}
}

VOID ClientHandler(SOCKET client_socket)
{
	SOCKADDR_IN client_address;
	client_address.sin_family = AF_INET; // TCP/IP
	INT namelen = sizeof(client_address);
	getpeername(client_socket, (SOCKADDR*) & client_address, &namelen);
	CHAR sz_client_address[32] = {};
	CHAR sz_client_connected[32] = {};
	sprintf(sz_client_address, "%s:%d - ", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	sprintf(sz_client_connected, "%s CONNECTED", sz_client_address);
	cout << "Client " << sz_client_connected << endl;

	INT iResult = 0;
	DWORD dwError = 0;
	CHAR szError[256] = {};
	//6) Получение данных от клиента
	do
	{
		CHAR recv_buffer[MTU] = {};
		CHAR send_buffer[MTU] = {};
		do
		{
			ZeroMemory(send_buffer, MTU);
			ZeroMemory(recv_buffer, MTU);
			iResult = recv(client_socket, recv_buffer, MTU, NULL);
			if (iResult > 0)
			{
				cout << sz_client_address << recv_buffer << ". (" << iResult << " Bytes);" << endl;
				cout << "\tThreadID: " << GetCurrentThreadId() << "\tPosition: " << GetClientPosition(GetCurrentThreadId());
				cout << endl;
				//cout << iResult << " Bytes received. Message: " << sz_client_address << recv_buffer << endl;

			}
			else if (iResult == 0) cout << "Nothing received, connection closing.\nНет данных от клиента, закрываем соединение" << endl;
			else
			{
				cout << FormatLastError(WSAGetLastError(), szError) << endl;
				cout << "Receive failed with error: " << WSAGetLastError() << endl;
				cout << "При получении данных возникла ошибка: " << WSAGetLastError() << endl;
				break;
			}

			//7) Отправка данных клиенту
			sprintf(send_buffer, "Привет Клиент, Ваше сообщение: %s", recv_buffer);
			iResult = send(client_socket, send_buffer, strlen(send_buffer), NULL);
			if (iResult == SOCKET_ERROR)
			{
				cout << FormatLastError(WSAGetLastError(), szError) << endl;
				cout << "send() failed with error: " << WSAGetLastError() << endl;
				cout << "При отправке данныхх возникла ошибка: " << WSAGetLastError() << endl;
			}
		} while (true);

		//cin.get(); // Ожидает нажатие клавиши 'Enter'

		//8) Закрываем соединение с клиентом
		cout << "Press 'Enter' to close connection" << endl;
		//cin.get();
		iResult = shutdown(client_socket, SD_BOTH);
		if (iResult) cout << FormatLastError(WSAGetLastError(), szError) << endl;
		if (iResult) cout << "shutdown failed with error: " << WSAGetLastError() << endl;
		closesocket(client_socket);
	} while (true);
}