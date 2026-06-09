#pragma once
#include "CTypedWrapableBase.h"
#include "IBinarySerializableWrapable.h"
#include "ILuaWrapable.h"
#include "CWrapableBindingBase.h"
#include "U_Pointers.h"
#include "U_String.h"

template<typename T>
class CWrapableBinding : public virtual CTypedWrapableBase<T>, public virtual ILuaWrapable<T>, public virtual IBinarySerializableWrapable<T>, public CWrapableBindingBase
{
public:
	CWrapableBinding(const T& val)
	{
		CTypedWrapableBase<T>::SetValue(val);
	}

	CWrapableBinding(T* ptr = nullptr)
	{
		Pointer = ptr;
		Update();
	}

	void Update() override
	{
		if (!Pointer) { return; }
		if ((*Pointer) != KnownValue)
		{
			CWrapableBase::OnStrValueChanged(this, StringUtils::ToStr<std::wstring>(KnownValue), StringUtils::ToStr<std::wstring>(*Pointer));
			CTypedWrapableBase<T>::OnValueChanged(this, KnownValue, (*Pointer));

			KnownValue = (*Pointer);
		}
	}

	void* GetPointer() override
	{
		return static_cast<void*>(Pointer);
	}

	void m_setValue(const T& val) override
	{
		(*Pointer) = KnownValue = val;
	}

	T m_getValue() const override
	{
		if (CWrapableBindingBase::ReturnCheck)
		{
			Update();
			return KnownValue;
		}
		return (*Pointer);
	}

	T KnownValue;
	T* Pointer = nullptr;
};
