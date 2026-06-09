#include "CAngle.h"
#include "U_Log.h"
#include "CScriptFieldsManager.h"
#include "U_Scripting.h"

CAngle::CAngle()
{
	m_degrees = 0.0f;
}

CAngle::CAngle(float rad)
{
	m_degrees = AngleUtils::normalizeTo360(AngleUtils::Rad2Deg(rad));
}

CAngle::CAngle(const CAngle& copy)
{
	m_degrees = copy.m_degrees;
}

CAngle::operator float() const { return asRadians(); }

float CAngle::asDegrees() const
{
	return m_degrees;
}

float CAngle::asRadians() const
{
	return AngleUtils::Deg2Rad(m_degrees);
}

float CAngle::asDegrees180() const
{
	return AngleUtils::normalizeTo180(m_degrees);
}

float CAngle::asRadians180() const
{
	return AngleUtils::Deg2Rad(AngleUtils::normalizeTo180(m_degrees));
}

void CAngle::wrap_inner_value()
{
	m_degrees = AngleUtils::normalizeTo360(m_degrees);
}

void CAngle::clamp360deg(float degmin, float degmax)
{
	m_degrees = glm::clamp(m_degrees, degmin, degmax);
}

void CAngle::clamp180deg(float degmin, float degmax)
{
	float wrapped180 = asDegrees180();
	wrapped180 = glm::clamp(wrapped180, degmin, degmax);

	setDegrees(wrapped180);
}

void CAngle::clamp360rad(float radmin, float radmax)
{
	float rad360 = asRadians();
	rad360 = glm::clamp(rad360, radmin, radmax);

	setRadians(rad360);
}

void CAngle::clamp180rad(float radmin, float radmax)
{
	float rad180 = asRadians180();
	rad180 = glm::clamp(rad180, radmin, radmax);

	setRadians(rad180);
}

CAngle& CAngle::operator=(const CAngle& right) { m_degrees = const_cast<CAngle&>(right).m_degrees; return *this; }

bool CAngle::operator==(const CAngle& right) const { return m_degrees == right.m_degrees; }
bool CAngle::operator!=(const CAngle& right) const { return m_degrees != right.m_degrees; }
bool CAngle::operator<(const CAngle& right) const { return m_degrees < right.m_degrees; }
bool CAngle::operator>(const CAngle& right) const { return m_degrees > right.m_degrees; }
bool CAngle::operator<=(const CAngle& right) const { return m_degrees <= right.m_degrees; }
bool CAngle::operator>=(const CAngle& right) const { return m_degrees >= right.m_degrees; }

CAngle CAngle::operator+(const CAngle& right) const { CAngle angl; angl.m_degrees = m_degrees; angl += right; return angl; }
CAngle& CAngle::operator+=(const CAngle& right) { m_degrees += right.m_degrees; wrap_inner_value(); return *this; }
CAngle CAngle::operator-(const CAngle& right) const { CAngle angl; angl.m_degrees = m_degrees; angl -= right; return angl; }
CAngle& CAngle::operator-=(const CAngle& right) { m_degrees -= right.m_degrees; wrap_inner_value(); return *this; }
CAngle CAngle::operator*(const CAngle& right) const { CAngle angl; angl.m_degrees = m_degrees; angl *= right; return angl; }
CAngle& CAngle::operator*=(const CAngle& right) { m_degrees *= right.m_degrees; wrap_inner_value(); return *this; }
CAngle CAngle::operator/(const CAngle& right) const { CAngle angl; angl.m_degrees = m_degrees; angl /= right; return angl; }
CAngle& CAngle::operator/=(const CAngle& right) { m_degrees /= right.m_degrees; wrap_inner_value(); return *this; }

CAngle CAngle::operator-() const
{
	return CAngle::degrees(-CAngle::asDegrees180());
}

CAngle CAngle::degrees(float angl)
{
	return { AngleUtils::Rad2Deg(angl) };
}

CAngle CAngle::radians(float angl)
{
	return { angl };
}

void CAngle::setDegrees(float deg)
{
	m_degrees = AngleUtils::normalizeTo360(deg);
}

void CAngle::setRadians(float deg)
{
	m_degrees = AngleUtils::normalizeTo360(AngleUtils::Rad2Deg(deg));
}

bool CAngle::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
	auto fieldsMan = CScriptFieldsManager::CreateFieldsManager(table);

	auto scDegrees = std::make_unique<CFunctionalScriptField>(
	[this](sol::state_view st) -> sol::object
	{
		return ScriptUtils::ToObject<float>(asDegrees(), st);
	},
	[this](sol::object obj) -> void
	{
		setDegrees(obj.as<float>());
	});

	auto scDegrees180 = std::make_unique<CFunctionalScriptField>(
	[this](sol::state_view st) -> sol::object
	{
		return ScriptUtils::ToObject<float>(asDegrees180(), st);
	},
	[this](sol::object obj) -> void
	{
		setDegrees(obj.as<float>());
	});

	auto scRadians = std::make_unique<CFunctionalScriptField>(
	[this](sol::state_view st) -> sol::object
	{
		return ScriptUtils::ToObject<float>(asRadians(), st);
	},
	[this](sol::object obj) -> void
	{
		setRadians(obj.as<float>());
	});

	auto scRadians180 = std::make_unique<CFunctionalScriptField>(
	[this](sol::state_view st) -> sol::object
	{
		return ScriptUtils::ToObject<float>(asRadians180(), st);
	},
	[this](sol::object obj) -> void
	{
		setRadians(obj.as<float>());
	});

	fieldsMan->AddField("degrees", std::move(scDegrees));
	fieldsMan->AddField("degrees180", std::move(scDegrees180));
	fieldsMan->AddField("radians", std::move(scRadians));
	fieldsMan->AddField("radians180", std::move(scRadians180));

	table.set("__type", "angle");
	auto meta = fieldsMan->CreateMetaTable(table);

	meta.set_function("__add", [table](sol::object obj_right, sol::this_state ts) mutable -> sol::object
	{
		CAngle angle_left = ScriptUtils::FromObject<CAngle>(table);
		CAngle angle_right = ScriptUtils::FromObject<CAngle>(obj_right);
		return ScriptUtils::ToObject<CAngle>(angle_left + angle_right, ts);
	});

	meta.set_function("__sub", [table](sol::object obj_right, sol::this_state ts) mutable -> sol::object
	{
		CAngle angle_left = ScriptUtils::FromObject<CAngle>(table);
		CAngle angle_right = ScriptUtils::FromObject<CAngle>(obj_right);
		return ScriptUtils::ToObject<CAngle>(angle_left - angle_right, ts);
	});

	meta.set_function("__mul", [table](sol::object obj_right, sol::this_state ts) mutable -> sol::object
	{
		CAngle angle_left = ScriptUtils::FromObject<CAngle>(table);
		CAngle angle_right = ScriptUtils::FromObject<CAngle>(obj_right);
		return ScriptUtils::ToObject<CAngle>(angle_left * angle_right, ts);
	});

	meta.set_function("__div", [table](sol::object obj_right, sol::this_state ts) mutable -> sol::object
	{
		CAngle angle_left = ScriptUtils::FromObject<CAngle>(table);
		CAngle angle_right = ScriptUtils::FromObject<CAngle>(obj_right);
		return ScriptUtils::ToObject<CAngle>(angle_left / angle_right, ts);
	});

	meta.set_function("__unm", [table](sol::this_state ts) mutable -> sol::object
	{
		CAngle angle_left = ScriptUtils::FromObject<CAngle>(table);
		return ScriptUtils::ToObject<CAngle>(-angle_left, ts);
	});

	return true;
}

CAngles::~CAngles() {} //HACK

bool CAngles::V_ScriptInit(std::shared_ptr<sol::state> _state, sol::table table)
{
	sol::state_view state(table.lua_state());

	sol::table pitch = state.create_table();
	sol::table yaw = state.create_table();
	sol::table roll = state.create_table();

	table.set_function("toQuat", [table](sol::this_state ts) mutable -> sol::object
	{
		CAngles self_euler = ScriptUtils::FromObject<CAngles>(table);
		glm::quat quat = AngleUtils::AnglesToQuat(self_euler);
		return ScriptUtils::ToObject<glm::quat>(quat, ts);
	});

	pitch = Pitch.GetScriptTable(_state);
	yaw = Yaw.GetScriptTable(_state);
	roll = Roll.GetScriptTable(_state);

	table[1] = table["x"] = table["pitch"] = pitch;
	table[2] = table["y"] = table["yaw"] = yaw;
	table[3] = table["z"] = table["roll"] = roll;

	table.set("__type", "angles");
	return true;
}
