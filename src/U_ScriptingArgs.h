#pragma once
#include <stdint.h>
#include "sol/sol.hpp"

namespace ScriptingArgs
{
    template <typename... Args, std::size_t... I>
    auto extract_args_impl(sol::variadic_args args, std::index_sequence<I...>)
    {
        return std::make_tuple(args.template get<Args>(I)...);
    }

    template <typename... Args>
    auto extract_args(sol::variadic_args args)
    {
        return extract_args_impl<Args...>(args, std::index_sequence_for<Args...>{});
    }
}
