#pragma once
#include <numbers>
#include <cmath>
#include <string>
#include "glm/gtc/quaternion.hpp"

class CAngle;
class CAngles;

namespace AngleUtils
{
    template<typename Real = float>
    float normalizeTo360(Real angle)
    {
        angle = fmod(angle, 360.0f);
        if (angle < 0.0f)
        {
            angle += 360.0f;
        }
        return angle;
    }

    template<typename Real = float>
    float normalizeTo180(Real angle)
    {
        angle = fmod(angle + 180.0f, 360.0f);
        if (angle < 0.0f)
        {
            angle += 360.0f;
        }
        return angle - 180.0f;
    }

    template<typename Real = float>
    Real Deg2Rad(Real deg)
    {
        return (deg * std::numbers::pi) / 180.0f;
    }

    template<typename Real = float>
    Real Rad2Deg(Real rad)
    {
        return (rad * 180.0f) / std::numbers::pi;
    }

    glm::quat AnglesToQuat(const CAngles& angles);
    CAngles QuatToAngles(const glm::quat& quat);

    std::string ToStrDegrees(const CAngles& angles);
    CAngles FromStrDegrees(const std::string& str);
}
