#include "U_Platform.h"
#include <iostream>
#include <fstream>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#elif defined __linux__
#include <pthread.h>
#include <unistd.h>
#endif

void Crossplatform::CreateConsole()
{
#ifdef _WIN32
	AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
#elif defined __linux__
	system("xterm -hold -e bash &");
#endif
}

void Crossplatform::DestroyConsole()
{
#ifdef _WIN32
	FreeConsole();
#elif defined __linux__
	system("pkill xterm");
#endif
}

void Crossplatform::StopTerminalInput(std::thread::native_handle_type threadid)
{
#ifdef _WIN32
	
#elif defined __linux__
	pthread_cancel(threadid);
#endif
}

void Crossplatform::SetConsoleColors(CConsoleColor text, CConsoleColor background)
{
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, (WORD)(((int)background << 4) | (int)text));
#elif defined __linux__
	int ansiText = (int)text;
	int ansiBackground = (int)background;

	int textCode = (ansiText < 8) ? (30 + ansiText) : (90 + (ansiText - 8));
	int bgCode = (ansiBackground < 8) ? (40 + ansiBackground) : (100 + (ansiBackground - 8));

	std::cout << "\033[" << bgCode << ";" << textCode << "m";
#endif
}
