#include "CFactoryInitterBase.h"

CFactoryInitterBase::~CFactoryInitterBase() {}

std::vector<CFactoryInitterBase*> Initters;
std::vector<CFactoryInitterBase*> EarlyInitters;