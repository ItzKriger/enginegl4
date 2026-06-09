#pragma once
#include <thread>
#include <mutex>
#include <atomic>

#include "CConsole.h"
#include "CThreadedValue.h"
#include "CThreadSleeper.h"

class CStdConsole : public CConsole
{
public:
	void V_Init() override;	
	void V_DeInit() override;

	void Print(const std::wstring& text) override;
	void Error(const std::wstring& err) override;

	void SetColor(const CColor& text, const CColor& bg) override;
	std::pair<CColor, CColor> GetColor() const override; //first: text, second: background

	~CStdConsole();

	DEFINE_CONSOLE();
private:
	CColor bgCol = CColor::Black, txCol = CColor::White;

	void m_threadedWorker();
	std::unique_ptr<std::thread> m_consoleThread;

	std::wstring m_Input;
	CThreadedValue<char> AsyncState{ 0 };

	CThreadSleeper Sleeper;
};
