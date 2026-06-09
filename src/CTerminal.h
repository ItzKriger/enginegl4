#pragma once
#include "CConsole.h"
#include "CComponent.h"

class CTerminal : public CComponent
{
public:
	bool V_Init() override;
	void V_DeInit() override;
	void V_Update() override;

	void CreateConsole(const std::string& type);

	void V_PostInit() override;
	bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

	void Print(const std::wstring& text);
	void Error(const std::wstring& err);

	void SetColor(const CColor& text, const CColor& bg);
	void SetColor(const std::pair<CColor, CColor>& txt_bg);
	std::pair<CColor, CColor> GetColor() const;

	bool IsConsoleAvailable() const;

	DEFINE_COMPONENT();
	std::unique_ptr<CConsole> Console;
};
