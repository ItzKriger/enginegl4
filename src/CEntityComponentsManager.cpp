#include "CEntityComponentsManager.h"
#include "CEngine.h"

void CEntityComponentsManager::Add(std::unique_ptr<CEntityComponent>&& comp)
{
    std::string type = comp->GetType();
    Items[type] = std::move(comp);
    Items[type]->OverridePersistent(m_Entity);

    //TODO add component net message
}

bool CEntityComponentsManager::IsComponentPresent(const std::string& type) const
{
    return Items.find(type) != Items.end();
}

std::unique_ptr<CEntityComponent>& CEntityComponentsManager::GetComponent(const std::string& type)
{
    auto it = Items.find(type);
    if (it == Items.end()){ throw std::runtime_error("No such component"); }
    return it->second;
}

void CEntityComponentsManager::CreateComponent(const std::string& type, bool instantInit)
{
    if(IsComponentPresent(type))
    {
        Log::ErrInstance() << "Entity component " << type << " already exist!\n";
        return;
    }

    std::unique_ptr<CEntityComponent> comp = CEngine::GetInstance()->EntityComponentsFactory.create<std::unique_ptr<CEntityComponent>>(type);
    if(!comp) { return; }

    Add(std::move(comp));

    if(instantInit)
    {
        GetComponent(type)->Init();
    }
}

CEntity* CEntityComponentsManager::GetEntity()
{
    return m_Entity;
}

void CEntityComponentsManager::Trace(const std::function<void(CEntityComponent* nod)>& func)
{
    for (auto& component : Items)
    {
        func(component.second.get());
    }
}
