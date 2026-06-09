#pragma once
#include <vector>
#include <string>

class CFactoryInitterBase
{
public:
	virtual ~CFactoryInitterBase();

	virtual void Process() = 0;
	std::string m_name;
};

extern std::vector<CFactoryInitterBase*> Initters;
extern std::vector<CFactoryInitterBase*> EarlyInitters;