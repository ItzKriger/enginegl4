#include "CStdConsole.h"
#include "CEngine.h"
#include "U_Platform.h"
#include "U_ConsoleColors.h"
#include "CConVarManager.h"
#include "CCommandProcessor.h"
#include "CWrapable.h"
#include <conio.h>

void CStdConsole::V_Init()
{
	auto mgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();

	mgr->AddConVar("con.time.maxfps", new CWrapable<unsigned int>(20));
	mgr->AddConVar("con.time.yield", new CWrapable<bool>(true));
	mgr->AddConVar("con.time.adaptive_sleep", new CWrapable<bool>(true));
	mgr->AddConVar("con.time.strict_sleep", new CWrapable<bool>(false));

	COMPONENT_CALL(CConVarManager, AddConVar("con.max_msg", new CWrapable<unsigned int>(100)));

	//std::ios::sync_with_stdio(false);
	//std::cin.tie(nullptr);

	Crossplatform::CreateConsole();
	m_consoleThread = std::make_unique<std::thread>(&CStdConsole::m_threadedWorker, this);
}

void CStdConsole::V_DeInit()
{
	AsyncState.Set(1);
	//while (AsyncState.Get() != 2) { std::this_thread::yield(); }

	if (m_consoleThread->joinable())
	{
		m_consoleThread->join();
	}

	Crossplatform::DestroyConsole();
	//COMPONENT_CALL(CConVarManager, DeleteConVar("con.max_msg")); TODO segmentation fault
}

void CStdConsole::Print(const std::wstring& text)
{
	std::wcout << text;
}

void CStdConsole::Error(const std::wstring& err)
{
	auto oldcolors = GetColor();

	SetColor(CColor::Red, oldcolors.second); //TODO don't hardcode the error color
	std::wcerr << err;

	SetColor(oldcolors.first, oldcolors.second);
}

void CStdConsole::SetColor(const CColor& text, const CColor& bg)
{
	CConsoleColor txt = ConsoleColors::RGBToConsoleColor(text);
	CConsoleColor bag = ConsoleColors::RGBToConsoleColor(bg);

	Crossplatform::SetConsoleColors(txt, bag);

	bgCol = bg;
	txCol = text;
}

std::pair<CColor, CColor> CStdConsole::GetColor() const
{
	return { txCol, bgCol };
}

void CStdConsole::m_threadedWorker()
{
	while (true)
	{
		if(CEngine::GetInstance()->GetStopFlag()) { return; }

		auto mgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();
		auto maxfps = mgr->GetConVarValue<unsigned int>("con.time.maxfps");

		Sleeper.Start(maxfps);

		auto yield = mgr->GetConVarValue<bool>("con.time.yield");
		auto adaptive_sleep = mgr->GetConVarValue<bool>("con.time.adaptive_sleep");
		auto strict_sleep = mgr->GetConVarValue<bool>("con.time.strict_sleep");

		unsigned int maxmsg = 100;
		COMPONENT_CALL_GET(maxmsg, CConVarManager, GetConVarValue<unsigned int>("con.max_msg", 100));

		for (size_t i = 0U; i < maxmsg; i++)
		{
			if (!_kbhit()) { continue; }
		
			constexpr int key_backspace = 8;
			constexpr int key_enter = 13;
			constexpr int key_del = 127;
		
			constexpr int key_special = 224;
		
			constexpr int key_up = 72;
			constexpr int key_down = 80;
			constexpr int key_left = 75;
			constexpr int key_right = 77;
			constexpr int key_delete = 83;
			constexpr int key_insert = 82;
			constexpr int key_home = 71;
			constexpr int key_end = 79;
			constexpr int key_pgup = 73;
			constexpr int key_pgdn = 81;
		
			int key = _getch();
			if (key == key_backspace)
			{
				if (!m_Input.empty())
				{
					m_Input.pop_back();

					COMPONENT_CALL(CCommandProcessor, History.EditedCommand(m_Input));
					std::wcout << L'\b' << L' ' << L'\b';
				}
			}
			else if (key == key_special)
			{
				key = _getch(); //TODO create method ReplaceText instead of fors
				if (key == key_up)
				{
					for (size_t i = 0U; i < m_Input.size(); i++) { std::wcout << L'\b'; } //HACK HACK HACK
					for (size_t i = 0U; i < m_Input.size(); i++) { std::wcout << L' '; }
					for (size_t i = 0U; i < m_Input.size(); i++) { std::wcout << L'\b'; }
		
					COMPONENT_CALL_GET(m_Input, CCommandProcessor, History.GetNext());
					std::wcout << m_Input;
				}
				else if (key == key_down)
				{
					for (size_t i = 0U; i < m_Input.size(); i++) { std::wcout << L'\b'; } //HACK HACK HACK
					for (size_t i = 0U; i < m_Input.size(); i++) { std::wcout << L' '; }
					for (size_t i = 0U; i < m_Input.size(); i++) { std::wcout << L'\b'; }
		
					COMPONENT_CALL_GET(m_Input, CCommandProcessor, History.GetPrev());
					std::wcout << m_Input;
				}
			}
			else if (key > 31 && key != 127)
			{
				m_Input.push_back(key);

				COMPONENT_CALL(CCommandProcessor, History.EditedCommand(m_Input));
				std::wcout << wchar_t(key);
			}
		
			if (!m_Input.empty() && key == key_enter)
			{
				std::wcout << '\n';
		
				SendCommand(m_Input);
				m_Input.clear();
			}
		}

		if(AsyncState.Get() == 1)
		{
			AsyncState.Set(2);
			break;
		}

		Sleeper.End(yield, adaptive_sleep, strict_sleep);
	}
}

CStdConsole::~CStdConsole()
{
	
}

LINK_CONSOLE_TO_CLASS(CStdConsole, stdconsole);