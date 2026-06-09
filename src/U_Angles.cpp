#include "U_Angles.h"
#include "CAngle.h"
#include "U_String.h"

glm::quat AngleUtils::AnglesToQuat(const CAngles& angles)
{
	return glm::quat(glm::vec3(angles.x.asRadians(), angles.y.asRadians(), angles.z.asRadians()));
}

CAngles AngleUtils::QuatToAngles(const glm::quat& quat)
{
	return glm::eulerAngles(quat);
}

std::string AngleUtils::ToStrDegrees(const CAngles& angles)
{
	return StringUtils::ToStr(angles.x.asDegrees()) + " " + StringUtils::ToStr(angles.y.asDegrees()) + " " + StringUtils::ToStr(angles.z.asDegrees());
}

CAngles AngleUtils::FromStrDegrees(const std::string& str)
{
	auto splitted = StringUtils::split_str(str, ' ');
	CAngles ret;

	for(size_t i = 0; i < splitted.size() && i < CAngles::length(); i++)
	{
		ret[i].setDegrees(StringUtils::FromStr<float>(splitted[i]));
	}
	return ret;
}
