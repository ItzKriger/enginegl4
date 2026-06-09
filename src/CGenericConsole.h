#pragma once
#include "CConsole.h"
#include "CScriptClass.h"

class CGenericConsole : public CConsole, public CScriptClass<std::string>
{
public:
    using CScriptClass<std::string>::CScriptClass;

	void V_Init() override;
	void V_Update() override;

	void Print(const std::wstring& text) override;
	void Error(const std::wstring& err) override;

	void SetColor(const CColor& text, const CColor& bg) override;
	std::pair<CColor, CColor> GetColor() const override; //first: text, second: background

    sol::table GetCurrentTable();
	std::string GetType() const override;
};
