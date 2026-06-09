#pragma once
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <functional>
#include <typeindex>

#include "U_Log.h"
#include "CComponent.h"
#include "CCallbackHandler.h"

class CComponentsManager
{
public:
	~CComponentsManager();

	void Add(std::unique_ptr<CComponent>&& comp);
	bool IsComponentPresent(const std::string& type) const;
	std::unique_ptr<CComponent>& GetComponent(const std::string& type);
	CComponent* GetComponentRaw(const std::string& type);
	void CreateComponent(const std::string& type, bool instantInit = true);

	void Trace(const std::function<void(CComponent* nod)>& func);
	void ReverseTrace(const std::function<void(CComponent* nod)>& func);
	
	template<typename ComponentType>
	void DeleteComponent()
	{
		auto curHash = typeid(ComponentType).hash_code();

		auto _comp_ptr_it = hashCache.find(curHash);
		if(_comp_ptr_it == hashCache.end()) { return; }

		auto _comp_ptr = _comp_ptr_it->second;

		auto it = std::find_if(Items.begin(), Items.end(), [_comp_ptr](const auto& kv) -> bool
			{
				//return dynamic_cast<ComponentType*>(kv.second.get());
				return kv.second.get() == _comp_ptr;
			});

		if(it != Items.end())
		{
			it->second->DeInit();

			RemoveCache(it->second.get());
			Items.erase(it);
		}
	}

	void DeleteComponent(const std::string& type);

	template<typename ComponentType>
	void CreateComponent(bool instantInit = true)
	{
		Add(std::make_unique<ComponentType>());
		if(instantInit)
		{
			GetComponentTyped<ComponentType>()->Init();
			GetComponentTyped<ComponentType>()->PostInit();
		}
	}

	template<typename ComponentType>
	bool IsComponentPresent() const
	{
		auto curHash = typeid(ComponentType).hash_code();
		
		auto _comp_ptr_it = hashCache.find(curHash);
		if(_comp_ptr_it == hashCache.end()) { return false; }

		auto _comp_ptr = _comp_ptr_it->second;

		auto it = std::find_if(Items.begin(), Items.end(), [_comp_ptr](const auto& kv) -> bool
			{
				//return dynamic_cast<ComponentType*>(kv.second.get());
				return kv.second.get() == _comp_ptr;
			});
			
		return it != Items.end();
	}

	template<typename ComponentType>
	ComponentType* GetComponentTyped()
	{
		auto curHash = typeid(ComponentType).hash_code();
		
		auto _comp_ptr_it = hashCache.find(curHash);
		if(_comp_ptr_it == hashCache.end()) { return nullptr; }

		auto _comp_ptr = _comp_ptr_it->second;

		return dynamic_cast<ComponentType*>(_comp_ptr);

		auto it = std::find_if(Items.begin(), Items.end(), [_comp_ptr](const auto& kv) -> bool
			{
				//if(!kv.second) { return false; }
				return dynamic_cast<ComponentType*>(kv.second.get());
			});

		if (it == Items.end())
		{
			//throw std::runtime_error("Component with the specified type not found");
			return nullptr; //unreachable
		}
		return dynamic_cast<ComponentType*>((*it).second.get());
	}

	template<typename ComponentType>
	ComponentType* GetComponentTyped(const std::string& type)
	{
		const auto& it = Items.find(type);
		//if (it == Items.end()) { throw std::runtime_error("No such component"); }
		if (it == Items.end()) { return nullptr; }

		if (ComponentType* casted = dynamic_cast<ComponentType*>(it->second.get()))
		{
			return casted;
		}
		else
		{
			//throw std::runtime_error("Component type mismatch");
			return nullptr;
		}
	}

	CCallbackHandler<void, std::string> OnComponentAdd;
	CCallbackHandler<void, std::string> OnComponentInit;
	CCallbackHandler<void, std::string> OnComponentPostInit;
	std::unordered_map<std::string, std::unique_ptr<CComponent>> Items;

	void AddCache(const std::string& name, std::type_index _type, CComponent* _component);
	void RemoveCache(CComponent* component);
private:
	std::vector<CComponent*> getComponentsOrder();

	std::unordered_map<size_t, CComponent*> hashCache;
	std::unordered_map<std::string, CComponent*> nameCache;
};
