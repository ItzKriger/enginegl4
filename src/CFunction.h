#pragma once
#include <functional>
#include "CFunctionBase.h"

template <typename ReturnValue, typename... Args>
class CFunction : public virtual CFunctionBase<ReturnValue, Args...>
{
public:
	CFunction(const std::function<ReturnValue(Args...)>& _func) : Function(_func) {}
	
	ReturnValue Call(Args... args) const override
	{
		return Function(args...);
	}

	bool IsValid() const override
	{
		return (bool)Function;
	}

	std::function<ReturnValue(Args...)> Function;
};
