#include "CComponent.h"
#include "U_Scripting.h"

#include "CScriptingManager.h"
#include "CEngine.h"
#include "U_Log.h"

const std::type_info& CComponent::GetTypeInfo() const
{
	return typeid(CComponent);
}

template<typename Ret>
static std::function<Ret(const sol::function&, CComponent*)> GetOutTransformer()
{
	return [](const sol::function& func, CComponent* comp) -> Ret
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
static std::function<sol::object(CFunctionBase<Ret, CComponent*>*, sol::variadic_args)> GetInTransformer()
{
	return [](CFunctionBase<Ret, CComponent*>* func, sol::variadic_args va) -> sol::object
	{
		if(va.size() < 1) { return sol::make_object<int>(va.lua_state(), 1); }

		auto arg = va.get<sol::table>(0);
		if(!arg.valid()) { return sol::make_object<int>(va.lua_state(), 2); }
		
		sol::state_view state(va.lua_state());

		CComponent* component = nullptr;
		CEngine::GetInstance()->Components.Trace([&component, &state, &arg](CComponent* comp)
		{
			sol::table tabl = comp->GetScriptTable(state);
			if(tabl == arg)
			{
				component = comp;
			}
		});

		if(!component) { return sol::make_object<int>(va.lua_state(), 3); }
		
		sol::object ret = sol::lua_nil;
		if constexpr (!std::is_void_v<Ret>)
		{
			ret = sol::make_object<Ret>(va.lua_state(), func->Call(component));
		}
		else
		{
			func->Call(component);
		}

		return ret;
	};
}

CComponent::CComponent()
{
	OnInit.SetupLuaTransformers(GetOutTransformer<bool>(), GetInTransformer<bool>());
	OnUpdate.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());
	OnPostInit.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());
	OnDestruct.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());
	OnDeInit.SetupLuaTransformers(GetOutTransformer<void>(), GetInTransformer<void>());
}

bool CComponent::V_BaseScriptInit(std::shared_ptr<sol::state> _state, sol::table _table)
{
	static std::vector<std::string> Priorities = 
	{
		"default",
		"indexed",
		"first",
		"last",
		"after"
	};

	if(_state->get<sol::object>("priority") == sol::lua_nil)
	{
		sol::table priority = _state->create_named_table("priority");

		size_t index = 0;
		for(auto& pr : Priorities)
		{
			priority.set(pr, index);
			index++;
		}
	}

	_table.set_function("setPriority", [this](int _type, sol::variadic_args va)
	{
		if(_type < 0 || _type >= Priorities.size()) { return; }

		std::string type = Priorities.at(_type);
		if(type == "default")
		{
			SetPriority(std::make_unique<CDefaultPriority>());
		}
		else if(type == "indexed")
		{
			if(va.size() == 0) { return; }
			if(va.get_type(0) != sol::type::number) { return; }

			int index = va.get<int>(0);

			SetPriority(std::make_unique<CIndexedPriority>(index));
		}
		else if(type == "first")
		{
			SetPriority(std::make_unique<CFirstPriority>());
		}
		else if(type == "last")
		{
			SetPriority(std::make_unique<CLastPriority>());
		}
		else if(type == "after")
		{
			std::vector<std::string> afterComps;
			for(auto arg : va)
			{
				if(arg.get_type() != sol::type::string) { continue; }
				afterComps.push_back(arg.get<std::string>());
			}

			if(!afterComps.empty())
			{
				SetPriority(std::make_unique<CAfterPriority>(afterComps));
			}
		}
	});

	_table.set_function("init", [this]() { Init(); });
	_table.set_function("postinit", [this]() { PostInit(); });
	_table.set_function("update", [this]() { Update(); });

	auto fieldsMan_pair = CScriptFieldsManager::ValidateFieldsManager(_table);
	auto fieldsMan = fieldsMan_pair.first;

	fieldsMan->AddField("type", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object(ts, this->GetType()); }));
	fieldsMan->AddField("oninit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnInit.GetScriptTable(ts); }));
	fieldsMan->AddField("onpostinit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnPostInit.GetScriptTable(ts); }));
	fieldsMan->AddField("onupdate", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnUpdate.GetScriptTable(ts); }));
	fieldsMan->AddField("ondestruct", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnDestruct.GetScriptTable(ts); }));
	fieldsMan->AddField("ondeinit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnDeInit.GetScriptTable(ts); }));

	fieldsMan->FieldCreationAbility = true;
	if(!fieldsMan_pair.second) { fieldsMan->CreateMetaTable(_table); }
	return true;
}

CComponent::~CComponent() { OnDestruct(this); }
std::string CComponent::GetType() const { return {}; }

bool CComponent::V_Init() { return true; }
void CComponent::V_Update() {}
void CComponent::V_PostInit() {}
void CComponent::V_DeInit() {}

bool CComponent::Init()
{
	bool res = V_Init();
	auto results = OnInit.CallWithReturns(this);
	CEngine::GetInstance()->Components.OnComponentInit(GetType());

	auto man = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>();
	if(man)
	{
		man->CallHook(GetType() + "_init");
	}

	return (res && (std::find(results.begin(), results.end(), false) == results.end()));
}

void CComponent::PostInit()
{
	V_PostInit();
	OnPostInit(this);

	CEngine::GetInstance()->Components.OnComponentPostInit(GetType());

	auto man = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>();
	if(man)
	{
		man->CallHook(GetType() + "_postinit");
	}
}

void CComponent::Update()
{
	V_Update();
	OnUpdate(this);
}

void CComponent::DeInit()
{
	V_DeInit();
	OnDeInit(this);
}

void CComponent::SetPriority(std::unique_ptr<CComponentPriority> priority) { Priority = std::move(priority); }
void CComponent::ResetPriority() { Priority.reset(); }
CComponentPriority* CComponent::GetPriority() const { return Priority.get(); }
