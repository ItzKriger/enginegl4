#include "CColor.h"
#include "CColorInt.h"

CColor::CColor(float _r, float _g, float _b, float _a) { r = _r; g = _g; b = _b; a = _a; }

CColor::CColor(const CColorInt& intcolor) { SetFromIntColor(intcolor); }
CColor::operator CColorInt() const { return GetIntColor(); }
CColor CColor::FromIntColor(const CColorInt& intcolor) { return intcolor.GetNormalized(); }

void CColor::SetFromIntColor(const CColorInt& intcolor)
{
	r = static_cast<typename CColor::value_type>(intcolor.r) / 255.0f;
    g = static_cast<typename CColor::value_type>(intcolor.g) / 255.0f;
    b = static_cast<typename CColor::value_type>(intcolor.b) / 255.0f;
    a = static_cast<typename CColor::value_type>(intcolor.a) / 255.0f;
}

CColorInt CColor::GetIntColor() const
{
	return
	{
		static_cast<typename CColorInt::value_type>(r * 255.0f),
		static_cast<typename CColorInt::value_type>(g * 255.0f),
		static_cast<typename CColorInt::value_type>(b * 255.0f),
		static_cast<typename CColorInt::value_type>(a * 255.0f)
	};
}

void CColor::Negate(bool negatealpha)
{
	r = 1.0f - r;
	g = 1.0f - g;
	b = 1.0f - b;
	if (negatealpha) { a = 1.0f - a; }
}

CColor CColor::GetNegated(bool negatealpha) const
{
	CColor ret{ r, g, b, a };
	ret.Negate(negatealpha);
	return ret;
}

float& CColor::operator[](int index)
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

float CColor::operator[](int index) const
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

glm::vec4 CColor::GetVector() const { return { r, g, b, a }; }
glm::ivec4 CColor::GetIVector() const { return GetIntColor(); }

CColor::operator glm::vec4() const
{
	return GetVector();
}

CColor::operator glm::ivec4() const
{
	return GetIVector();
}

const CColor CColor::Black(CColor::FromIntColor({ 0, 0, 0 }));
const CColor CColor::White(CColor::FromIntColor({ 255, 255, 255 }));
const CColor CColor::Red(CColor::FromIntColor({ 255, 0, 0 }));
const CColor CColor::Green(CColor::FromIntColor({ 0, 255, 0 }));
const CColor CColor::Blue(CColor::FromIntColor({ 0, 0, 255 }));
const CColor CColor::Yellow(CColor::FromIntColor({ 255, 255, 0 }));
const CColor CColor::Magenta(CColor::FromIntColor({ 255, 0, 255 }));
const CColor CColor::Cyan(CColor::FromIntColor({ 0, 255, 255 }));
const CColor CColor::Transparent(CColor::FromIntColor({ 0, 0, 0, 0 }));