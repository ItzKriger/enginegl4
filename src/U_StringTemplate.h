#pragma once
#include <string>
#include <type_traits>

inline std::string normalize_string(const char* str)
{
    return std::string(str);
}

inline std::wstring normalize_string(const wchar_t* str)
{
    return std::wstring(str);
}

template <std::size_t N>
inline std::string normalize_string(const char (&str)[N])
{
    return std::string(str);
}

template <std::size_t N>
inline std::wstring normalize_string(const wchar_t (&str)[N])
{
    return std::wstring(str);
}

inline std::string normalize_string(const std::string& str)
{
    return str;
}

inline std::wstring normalize_string(const std::wstring& str)
{
    return str;
}

template <typename T>
inline T normalize_string(const T& value)
{
    return value;
}
