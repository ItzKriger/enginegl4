#pragma once
#include "CFactoryInitterBase.h"
#include "CObjectFactory.h"
#include <functional>

#include "U_Macros.h"

template<typename T, typename Base, bool early = false>
class CFactoryInitter : public CFactoryInitterBase
{
public:
	CFactoryInitter() = delete;
	CFactoryInitter(std::string _name, CObjectFactory<Base, std::string>* factory = nullptr)
	{
		m_name = _name;
		m_factory = factory;

		if constexpr (!early)
		{
			Initters.push_back(this);
		}
		else
		{
			EarlyInitters.push_back(this);
		}
	}

	CFactoryInitter(std::string _name, std::function<void(CFactoryInitter<T, Base, early>*)> func)
	{
		m_name = _name;
		m_factory = nullptr;
		PreProcess = func;

		if constexpr (!early)
		{
			Initters.push_back(this);
		}
		else
		{
			EarlyInitters.push_back(this);
		}
	}

	void Process() override
	{
		if (PreProcess) { PreProcess(this); }
		if (m_factory) { m_factory->template add<T>(m_name); }
	}

	std::function<void(CFactoryInitter<T, Base, early>*)> PreProcess;
	CObjectFactory<Base, std::string>* m_factory = nullptr;
};
