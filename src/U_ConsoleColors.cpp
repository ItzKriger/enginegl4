#include "U_ConsoleColors.h"
#include "CColorInt.h"

int ConsoleColors::Internal::GetBrightness(const CColorInt& color)
{
    //CColorInt color = _color.GetIntColor();
    return (color.r + color.g + color.b) / 3;
}

CConsoleColor ConsoleColors::Internal::GetDominantColor(const CColorInt& color, int brightness)
{
    //CColorInt color = _color.GetIntColor();
    if (brightness < 128)
    {
        if (color.r > color.g && color.r > color.b) { return CConsoleColor::Red; }
        if (color.g > color.r && color.g > color.b) { return CConsoleColor::Green; }
        if (color.b > color.r && color.b > color.g) { return CConsoleColor::Blue; }
        return CConsoleColor::Black;
    }
    else
    {
        if (color.r > color.g && color.r > color.b) { return CConsoleColor::LightRed; }
        if (color.g > color.r && color.g > color.b) { return CConsoleColor::LightGreen; }
        if (color.b > color.r && color.b > color.g) { return CConsoleColor::LightBlue; }
        return CConsoleColor::LightGray;
    }
}

CConsoleColor ConsoleColors::RGBToConsoleColor(const CColor& _color)
{
    CColorInt color = _color.GetIntColor();
    int brightness = Internal::GetBrightness(color);

    if (color.r == color.g && color.g == color.b)
    {
        if (brightness < 64) { return CConsoleColor::Black; }
        if (brightness < 128) { return CConsoleColor::DarkGray; }
        if (brightness < 192) { return CConsoleColor::LightGray; }
        return CConsoleColor::White;
    }
    else
    {
        if (color.r > 128 && color.g > 128 && color.b < 128)
        {
            return CConsoleColor::Yellow;
        }

        CConsoleColor dominantColor = Internal::GetDominantColor(color, brightness);

        if (dominantColor == CConsoleColor::Red)
        {
            if (color.g > 128 && color.b > 128) { return CConsoleColor::Magenta; }
            if (color.g > 128) { return CConsoleColor::Yellow; }
            return CConsoleColor::Red;
        }
        else if (dominantColor == CConsoleColor::Green)
        {
            if (color.b > 128) { return CConsoleColor::Cyan; }
            return CConsoleColor::Green;
        }
        else if (dominantColor == CConsoleColor::Blue)
        {
            return CConsoleColor::Blue;
        }
        else if (dominantColor == CConsoleColor::LightRed)
        {
            if (color.g > 128 && color.b > 128) { return CConsoleColor::LightMagenta; }
            if (color.g > 128) { return CConsoleColor::Yellow; }
            return CConsoleColor::LightRed;
        }
        else if (dominantColor == CConsoleColor::LightGreen)
        {
            if (color.b > 128) { return CConsoleColor::LightCyan; }
            return CConsoleColor::LightGreen;
        }
        else if (dominantColor == CConsoleColor::LightBlue)
        {
            return CConsoleColor::LightBlue;
        }
    }

    return CConsoleColor::Black;
}

CColor ConsoleColors::ConsoleColorToRGB(CConsoleColor color)
{
    switch (color)
    {
        case CConsoleColor::Black:          return CColor::FromIntColor({ 0, 0, 0 });
        case CConsoleColor::Blue:           return CColor::FromIntColor({ 0, 0, 128 });
        case CConsoleColor::Green:          return CColor::FromIntColor({ 0, 128, 0 });
        case CConsoleColor::Cyan:           return CColor::FromIntColor({ 0, 128, 128 });
        case CConsoleColor::Red:            return CColor::FromIntColor({ 128, 0, 0 });
        case CConsoleColor::Magenta:        return CColor::FromIntColor({ 128, 0, 128 });
        case CConsoleColor::Brown:          return CColor::FromIntColor({ 128, 128, 0 });
        case CConsoleColor::LightGray:      return CColor::FromIntColor({ 192, 192, 192 });
        case CConsoleColor::DarkGray:       return CColor::FromIntColor({ 128, 128, 128 });
        case CConsoleColor::LightBlue:      return CColor::FromIntColor({ 0, 0, 255 });
        case CConsoleColor::LightGreen:     return CColor::FromIntColor({ 0, 255, 0 });
        case CConsoleColor::LightCyan:      return CColor::FromIntColor({ 0, 255, 255 });
        case CConsoleColor::LightRed:       return CColor::FromIntColor({ 255, 0, 0 });
        case CConsoleColor::LightMagenta:   return CColor::FromIntColor({ 255, 0, 255 });
        case CConsoleColor::Yellow:         return CColor::FromIntColor({ 255, 255, 0 });
        case CConsoleColor::White:          return CColor::FromIntColor({ 255, 255, 255 });
        default:                            return CColor::FromIntColor({ 0, 0, 0 });
    }
}