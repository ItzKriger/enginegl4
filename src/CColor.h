#pragma once
#include "glm/vec4.hpp"

#include <stdexcept>

class CColorInt;
class CColor //TODO colors string conv, lua conv
{
public:
	using value_type = float;

	CColor(float _r = 0.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f);

	CColor(const CColorInt& intcolor);
    operator CColorInt() const;
    static CColor FromIntColor(const CColorInt& intcolor);
    void SetFromIntColor(const CColorInt& intcolor);
    CColorInt GetIntColor() const;

	void Negate(bool negatealpha = false);
	CColor GetNegated(bool negatealpha = false) const;

	glm::vec4 GetVector() const;
    glm::ivec4 GetIVector() const;

	operator glm::vec4() const;
	operator glm::ivec4() const;

	float& operator[](int index);
	float operator[](int index) const;

	union { float r, x; };
	union { float g, y; };
	union { float b, z; };
	union { float a, w; };

	static const CColor Black;
	static const CColor White;
	static const CColor Red;
	static const CColor Green;
	static const CColor Blue;
	static const CColor Yellow;
	static const CColor Magenta;
	static const CColor Cyan;
	static const CColor Transparent;

	static constexpr size_t length() { return 4; }
};

