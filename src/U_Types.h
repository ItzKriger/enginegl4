#pragma once
#include <type_traits>
#include <string>
#include <map>
#include <unordered_map>

#include "glm/gtc/quaternion.hpp"
#include "glm/glm.hpp"
#include "CAngle.h"
//#include "CWrapableBase.h"
#include "U_General.h"
#include "CColor.h"
#include "CColorInt.h"
//#include "CWrapable.h"

#include "SDL3/SDL_events.h"

#define WRAP_TYPE_STRING_START(type, wrap) if constexpr (std::is_same_v<T, type>) { return wrap; }
#define WRAP_TYPE_STRING_END() else { return typeid(T).name(); }
#define WRAP_TYPE_STRING(type, wrap) else if constexpr (std::is_same_v<T, type>) { return wrap; }
#define WRAP_TYPE_STRING_GLM(cond, basename) else if constexpr (cond) { return basename + GetGlmLengthString<T>() + GetTypeNameFirstLetter<typename T::value_type>(); }
#define WRAP_TYPE_STRING_GLM_MAT(cond, basename) else if constexpr (cond) { return basename + GetGlmLengthString<T>() + "x" + GetGlmColLengthString<T>() + GetTypeNameFirstLetter<typename T::value_type>(); }
#define WRAP_TYPE_STRING_BASE(type, wrap) else if constexpr (are_same_template<T, type<T>>::value) { return wrap; }
#define WRAP_TYPE_STRING_BASE_WITH_TYPE(type, wrap) else if constexpr (are_same_template<T, type<T>>::value) { return wrap + GetTypeName<typename T::value_type>(); }
#define WRAP_TYPE_STRING_BASE_WITH_TYPE_FIRST_LETTER(type, wrap) else if constexpr (are_same_template<T, type<T>>::value) { return wrap + GetTypeNameFirstLetter<typename T::value_type>(); }
#define WRAP_TYPE_STRING_BASE_WITH_TYPE_SPACE(type, wrap) else if constexpr (are_same_template<T, type<T>>::value) { return wrap + " " + GetTypeName<typename T::value_type>(); }
#define WRAP_TYPE_STRING_BASE_WITH_TYPE_UNDERSCORE(type, wrap) else if constexpr (are_same_template<T, type<T>>::value) { return wrap + "_" + GetTypeName<typename T::value_type>(); }

template<typename T>
std::string GetTypeName();

template<typename T>
std::string GetTypeNameFirstLetter()
{
    return std::string(1, GetTypeName<T>()[0]);
}

template<typename T>
std::string GetGlmLengthString()
{
    return std::to_string(T::length());
}

template<typename T>
std::string GetGlmColLengthString()
{
    return std::to_string(T::col_type::length());
}

template<typename T>
struct is_glm_vec : std::false_type {};

template<glm::length_t L, typename T, glm::qualifier Q>
struct is_glm_vec<glm::vec<L, T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_vec_v = is_glm_vec<T>::value;

template<typename T>
struct is_glm_quat : std::false_type {};

template<typename T, glm::qualifier Q>
struct is_glm_quat<glm::qua<T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_quat_v = is_glm_quat<T>::value;

template<typename T>
struct is_glm_mat : std::false_type {};

template<glm::length_t X, glm::length_t Y, typename T, glm::qualifier Q>
struct is_glm_mat<glm::mat<X, Y, T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_mat_v = is_glm_mat<T>::value;

template<typename T>
struct is_glm_vec1 : std::false_type {};

template<typename T, glm::qualifier Q>
struct is_glm_vec1<glm::vec<1, T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_vec1_v = is_glm_vec1<T>::value;

template<typename T>
struct is_glm_vec2 : std::false_type {};

template<typename T, glm::qualifier Q>
struct is_glm_vec2<glm::vec<2, T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_vec2_v = is_glm_vec2<T>::value;

template<typename T>
struct is_glm_vec3 : std::false_type {};

template<typename T, glm::qualifier Q>
struct is_glm_vec3<glm::vec<3, T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_vec3_v = is_glm_vec3<T>::value;

template<typename T>
struct is_glm_vec4 : std::false_type {};

template<typename T, glm::qualifier Q>
struct is_glm_vec4<glm::vec<4, T, Q>> : std::true_type {};

template<typename T>
constexpr bool is_glm_vec4_v = is_glm_vec4<T>::value;

template<glm::length_t N, typename T>
struct is_glm_length_type : std::false_type {};

template<glm::length_t N, glm::length_t L, typename T, glm::qualifier Q>
struct is_glm_length_type<N, glm::vec<L, T, Q>> : std::bool_constant<L == N> {};

template<glm::length_t N, typename T>
constexpr bool is_glm_length_type_v = is_glm_length_type<N, T>::value;

template<typename T>
constexpr bool IsSequentParseable()
{
    return (is_glm_vec_v<T> || is_glm_quat_v<T> || std::is_same_v<T, CAngles> || std::is_same_v<T, CColor>);
}

template<typename T>
std::string GetTypeName()
{
    WRAP_TYPE_STRING_START(std::string, "string")

    WRAP_TYPE_STRING(std::wstring, "wstring")
    WRAP_TYPE_STRING(CAngle, "angle")
    WRAP_TYPE_STRING(CAngles, "angles")
    WRAP_TYPE_STRING(CColor, "color")
    WRAP_TYPE_STRING(CColorInt, "colorint")
    WRAP_TYPE_STRING_GLM(is_glm_vec_v<T>, "vec")
    WRAP_TYPE_STRING_GLM(is_glm_quat_v<T>, "quat")
    WRAP_TYPE_STRING_GLM_MAT(is_glm_mat_v<T>, "mat")
    WRAP_TYPE_STRING(std::int8_t, "int8")
    WRAP_TYPE_STRING(std::uint8_t, "uint8")
    WRAP_TYPE_STRING(std::int16_t, "int16")
    WRAP_TYPE_STRING(std::uint16_t, "uint16")
    WRAP_TYPE_STRING(std::int32_t, "int32")
    WRAP_TYPE_STRING(std::uint32_t, "uint32")
    WRAP_TYPE_STRING(std::int64_t, "int64")
    WRAP_TYPE_STRING(std::uint64_t, "uint64")
    WRAP_TYPE_STRING(float, "float")
    WRAP_TYPE_STRING(double, "double")
    WRAP_TYPE_STRING(bool, "bool")
    WRAP_TYPE_STRING(SDL_Event, "sdlevent")

    WRAP_TYPE_STRING_END()
    return "unknown";
} //TODO wrapables

#define TABLE_TYPE_STRING_DECLARE(name) extern std::unordered_map<size_t, std::string> name;
#define TABLE_TYPE_STRING_START(name) std::unordered_map<size_t, std::string> name = {
#define TABLE_TYPE_STRING_END() };
#define TABLE_TYPE_STRING(type, wrap) { typeid(type).hash_code(), wrap },
#define TABLE_TYPE_STRING_PRE_END(type, wrap) { typeid(type).hash_code(), wrap }
#define TABLE_TYPE_STRING_GLM(type, basename) { typeid(type).hash_code(), basename + GetGlmLengthString<typename type>() + GetTypeNameFirstLetter<typename type::value_type>() },
#define TABLE_TYPE_STRING_GLM_MAT(type, basename) { typeid(type).hash_code(), basename + GetGlmLengthString<typename type>() + "x" + GetGlmColLengthString<typename type>() + GetTypeNameFirstLetter<typename type::value_type>() },

TABLE_TYPE_STRING_DECLARE(TableStringTypes);
