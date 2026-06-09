#include "CColorInt.h"
#include "CColor.h"

CColorInt::CColorInt(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) { r = _r; g = _g; b = _b; a = _a; }

void CColorInt::Negate(bool negatealpha)
{
	r = 255 - r;
	g = 255 - g;
	b = 255 - b;
	if (negatealpha) { a = 255 - a; }
}

CColorInt CColorInt::GetNegated(bool negatealpha) const
{
	CColorInt ret{ r, g, b, a };
	ret.Negate(negatealpha);
	return ret;
}

glm::vec4 CColorInt::GetVector() const { return GetNormalized(); }
glm::ivec4 CColorInt::GetIVector() const { return { r, g, b, a }; }

CColorInt::operator glm::vec4() const
{
	return GetVector();
}

CColorInt::operator glm::ivec4() const
{
	return GetIVector();
}

CColorInt::CColorInt(const CColor& floatcolor) { SetFromNormalized(floatcolor); }
CColorInt::operator CColor() const { return GetNormalized(); }
CColorInt CColorInt::FromNormalized(const CColor& floatcolor) { return floatcolor.GetIntColor(); }

void CColorInt::SetFromNormalized(const CColor& floatcolor)
{
    r = static_cast<typename CColorInt::value_type>(floatcolor.r * 255.0f);
    g = static_cast<typename CColorInt::value_type>(floatcolor.g * 255.0f);
    b = static_cast<typename CColorInt::value_type>(floatcolor.b * 255.0f);
    a = static_cast<typename CColorInt::value_type>(floatcolor.a * 255.0f);
}

CColor CColorInt::GetNormalized() const
{
    return
    {
        static_cast<typename CColor::value_type>(r) / 255.0f,
        static_cast<typename CColor::value_type>(g) / 255.0f,
        static_cast<typename CColor::value_type>(b) / 255.0f,
        static_cast<typename CColor::value_type>(a) / 255.0f,
    };
}

unsigned char& CColorInt::operator[](int index)
{
    switch (index)
    {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: throw std::out_of_range("Out of range index");
    }
}

unsigned char CColorInt::operator[](int index) const
{
    switch (index)
    {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: throw std::out_of_range("Out of range index");
    }
}

const CColorInt CColorInt::Black(0, 0, 0);
const CColorInt CColorInt::White(255, 255, 255);
const CColorInt CColorInt::Red(255, 0, 0);
const CColorInt CColorInt::Green(0, 255, 0);
const CColorInt CColorInt::Blue(0, 0, 255);
const CColorInt CColorInt::Yellow(255, 255, 0);
const CColorInt CColorInt::Magenta(255, 0, 255);
const CColorInt CColorInt::Cyan(0, 255, 255);
const CColorInt CColorInt::Transparent(0, 0, 0, 0);