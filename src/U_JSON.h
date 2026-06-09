#pragma once
#include "nlohmann/json.hpp"
#include "VHACD.h"
#include "U_Types.h"

template<typename T>
T GetFromJson(const nlohmann::json& _json, const std::string& key, T fallback = T{})
{
    if(_json.contains(key))
    {
        auto& val = _json[key];
        if constexpr (std::is_arithmetic_v<T>)
        {
            if(val.is_number())
            {
                return val.get<T>();
            }
        }
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>)
        {
            if(val.is_string())
            {
                return val.get<T>();
            }
        }
        if constexpr (std::is_same_v<T, bool>)
        {
            if(val.is_boolean())
            {
                return val.get<T>();
            }
        }
        if constexpr (is_glm_vec_v<T> || is_glm_quat_v<T>)
        {
            if(val.is_array())
            {
                T vec;
                std::vector<typename T::value_type> nums = val.get<std::vector<typename T::value_type>>();

                for(size_t i = 0; i < nums.size() && i < T::length(); i++)
                {
                    vec[i] = nums.at(i);
                }
                return vec;
            }
        }
    }
    return fallback;
}

void JsonToVHACD(const nlohmann::json& _json, VHACD::IVHACD::Parameters& params);
