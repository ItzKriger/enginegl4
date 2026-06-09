#include "CComponentsManager.h"
#include "CEngine.h"
#include "CScriptingManager.h"

CComponentsManager::~CComponentsManager()
{
    //Trace([](CComponent* comp) { comp->DeInit(); });
}

void CComponentsManager::Add(std::unique_ptr<CComponent>&& comp)
{
    std::string type = comp->GetType();
    AddCache(type, comp->GetTypeInfo(), comp.get());

    Items[type] = std::move(comp);
    OnComponentAdd(type);

    auto man = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>();
	if(man)
	{
		man->CallHook(type + "_add");
	}
}

bool CComponentsManager::IsComponentPresent(const std::string& type) const
{
    return Items.find(type) != Items.end();
}

void CComponentsManager::AddCache(const std::string& name, std::type_index _type, CComponent* _component)
{
    hashCache.emplace(_type.hash_code(), _component);
    nameCache.emplace(name, _component);
}

void CComponentsManager::RemoveCache(CComponent* component)
{
    auto it_h = std::find_if(hashCache.begin(), hashCache.end(), [component](auto& kv) -> bool { return kv.second == component; });
    auto it_n = std::find_if(nameCache.begin(), nameCache.end(), [component](auto& kv) -> bool { return kv.second == component; });

    if(it_h != hashCache.end()) { hashCache.erase(it_h); }
    if(it_n != nameCache.end()) { nameCache.erase(it_n); }
}

void CComponentsManager::DeleteComponent(const std::string& type)
{
    auto it = Items.find(type);
    if(it != Items.end())
    {
        it->second->DeInit();

        RemoveCache(it->second.get());
        Items.erase(it);
    }
}

std::unique_ptr<CComponent>& CComponentsManager::GetComponent(const std::string& type)
{
    auto it = Items.find(type);
    if (it == Items.end()) { throw std::runtime_error("No such component"); }
    return it->second;
}

CComponent* CComponentsManager::GetComponentRaw(const std::string& type)
{
    auto it = Items.find(type);
    if (it == Items.end()) { return nullptr; }
    return it->second.get();
}

void CComponentsManager::CreateComponent(const std::string& type, bool instantInit)
{
    std::unique_ptr<CComponent> comp = CEngine::GetInstance()->ComponentsFactory.create<std::unique_ptr<CComponent>>(type);
    if(!comp) { return; }

    Add(std::move(comp));
    if(instantInit)
    {
        GetComponent(type)->Init();
        GetComponent(type)->PostInit();
    }
}

std::vector<CComponent*> CComponentsManager::getComponentsOrder()
{
    std::vector<CComponent*> components;
    for (auto& [id, comp] : Items)
    {
        components.push_back(comp.get());
    }

    auto comparator = [](CComponent* a, CComponent* b)
    {
        const auto getPriority = [](CComponent* comp) -> CComponentPriority*
        {
            return comp ? comp->GetPriority() : nullptr;
        };

        CComponentPriority* prioA = getPriority(a);
        CComponentPriority* prioB = getPriority(b);

        if (prioB && prioB->ShouldExecuteAfter(a->GetType()))
        {
            return true;
        }

        if (prioA && prioA->ShouldExecuteAfter(b->GetType()))
        {
            return false;
        }

        int keyA = prioA ? prioA->GetSortingKey() : 0;
        int keyB = prioB ? prioB->GetSortingKey() : 0;

        if (keyA != keyB)
        {
            return keyA > keyB;
        }

        return false;
    };

    std::stable_sort(components.begin(), components.end(), comparator);
    return components;
}

void CComponentsManager::Trace(const std::function<void(CComponent* nod)>& func)
{
    auto components = getComponentsOrder();
    
    for (auto comp : components)
    {
        func(comp);
    }
}

void CComponentsManager::ReverseTrace(const std::function<void(CComponent* nod)>& func)
{
    auto components = getComponentsOrder();

    for (auto it = components.rbegin(); it != components.rend(); ++it)
    {
        func(*it);
    }
}
