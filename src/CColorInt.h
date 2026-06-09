#pragma once
#include "glm/glm.hpp"

#include <stdexcept>

class CColor;
class CColorInt //TODO colors string conv, lua conv
{
public:
	using value_type = unsigned char;
	
	CColorInt(unsigned char _r = 0, unsigned char _g = 0, unsigned char _b = 0, unsigned char _a = 255);

    CColorInt(const CColor& floatcolor);
    operator CColor() const;
    static CColorInt FromNormalized(const CColor& floatcolor);
    void SetFromNormalized(const CColor& floatcolor);
    CColor GetNormalized() const;

	void Negate(bool negatealpha = false);
	CColorInt GetNegated(bool negatealpha = false) const;

    glm::vec4 GetVector() const;
    glm::ivec4 GetIVector() const;

	operator glm::vec4() const;
	operator glm::ivec4() const;

    unsigned char& operator[](int index);
	unsigned char operator[](int index) const;

	union { unsigned char r, x; };
	union { unsigned char g, y; };
	union { unsigned char b, z; };
	union { unsigned char a, w; };

	static const CColorInt Black;
	static const CColorInt White;
	static const CColorInt Red;
	static const CColorInt Green;
	static const CColorInt Blue;
	static const CColorInt Yellow;
	static const CColorInt Magenta;
	static const CColorInt Cyan;
	static const CColorInt Transparent;

    static constexpr size_t length() { return 4; }
};
