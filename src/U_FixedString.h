#pragma once
#include <utility>
#include <algorithm>
#include <string>
#include <string_view>

template <size_t N>
struct FixedString
{
    char value[N];

    constexpr FixedString(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    constexpr operator std::string_view() const
    {
        return { value, N - 1 }; //without null terminator
    }
};

//comparator to make use in templates
template <size_t N1, size_t N2>
constexpr bool operator==(const FixedString<N1>& lhs, const FixedString<N2>& rhs)
{
    if constexpr (N1 != N2) { return false; }
    for (size_t i = 0; i < N1; ++i)
    {
        if (lhs.value[i] != rhs.value[i])
        {
            return false;
        }
    }
    return true;
}
