#pragma once
#include "CTypedWrapableBase.h"
#include "ILuaWrapable.h"
#include "IBinarySerializableWrapable.h"

template<typename T>
class CWrapable : public virtual CTypedWrapableBase<T>, public virtual ILuaWrapable<T>, public virtual IBinarySerializableWrapable<T>
{
public:
	CWrapable() = default;

	CWrapable(const T& val)
	{
		CTypedWrapableBase<T>::SetValue(val);
	}

	void m_setValue(const T& val) override
	{
		Value = val;
	}

	T m_getValue() const override
	{
		return Value;
	}

	T Value{};
};
