#pragma once
#include "U_General.h"

template<typename Type>
class CSwitchableBase
{
public:
	virtual ~CSwitchableBase() {}

	void Enable()
	{
		m_getSwitchVar() = true;
	}

	void Disable()
	{
		m_getSwitchVar() = false;
	}

	void ToggleEnabledState()
	{
		m_getSwitchVar() = !m_getSwitchVar();
	}

	void SetEnabledState(bool isenabled = true)
	{
		m_getSwitchVar() = isenabled;
	}

	Type IsEnabled() const
	{
		return GET_CONST_FIELD(Type&, CSwitchableBase, m_getSwitchVar());
	}
private:
	virtual Type& m_getSwitchVar() = 0;
};
