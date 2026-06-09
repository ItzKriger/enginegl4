#include "CWrapableBase.h"

CWrapableBase::~CWrapableBase()
{

}

void CWrapableBase::SetValueStrANSI(const std::string& str)
{
	SetValueStr(StringUtils::StrToWstr(str));
}

std::string CWrapableBase::GetValueStrANSI() const
{
	return StringUtils::WstrToStr(GetValueStr());
}
