#pragma once
#include <map>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>
#include <limits>
#include "CCallbackHandler.h"

#include <iostream>

template <class Base>
class AbstractCreator
{
public:
    virtual ~AbstractCreator() = default;
    virtual Base* create() const = 0;
};

template <class C, class Base>
class Creator : public AbstractCreator<Base>
{
public:
    Base* create() const override { return new C(); }
};

template <class IdType>
class CObjectFactoryBase
{
public:
    virtual ~CObjectFactoryBase() = default;
    virtual std::vector<IdType> GetRegisteredIds() const { return {}; }
    virtual std::vector<IdType> GetRegisteredIdsNative() const { return {}; }

    virtual size_t GetIndexNative(const IdType& id) const { return 0; }
    virtual size_t GetIndex(const IdType& id) const { return 0; }

    IdType GetByID(size_t id) const
    {
        auto it = IdCache.find(id);
        if(it == IdCache.end()) { return {}; }
        return it->second;
    }

    void RecacheIds()
    {
        IdCache.clear();
        auto ids = GetRegisteredIds();

        for(auto& id : ids)
        {
            IdCache.emplace(GetIndex(id), id);
        }
    }

    void AddScriptClass(const IdType& id)
    {
        ScriptClasses.push_back(id);
        RecacheIds();
    }

    void RemoveScriptClass(const IdType& id)
    {
        std::erase(ScriptClasses, id);
        RecacheIds();
    }

    std::vector<IdType> ScriptClasses;
    std::unordered_map<size_t, IdType> IdCache;
};

template <class Base, class IdType>
class CObjectFactory : public CObjectFactoryBase<IdType> //TODO exists && getcount aren't interacting with custom creation funcs
{
public:
    using PBase = CObjectFactoryBase<IdType>; //as in Parent Base

    using AbstractFactory = std::unique_ptr<AbstractCreator<Base>>;
    using FactoryMap = std::unordered_map<IdType, AbstractFactory>;

    CObjectFactory() = default;
    virtual ~CObjectFactory() = default;

    template <class C>
    void add(const IdType& id)
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        if (Factory.find(id) == Factory.end())
        {
            Factory[id] = std::make_unique<Creator<C, Base>>();
            PBase::RecacheIds();
        }
    }

    template <class C>
    void replace(const IdType& id) //TODO script classes support
    {
        auto it = Factory.find(id);
        if(it != Factory.end())
        {
            Factory.erase(it);
        }

        Factory[id] = std::make_unique<Creator<C, Base>>();
        PBase::RecacheIds();
    }

    Base* createRaw(const IdType& id) const
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);

        if(!OnTryCreateHandler.IsEmpty())
        {
            Base* bs = nullptr;
            for(auto& func : OnTryCreateHandler.Functions)
            {
                bs = static_cast<Base*>(func(id));
                if(bs) { break; }
            }

            if(bs)
            {
                OnCreate(bs);
                return bs;
            }
        }

        auto it = Factory.find(id);
        if (it != Factory.end())
        {
            Base* obj = it->second->create();
            OnCreate(obj);

            return obj;
        }
        return nullptr;
    }

    template <typename PtrType = std::shared_ptr<Base>>
    PtrType create(const IdType& id) const
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);

        if(!OnTryCreateHandler.IsEmpty())
        {
            Base* bs = nullptr;
            for(auto& func : OnTryCreateHandler.Functions)
            {
                bs = static_cast<Base*>((*func)(id));
                if(bs) { break; }
            }

            if(bs)
            {
                std::unique_ptr<Base> obj(bs);
                OnCreate(obj.get());
                
                if constexpr (std::is_same_v<PtrType, std::shared_ptr<Base>>)
                {
                    return std::shared_ptr<Base>(std::move(obj));
                }
                return obj;
            }
        }

        auto it = Factory.find(id);
        
        if (it != Factory.end())
        {
            std::unique_ptr<Base> obj(it->second->create());
            OnCreate(obj.get());

            if constexpr (std::is_same_v<PtrType, std::shared_ptr<Base>>)
            {
                return std::shared_ptr<Base>(std::move(obj));
            }
            return obj;
        }
        return nullptr;
    }

    bool remove(const IdType& id)
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        return Factory.erase(id) > 0;
    }

    size_t GetCount() const
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        return Factory.size() + PBase::ScriptClasses.size();
    }

    size_t GetCountNative() const
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        return Factory.size();
    }

    size_t GetIndexNative(const IdType& id) const override
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        auto it = Factory.find(id);
        if (it != Factory.end())
        {
            return std::distance(Factory.begin(), it);
        }
        return std::numeric_limits<size_t>::max();
    }

    size_t GetIndex(const IdType& id) const override
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        auto it = Factory.find(id);
        if (it != Factory.end())
        {
            return std::distance(Factory.begin(), it);
        }
        else
        {
            auto it2 = std::find(PBase::ScriptClasses.begin(), PBase::ScriptClasses.end(), id);
            if(it2 != PBase::ScriptClasses.end())
            {
                return Factory.size() + std::distance(PBase::ScriptClasses.begin(), it2);
            }
        }
        return std::numeric_limits<size_t>::max();
    }

    bool Exists(const IdType& id) const
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);

        if(!OnTryFindHandler.IsEmpty())
        {
            for(auto& func : OnTryFindHandler.Functions)
            {
                if((*func)(id))
                {
                    return true;
                }
            }
        }
        return Factory.find(id) != Factory.end();
    }

    std::vector<IdType> GetRegisteredIds() const override
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        std::vector<IdType> ids;
        for (const auto& pair : Factory)
        {
            ids.push_back(pair.first);
        }
        for(const auto& sclass : PBase::ScriptClasses)
        {
            ids.push_back(sclass);
        }
        return ids;
    }

    std::vector<IdType> GetRegisteredIdsNative() const override
    {
        //std::lock_guard<std::mutex> lock(factoryMutex);
        std::vector<IdType> ids;
        for (const auto& pair : Factory)
        {
            ids.push_back(pair.first);
        }
        return ids;
    }

    CCallbackHandler<void, Base*> OnCreate;
    CCallbackHandler<void*, const IdType&> OnTryCreateHandler;
    CCallbackHandler<bool, const IdType&> OnTryFindHandler;
private:
    FactoryMap Factory;
    mutable std::mutex factoryMutex;
};
