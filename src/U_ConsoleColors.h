#pragma once
#include "CColor.h"

enum class CConsoleColor
{
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    LightMagenta = 13,
    Yellow = 14,
    White = 15
};

namespace ConsoleColors
{
	namespace Internal
	{
		int GetBrightness(const CColorInt& color);
		CConsoleColor GetDominantColor(const CColorInt& color, int brightness);
	}

    CConsoleColor RGBToConsoleColor(const CColor& color);
    CColor ConsoleColorToRGB(CConsoleColor color);
}
