#pragma once
#include <map>
#include <memory>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <functional>
#include <unordered_map>

#include "U_Log.h"
#include "CEntityComponent.h"

class CEntity;
class CEntityComponentsManager
{
public:
    CEntityComponentsManager() = delete;
    CEntityComponentsManager(CEntity* ent) : m_Entity(ent) {}

	void Add(std::unique_ptr<CEntityComponent>&& comp);
	bool IsComponentPresent(const std::string& type) const;
	std::unique_ptr<CEntityComponent>& GetComponent(const std::string& type);
	void CreateComponent(const std::string& type, bool instantInit = true);
	void Trace(const std::function<void(CEntityComponent* nod)>& func);
	
	template<typename ComponentType>
	void CreateComponent(bool instantInit = true)
	{
		if(IsComponentPresent<ComponentType>())
		{
			Log::ErrInstance() << "Entity component already exist!\n";
			return;
		}

		Add(std::make_unique<ComponentType>());

		if(instantInit)
		{
			GetComponentTyped<ComponentType>()->Init();
			//GetComponentTyped<ComponentType>()->PostInit(); TODO
		}
	}

	template<typename ComponentType>
	bool IsComponentPresent() const
	{
		auto it = std::find_if(Items.begin(), Items.end(), [](const auto& kv) -> bool
			{
				return dynamic_cast<ComponentType*>(kv.second.get());
			});
			
		return it != Items.end();
	}

	template<typename ComponentType>
	ComponentType* GetComponentTyped()
	{
		auto it = std::find_if(Items.begin(), Items.end(), [](const auto& kv) -> bool
			{
				return dynamic_cast<ComponentType*>(kv.second.get());
			});

		if (it == Items.end())
		{
			return nullptr;
			//throw std::runtime_error("Component with the specified type not found");
		}
		return dynamic_cast<ComponentType*>((*it).second.get());
	}

	template<typename ComponentType>
	ComponentType* GetComponentTyped(const std::string& type)
	{
		const auto& it = Items.find(type);
		if (it == Items.end()) { throw std::runtime_error("No such component"); }

		if (ComponentType* casted = dynamic_cast<ComponentType*>(it->second.get()))
		{
			return casted;
		}
		else
		{
			return nullptr;
			//throw std::runtime_error("Component type mismatch");
		}
	}

	//TODO delete component + net message

	CEntity* GetEntity();
	std::unordered_map<std::string, std::unique_ptr<CEntityComponent>> Items;
private:
    CEntity* m_Entity = nullptr;
};
