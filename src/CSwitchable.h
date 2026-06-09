#pragma once
#include "CSwitchableBase.h"

class CSwitchable : public virtual CSwitchableBase<bool>
{
public:
private:
	bool m_enabled = true;

	bool& m_getSwitchVar() override
	{
		return m_enabled;
	}
};
