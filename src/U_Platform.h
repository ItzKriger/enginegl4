#pragma once
#include <thread>
#include "U_ConsoleColors.h"

namespace Crossplatform
{
	void CreateConsole();
	void DestroyConsole();
	void StopTerminalInput(std::thread::native_handle_type threadid);

	void SetConsoleColors(CConsoleColor text, CConsoleColor background);
}
