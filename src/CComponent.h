#pragma once
#include "CCallbackHandler.h"
#include "CFactoryInitter.h"
#include "CScriptFieldsManager.h"
#include "CComponentPriority.h"
#include "CScriptObject.h"

#include "sol/sol.hpp"

#define LINK_COMPONENT_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; const std::type_info& _class::GetTypeInfo() const { return typeid(_class); }; CFactoryInitter<_class, CComponent> __component_initter_ ## name = CFactoryInitter<_class, CComponent>(#name, std::function<void(CFactoryInitter<_class, CComponent>*)>([](CFactoryInitter<_class, CComponent>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->ComponentsFactory; }));
#define DEFINE_COMPONENT() std::string GetType() const override; const std::type_info& GetTypeInfo() const override

//bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override

class CComponent : public CScriptObject
{
public:
	CComponent();
	virtual ~CComponent();

	bool Init();
	void PostInit();
	void DeInit();
	void Update();

	virtual bool V_Init();
	virtual void V_PostInit();
	virtual void V_DeInit();

	virtual void V_Update();
	virtual std::string GetType() const;
	virtual const std::type_info& GetTypeInfo() const;

	bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

	void SetPriority(std::unique_ptr<CComponentPriority> priority);
	void ResetPriority();
    CComponentPriority* GetPriority() const;

	CCallbackHandler<bool, CComponent*> OnInit; //ret:is_success this_component
	CCallbackHandler<void, CComponent*> OnPostInit; //ret:nothing this_component
	CCallbackHandler<void, CComponent*> OnUpdate; //ret:nothing this_component
	CCallbackHandler<void, CComponent*> OnDestruct; //ret:nothing this_component
	CCallbackHandler<void, CComponent*> OnDeInit; //ret:nothing this_component

	sol::table UserData;
private:
	std::unique_ptr<CComponentPriority> Priority;
};
