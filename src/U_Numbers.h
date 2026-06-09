#pragma once
#include <cmath>
#include <limits>

#include "CAngle.h"
#include "U_Types.h"

template<typename E = float, typename T = float>
bool almost_equal(T a, T b, E epsilon = std::numeric_limits<E>::epsilon())
{
    if (std::abs(a - b) <= epsilon)
    {
        return true;
    }
    
    return std::abs(a - b) <= epsilon * std::max(std::abs(a), std::abs(b));
}

template<typename E = float, typename T = float, glm::length_t L = 3, glm::qualifier Q = glm::highp>
bool almost_equal(glm::vec<L, T, Q> a, glm::vec<L, T, Q> b, E epsilon = std::numeric_limits<E>::epsilon())
{
    for(size_t i = 0; i < L; i++)
    {
        if(!almost_equal(a[i], b[i], epsilon)) { return false; }
    }
    return true;
}

template<typename E = float, typename T = float, glm::qualifier Q = glm::highp>
bool almost_equal(glm::qua<T, Q> a, glm::qua<T, Q> b, E epsilon = std::numeric_limits<E>::epsilon())
{
    for(size_t i = 0; i < glm::qua<T, Q>::length(); i++)
    {
        if(!almost_equal(a[i], b[i], epsilon)) { return false; }
    }
    return true;
}

template<typename E = float>
bool almost_equal(CAngles a, CAngles b, E epsilon = std::numeric_limits<E>::epsilon())
{
    for(size_t i = 0; i < CAngles::length(); i++)
    {
        if(!almost_equal(a[i], b[i], epsilon)) { return false; }
    }
    return true;
}

template<typename E = float>
bool almost_equal(CAngle a, CAngle b, E epsilon = std::numeric_limits<E>::epsilon())
{
    return almost_equal(a.asRadians(), b.asRadians(), epsilon);
}

template<typename E = float, glm::length_t C = 4, glm::length_t R = 4, typename T = float, glm::qualifier Q = glm::highp>
bool almost_equal(glm::mat<C, R, T, Q> a, glm::mat<C, R, T, Q> b, E epsilon = std::numeric_limits<E>::epsilon())
{
    for(size_t y = 0; y < T::col_type::length(); y++)
    {
        for(size_t x = 0; x < T::length(); x++)
        {
            if(!almost_equal(a[x][y], b[x][y], epsilon)) { return false; }
        }
    }
    return true;
}
