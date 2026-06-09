#pragma once
#include <string>
#include <mutex>
#include "CColor.h"

#include "CCallbackHandler.h"
#include "CScriptObject.h"

#define LINK_CONSOLE_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CConsole> __component_initter_ ## name = CFactoryInitter<_class, CConsole>(#name, std::function<void(CFactoryInitter<_class, CConsole>*)>([](CFactoryInitter<_class, CConsole>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->ConsoleFactory; }));
#define DEFINE_CONSOLE() std::string GetType() const override

class CConsole : public CScriptObject
{
public:
	CConsole();

	void Init();
	void DeInit();
	void Update();

	bool IsInitted() const;

	virtual void V_Init();
	virtual void V_DeInit();
	virtual void V_Update();
	virtual ~CConsole();

	virtual void Print(const std::wstring& text);
	virtual void Error(const std::wstring& err);

	//sync functions are supposed to be called outside the main thread

	//virtual void SyncPrint(const std::wstring& text);
	//virtual void SyncError(const std::wstring& err);

	virtual void SetColor(const CColor& text, const CColor& bg);
	virtual std::pair<CColor, CColor> GetColor() const; //first: text, second: background

	void SendCommand(const std::wstring& cmdline);
	virtual std::string GetType() const = 0;

	CCallbackHandler<void, CConsole*> OnInit;
	CCallbackHandler<void, CConsole*> OnDeInit;
	CCallbackHandler<void, CConsole*> OnUpdate;
	CCallbackHandler<void, CConsole*> OnDestruct;
private:
	bool m_Initted = false;
};
