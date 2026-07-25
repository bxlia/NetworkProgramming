#include<Windows.h>
#include<iostream>

using std::cin;
using std::cout;
using std::endl;

BOOL finish = FALSE;
HANDLE g_hMutex = NULL;

void Plus()
{
	while (!finish)
	{
		WaitForSingleObject(g_hMutex, INFINITE);
		cout << "+ ";
		Sleep(100);
		ReleaseMutex(g_hMutex);
	}
}
void Minus()
{
	while (!finish)
	{
		WaitForSingleObject(g_hMutex, INFINITE);
		cout << "- ";
		Sleep(100);
		ReleaseMutex(g_hMutex);
	}
}

void main()
{
	setlocale(LC_ALL, " ");
	//Plus();
	//Minus();
	g_hMutex = CreateMutex(NULL, NULL, "Mutex");
	HANDLE hThreads[2] = {};
	hThreads[0] = CreateThread
	(
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)Plus,
		NULL,
		NULL,
		NULL
	);
	hThreads[1] = CreateThread
	(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)Minus,
		NULL,
		0,
		NULL
	);
	cin.get();
	finish = TRUE;
	WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
	CloseHandle(hThreads[0]);
	CloseHandle(hThreads[1]);
	CloseHandle(g_hMutex);
}
