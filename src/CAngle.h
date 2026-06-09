#pragma once
#include "U_Angles.h"
#include "CScriptObject.h"

#include <stdexcept>
#include "sol/sol.hpp"

class CAngle : public CScriptObject
{
public:
	using value_type = float;
	static constexpr size_t length() { return 1; }

	CAngle();
	CAngle(float rad);
	CAngle(const CAngle& copy);

	static CAngle degrees(float angl);
	static CAngle radians(float angl);

	float asDegrees() const;
	float asRadians() const;
	float asDegrees180() const;
	float asRadians180() const;

	void clamp360deg(float degmin, float degmax);
	void clamp180deg(float degmin, float degmax);
	void clamp360rad(float radmin, float radmax);
	void clamp180rad(float radmin, float radmax);

	void setDegrees(float deg);
	void setRadians(float deg);

	CAngle& operator=(const CAngle& right);

	bool operator==(const CAngle& right) const;
	bool operator!=(const CAngle& right) const;
	bool operator<(const CAngle& right) const;
	bool operator>(const CAngle& right) const;
	bool operator>=(const CAngle& right) const;
	bool operator<=(const CAngle& right) const;

	CAngle operator+(const CAngle& right) const;
	CAngle& operator+=(const CAngle& right);
	CAngle operator-(const CAngle& right) const;
	CAngle& operator-=(const CAngle& right);
	CAngle operator*(const CAngle& right) const;
	CAngle& operator*=(const CAngle& right);
	CAngle operator/(const CAngle& right) const;
	CAngle& operator/=(const CAngle& right);

	CAngle operator-() const;
	operator float() const;

	bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
private:
	void wrap_inner_value();

	value_type m_degrees = 0.0f;
};

class CAngles : public CScriptObject
{
public:
	using value_type = CAngle::value_type;

	CAngles() : Pitch(0.0f), Yaw(0.0f), Roll(0.0f) {}
	CAngles(const CAngle& pitch, const CAngle& yaw, const CAngle& roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}
	CAngles(const CAngles& other) : Pitch(other.Pitch), Yaw(other.Yaw), Roll(other.Roll) {}

	CAngles(const glm::vec3& vec)
	{
		Pitch = CAngle::radians(vec.x);
		Yaw = CAngle::radians(vec.y);
		Roll = CAngle::radians(vec.z);
	}

	~CAngles();

	CAngles& operator=(const CAngles& other)
	{
		if (this != &other)
		{
			Pitch = other.Pitch;
			Yaw = other.Yaw;
			Roll = other.Roll;
		}
		return *this;
	}

	union
	{
		CAngle Pitch;
		CAngle x;
	};

	union
	{
		CAngle Yaw;
		CAngle y;
	};

	union
	{
		CAngle Roll;
		CAngle z;
	};

	CAngle& operator[](int index)
	{
		switch (index)
		{
			case 0: return Pitch;
			case 1: return Yaw;
			case 2: return Roll;
			default: throw std::out_of_range("Out of range index");
		}
	}

	CAngle operator[](int index) const
	{
		switch (index)
		{
			case 0: return Pitch;
			case 1: return Yaw;
			case 2: return Roll;
			default: throw std::out_of_range("Out of range index");
		}
	}

	operator glm::vec3() const
	{
		return glm::vec3(Pitch.asRadians(), Yaw.asRadians(), Roll.asRadians());
	}

	//always returns 3 in 3D world
	static constexpr size_t length() { return 3; }

	bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
};
