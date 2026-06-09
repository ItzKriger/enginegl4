#pragma once
#include <string>
#include <unordered_map>

#include "CScriptingEngine.h"
#include "CScriptClassInfo.h"
#include "CObjectFactory.h"

template<typename IdType = std::string>
class CScriptClassesManager
{
public:
    using ClassInfoMap = std::unordered_map<IdType, CScriptClassInfo<IdType>>;
    using ClassesList = std::vector<IdType>;

    class CDeleterBase
    {
    public:
        virtual void Delete() = 0;
        virtual ~CDeleterBase() {}
    };

    template<typename Base, typename Id>
    class CDeleter : public CDeleterBase
    {
    public:
        CDeleter(CObjectFactory<Base, Id>& _Factory, CFunctionBase<void*, const Id&>* _Function, CFunctionBase<bool, const Id&>* _FindFunction) : Factory(_Factory), Function(_Function), FindFunction(_FindFunction) {}
        ~CDeleter() { Delete(); }

        void Delete() override
        {
            Factory.OnTryCreateHandler.DeleteFunction(Function);
            Factory.OnTryFindHandler.DeleteFunction(FindFunction);
        }
        
        CObjectFactory<Base, Id>& Factory;
        CFunctionBase<void*, const Id&>* Function = nullptr;
        CFunctionBase<bool, const Id&>* FindFunction = nullptr;
    };

    template<typename Base>
    ClassInfoMap& GetClassInfoMap()
    {
        size_t hash = typeid(Base).hash_code();
        if(HashClassInfos.find(hash) == HashClassInfos.end())
        {
            HashClassInfos.insert({ hash, ClassInfoMap() });
        }
        return HashClassInfos.find(hash)->second;
    }

    template<typename Base>
    CObjectFactoryBase<IdType>& GetFactory()
    {
        size_t hash = typeid(Base).hash_code();
        return HashFactories.find(hash)->second.get();
    }

	template<typename Generic, typename Base>
	void SetFunction(CObjectFactory<Base, IdType>& factory)
	{
        size_t hash = typeid(Base).hash_code();
        if(HashClassInfos.find(hash) == HashClassInfos.end())
        {
            HashClassInfos.insert({ hash, ClassInfoMap() });
        }

        if(HashFactories.find(hash) == HashFactories.end())
        {
            HashFactories.insert({ hash, std::ref(factory) });
        }

		factory.OnTryCreateHandler += [this, hash](const IdType& id) -> void*
		{
            auto it_hash = HashClassInfos.find(hash);
            if(it_hash == HashClassInfos.end()) { return nullptr; }

            auto& ClassInfos = it_hash->second;

			auto it = ClassInfos.find(id);
			if(it == ClassInfos.end()) { return nullptr; }
			
			return (void*)(new Generic(it->second.LuaKey, it->second.Functions, it->second.State));
		};

        factory.OnTryFindHandler += [this, hash](const IdType& id) -> bool
        {
            auto it_hash = HashClassInfos.find(hash);
            if(it_hash == HashClassInfos.end()) { return false; }

            auto& ClassInfos = it_hash->second;

			auto it = ClassInfos.find(id);
			if(it == ClassInfos.end()) { return false; }

            return true;
        };

        CFunctionBase<void*, const IdType&>* func = factory.OnTryCreateHandler.GetLastFunctionRaw();
        CFunctionBase<bool, const IdType&>* findfunc = factory.OnTryFindHandler.GetLastFunctionRaw();
        Deleters.push_back(std::make_unique<CDeleter<Base, IdType>>(factory, func, findfunc));
	}

    template<typename Base>
    void RegisterClassInfo(const CScriptClassInfo<IdType>& info)
    {
        if(info.LuaKey.empty()) { return; }

        auto& info_map = GetClassInfoMap<Base>();
        auto& factory = GetFactory<Base>();

        if(info_map.find(info.LuaKey) != info_map.end()) { return; }

        info_map.insert({ info.LuaKey, info });
        factory.AddScriptClass(info.LuaKey);
    }

    template<typename Base>
    bool IsClassRegistered(const IdType& className) const
    {
        size_t hash = typeid(Base).hash_code();
        auto it = HashClassInfos.find(hash);
        if (it == HashClassInfos.end()) 
        {
            return false;
        }
        
        return it->second.find(className) != it->second.end();
    }

    template<typename Base>
    void UnregisterClassInfo(const IdType& id)
    {
        std::erase_if(GetClassInfoMap<Base>(), [&id](auto& kv) -> bool
        {
            return kv.second.LuaKey == id;
        });

        auto& factory = GetFactory<Base>();
        factory.RemoveScriptClass(id);
    }

    template<typename Base>
    void UnregisterClassesInfo(CScriptingEngine* engine)
    {
        std::erase_if(GetClassInfoMap<Base>(), [engine](auto& kv) -> bool
        {
            return kv.second.Engine == engine;
        });
    }

    template<typename Base>
    void UnregisterClassesInfo(sol::state_view st)
    {
        std::erase_if(GetClassInfoMap<Base>(), [&st](auto& kv) -> bool
        {
            return kv.second.State == st;
        });
    }
	
    template<typename Base>
	void ResetEngineClasses(CScriptingEngine* engine)
	{
        UnregisterClassesInfo<Base>(engine);
	}

    void UnregisterClassInfoEverywhere(const IdType& id)
    {
        for(auto& kv : HashClassInfos)
        {
            std::erase_if(kv.second, [&id](auto& kv) -> bool
            {
                return kv.second.LuaKey == id;
            });
        }
    }

    void UnregisterClassesInfoEverywhere(CScriptingEngine* engine)
    {
        for(auto& kv : HashClassInfos)
        {
            std::erase_if(kv.second, [engine](auto& kv) -> bool
            {
                return kv.second.Engine == engine;
            });
        }
    }

    void UnregisterClassesInfoEverywhere(sol::state_view st)
    {
        for(auto& kv : HashClassInfos)
        {
            std::erase_if(kv.second, [&st](auto& kv) -> bool
            {
                return kv.second.State == st;
            });
        }
    }
	
	void ResetEngineClassesEverywhere(CScriptingEngine* engine)
	{
        UnregisterClassesInfoEverywhere(engine);
	}
	
    std::vector<std::unique_ptr<CDeleterBase>> Deleters;
    std::unordered_map<size_t, ClassInfoMap> HashClassInfos;
    std::unordered_map<size_t, std::reference_wrapper<CObjectFactoryBase<IdType>>> HashFactories;

    //std::unique_ptr<CDeleterBase> Deleter;
	//ClassInfos;
};
