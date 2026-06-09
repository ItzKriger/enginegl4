#include "CTerminal.h"
#include "CEngine.h"
#include "CStartupArgsManager.h"
#include "U_ScriptClasses.h"

bool CTerminal::V_Init()
{
	return true;
}

void CTerminal::V_DeInit()
{
	if (Console)
	{
		Console->DeInit();
	}
}

void CTerminal::V_Update()
{
	if (Console && Console->IsInitted())
	{
		Console->Update();
	}
}

void CTerminal::V_PostInit()
{
	std::string ConsoleType;
	COMPONENT_CALL_GET(ConsoleType, CStartupArgsManager, GetArgumentValue("-ui"));

	CreateConsole(ConsoleType);

	if (Console)
	{
		Console->Init();
	}
}

bool CTerminal::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
	table.set_function("registerNew", InvokeRegisterFunction<CConsole, "console">);
	return true;
}

void CTerminal::CreateConsole(const std::string& type)
{
	if (!CEngine::GetInstance()->ConsoleFactory.Exists(type))
	{
		Log::ErrInstance() << "Couldn't create console \"" << type << "\"\n";
		return;
	}
	Console = CEngine::GetInstance()->ConsoleFactory.create<std::unique_ptr<CConsole>>(type);
}

void CTerminal::Print(const std::wstring& text)
{
	if (!Console || !Console->IsInitted()) { return; }
	Console->Print(text);
}

void CTerminal::Error(const std::wstring& err)
{
	if (!Console || !Console->IsInitted()) { return; }
	Console->Error(err);
}

void CTerminal::SetColor(const CColor& text, const CColor& bg)
{
	if (!Console || !Console->IsInitted()) { return; }
	Console->SetColor(text, bg);
}

void CTerminal::SetColor(const std::pair<CColor, CColor>& txt_bg)
{
	return SetColor(txt_bg.first, txt_bg.second);
}

std::pair<CColor, CColor> CTerminal::GetColor() const
{
	if (!Console || !Console->IsInitted()) { return { CColor::White, CColor::Black }; }
	return Console->GetColor();
}

bool CTerminal::IsConsoleAvailable() const
{
	return (bool)Console;
}

LINK_COMPONENT_TO_CLASS(CTerminal, terminal);