#include "CEntityComponent.h"
#include "CScriptFieldsManager.h"
#include "U_Log.h"
#include "U_ShortAPI.h"

CEntityComponent::~CEntityComponent() { OnDestruct(this); }
std::string CEntityComponent::GetType() const { return {}; }

void CEntityComponent::V_Init() {}
void CEntityComponent::V_Update() {}

CEntityComponent::CEntityComponent()
{
	
}

CEntity* CEntityComponent::GetEntity()
{
	return m_Entity;
}

void CEntityComponent::Init()
{
	if(m_Initted) { Log::ErrInstance() << GetType() << " already initted\n"; return; }

	V_Init();
	OnInit(this);

	m_Initted = true;
}

void CEntityComponent::DevalidateInit()
{
	m_Initted = false;
}

void CEntityComponent::OverridePersistent(CEntity* ent)
{
    m_Entity = ent;
}

void CEntityComponent::Update()
{
	V_Update();
	OnUpdate(this);
}

void CEntityComponent::FullPack(CBufferWrapper& packet)
{

}

void CEntityComponent::FullUnpack(CBufferWrapper& packet)
{

}

bool CEntityComponent::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
	table.set_function("init", [this]() { Init(); });
	table.set_function("update", [this]() { Update(); });
	table.set_function("devalidateInit", [this]() { DevalidateInit(); });

	SetFieldsManager(table, [this](FieldsManPtr fieldsMan)
    {
		fieldsMan->AddField("type", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object(ts, this->GetType()); }));
		fieldsMan->AddField("oninit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnInit.GetScriptTable(ts); }));
		fieldsMan->AddField("onupdate", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnUpdate.GetScriptTable(ts); }));
		fieldsMan->AddField("ondestruct", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnDestruct.GetScriptTable(ts); }));
	});
	return true;
}

LINK_SOL_USERTYPE(CEntityComponent);