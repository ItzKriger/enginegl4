#include "CConsole.h"
#include "CEngine.h"
#include "CCommandProcessor.h"
#include "CTerminal.h"

template<typename Ret>
static std::function<Ret(const sol::function&, CConsole*)> GetOutTransformer()
{
	return [](const sol::function& func, CConsole* comp) -> Ret
	{
		sol::table comp_table = comp->GetScriptTable(func.lua_state());

		if constexpr (std::is_void_v<Ret>)
		{
			SafeCallLua(func, comp_table);
			//func(comp_table);
		}
		else
		{
			sol::object ret = SafeCallLuaReturn(func, comp_table);
			return ret.valid() ? ret.as<bool>() : false;
		}
	};
}

template<typename Ret>
static std::function<sol::object(CFunctionBase<Ret, CConsole*>*, sol::variadic_args)> GetInTransformer()
{
	return [](CFunctionBase<Ret, CConsole*>* func, sol::variadic_args va) -> sol::object
	{
		if(va.size() < 1) { return sol::make_object<int>(va.lua_state(), 1); }

		auto arg = va.get<sol::table>(0);
		if(!arg.valid()) { return sol::make_object<int>(va.lua_state(), 2); }
		
		sol::state_view state(va.lua_state());

		CConsole* console = nullptr;

		auto terminal = CEngine::GetInstance()->Components.GetComponentTyped<CTerminal>();
		if(terminal->Console && terminal->Console->GetScriptTable(state) == arg)
		{
			console = terminal->Console.get();
		}

		if(!console) { return sol::make_object<int>(va.lua_state(), 3); }
		
		sol::object ret = sol::lua_nil;
		if constexpr (!std::is_void_v<Ret>)
		{
			ret = sol::make_object<Ret>(va.lua_state(), func->Call(console));
		}
		else
		{
			func->Call(console);
		}

		return ret;
	};
}

CConsole::CConsole()
{
	OnInit.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());
	OnUpdate.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());
	OnDestruct.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());

	OnScriptInit += [this](std::shared_ptr<sol::state> _state, sol::table _table) -> bool
	{
		_table.set_function("init", [this]() { Init(); });
		_table.set_function("update", [this]() { Update(); });

		auto fieldsMan_pair = CScriptFieldsManager::ValidateFieldsManager(_table);
		auto fieldsMan = fieldsMan_pair.first;

		fieldsMan->AddField("type", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object(ts, this->GetType()); }));
		fieldsMan->AddField("oninit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnInit.GetScriptTable(ts); }));
		fieldsMan->AddField("onupdate", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnUpdate.GetScriptTable(ts); }));
		fieldsMan->AddField("ondestruct", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnDestruct.GetScriptTable(ts); }));

		fieldsMan->FieldCreationAbility = true;
		if(!fieldsMan_pair.second) { fieldsMan->CreateMetaTable(_table); }
		return true;
	};
}

bool CConsole::IsInitted() const { return m_Initted; }

void CConsole::Init()
{
	V_Init();
	OnInit(this);

	m_Initted = true;
}

void CConsole::DeInit()
{
	V_DeInit();
	OnDeInit(this);

	m_Initted = false;
}

void CConsole::Update()
{
	V_Update();
	OnUpdate(this);
}

CConsole::~CConsole()
{
	OnDestruct(this);
}

void CConsole::V_Init() {}
void CConsole::V_DeInit() {}
void CConsole::V_Update() {}

void CConsole::Print(const std::wstring& text) {}
void CConsole::Error(const std::wstring& err) {}
void CConsole::SetColor(const CColor& text, const CColor& bg) {}
std::pair<CColor, CColor> CConsole::GetColor() const { return { CColor::White, CColor::Black }; }

void CConsole::SendCommand(const std::wstring& cmdline)
{
	COMPONENT_CALL(CCommandProcessor, AddCommandToQueue(cmdline));
}
