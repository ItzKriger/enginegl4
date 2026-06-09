#pragma once
#include <vector>
#include <string>
#include <limits>

#include "sol/sol.hpp"
#include "U_String.h"

class CCommandArgsWrapper
{
public:
    CCommandArgsWrapper() = default;
    CCommandArgsWrapper(const std::vector<std::wstring>& _RealArgs);
	std::vector<std::wstring> RealArgs;

	std::wstring GetArgument(size_t index) const;
	
	template<typename T>
    T WrapArgument(size_t index) const
    {
        return StringUtils::FromStr<T>(GetArgument(index));
    }

    template<typename T>
    T WrapArgumentOr(size_t index, T fallback = T{}) const
    {
        if(!IsArgumentAvailable(index)) { return fallback; }
        return StringUtils::FromStr<T>(GetArgument(index));
    }

    std::wstring operator[](size_t index) const
    {
        return GetArgument(index);
    }

    std::wstring& operator[](size_t index)
    {
        return RealArgs.at(index);
    }

    std::wstring at(size_t index) const
    {
        return GetArgument(index);
    }

    std::wstring& at(size_t index)
    {
        return RealArgs.at(index);
    }

    sol::as_table_t<std::vector<std::wstring>> ToLua() const;
    sol::table ToLua(sol::state_view st) const;

    bool IsEmpty() const;
    size_t GetSize() const;
    bool IsArgumentAvailable(size_t index) const;
    bool IsArgumentEmpty(size_t index) const;

    std::wstring Merge(size_t startindex = std::numeric_limits<size_t>::max(), size_t endindex = std::numeric_limits<size_t>::max()) const;

    bool empty() const;
    size_t size() const;
};
