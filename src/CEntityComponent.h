#pragma once
#include "CCallbackHandler.h"
#include "CScriptObject.h"
#include "CBufferWrapper.h"

#include "sol/sol.hpp"

#define LINK_ENTITY_COMPONENT_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CEntityComponent> __entity_component_initter_ ## name = CFactoryInitter<_class, CEntityComponent>(#name, std::function<void(CFactoryInitter<_class, CEntityComponent>*)>([](CFactoryInitter<_class, CEntityComponent>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->EntityComponentsFactory; }));
#define DEFINE_ENTITY_COMPONENT() std::string GetType() const override

class CEntity;
class CEntityComponent : public CScriptObject
{
public:
	CEntityComponent();
	virtual ~CEntityComponent();

    void OverridePersistent(CEntity* ent);
	void Init();
	void Update();

	void DevalidateInit(); //TODO THAT'S A HACK
	virtual void V_Init();

	virtual void V_Update();
	virtual std::string GetType() const;

	virtual void FullPack(CBufferWrapper& packet);
	virtual void FullUnpack(CBufferWrapper& packet);

	bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
	CEntity* GetEntity();

	CCallbackHandler<void, CEntityComponent*> OnInit; //ret:nothing this_component
	CCallbackHandler<void, CEntityComponent*> OnUpdate; //ret:nothing this_component
	CCallbackHandler<void, CEntityComponent*> OnDestruct; //ret:nothing this_component

	DEFINE_SOL_USERTYPE();

	bool NetSync = true; //TODO on change events
private:
    CEntity* m_Entity = nullptr;
	bool m_Initted = false;
};
