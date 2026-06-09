#include "CCommandArgsWrapper.h"

CCommandArgsWrapper::CCommandArgsWrapper(const std::vector<std::wstring>& _RealArgs) : RealArgs(_RealArgs) {}

std::wstring CCommandArgsWrapper::GetArgument(size_t index) const
{
    if(index >= RealArgs.size()) { return {}; }
    return RealArgs.at(index);
}

size_t CCommandArgsWrapper::GetSize() const
{
    return RealArgs.size();
}

bool CCommandArgsWrapper::IsArgumentAvailable(size_t index) const
{
    return index < RealArgs.size();
}

bool CCommandArgsWrapper::IsArgumentEmpty(size_t index) const
{
    return (index < RealArgs.size()) ? RealArgs.at(index).empty() : true;
}

sol::as_table_t<std::vector<std::wstring>> CCommandArgsWrapper::ToLua() const
{
    return sol::as_table(RealArgs);
}

sol::table CCommandArgsWrapper::ToLua(sol::state_view st) const
{
    sol::table ret = st.create_table();

    size_t index = 1;
    for(auto& arg : RealArgs)
    {
        ret[index] = arg;
        index++;
    }

    return ret;
}

bool CCommandArgsWrapper::IsEmpty() const
{
    return RealArgs.empty();
}

bool CCommandArgsWrapper::empty() const
{
    return IsEmpty();
}

std::wstring CCommandArgsWrapper::Merge(size_t startindex, size_t endindex) const
{
    size_t start = std::min(startindex, endindex);
    size_t end = std::max(startindex, endindex);

    if(start == std::numeric_limits<size_t>::max())
    {
        start = 0;
    }

    if(end == std::numeric_limits<size_t>::max())
    {
        end = RealArgs.size() - 1;
    }

    if(RealArgs.empty() || start >= GetSize() || end >= GetSize()) { return {}; }
    return StringUtils::merge_arg<std::wstring>(RealArgs, start, end);
}

size_t CCommandArgsWrapper::size() const
{
    return GetSize();
}
