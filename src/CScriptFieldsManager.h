#pragma once
#include "sol/sol.hpp"

#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>

#include "U_StringTemplate.h"

static bool lua_equal(const sol::object& a, const sol::object& b)
{
    sol::state_view lua = a.lua_state();
    if (a.get_type() != b.get_type())
    {
        return false;
    }

    switch (a.get_type())
    {
        case sol::type::nil:
            return true;
        case sol::type::boolean:
            return a.as<bool>() == b.as<bool>();
        case sol::type::number:
            return a.as<double>() == b.as<double>();
        case sol::type::string:
            return a.as<std::string>() == b.as<std::string>();
        default:
            break;
    }

    sol::function eq = lua["__cpp_eq"];
    if (!eq.valid())
    {
        lua["__cpp_eq"] = lua.safe_script
        (
            "return function(a, b) return a == b end",
            sol::script_pass_on_error
        );
        eq = lua["__cpp_eq"];
    }
    return eq(a, b);
}

class CScriptFieldBase
{
public:
    CScriptFieldBase() = delete;
    CScriptFieldBase(bool readonly = false) : ReadOnly(readonly) {}

	bool ReadOnly = false;
	
	virtual void SetObject(sol::object obj) = 0;
    virtual sol::object GetObject(sol::state_view s) = 0;
};

template<typename T>
class CScriptField : public CScriptFieldBase
{
public:
    CScriptField() : CScriptFieldBase(false) {}
    CScriptField(const T& val, bool readonly = false) : CScriptFieldBase(readonly), Value(val) {}
    CScriptField(bool readonly) : CScriptFieldBase(readonly) {}

	T Value;

	void SetObject(sol::object obj) override
	{
        if constexpr(std::is_same_v<T, sol::object>)
        {
            Value = obj;
        }
        else
        {
		    Value = obj.as<T>();
        }
	};

    sol::object GetObject(sol::state_view s) override
    {
        if constexpr(std::is_same_v<T, sol::object>)
        {
            return Value;
        }
        else
        {
            return sol::make_object<T>(s, Value);
        }
    }
};

template<typename T>
class CProxyScriptField : public CScriptFieldBase
{
public:
    CProxyScriptField() = delete;
    CProxyScriptField(T& val, bool readonly = false) : CScriptFieldBase(readonly), Value(val) {}

	T& Value;

	void SetObject(sol::object obj) override
	{
        if constexpr(std::is_same_v<T, sol::object>)
        {
            Value = obj;
        }
        else
        {
		    Value = obj.as<T>();
        }
	};

    sol::object GetObject(sol::state_view s) override
    {
        if constexpr(std::is_same_v<T, sol::object>)
        {
            return Value;
        }
        else
        {
            return sol::make_object<T>(s, Value);
        }
    }
};

class CFunctionalScriptField : public CScriptFieldBase
{
public:
    using SetterFunc = std::function<void(sol::object)>;
    using GetterFunc = std::function<sol::object(sol::state_view)>;

    using SetterFuncPtr = std::unique_ptr<SetterFunc>;
    using GetterFuncPtr = std::unique_ptr<GetterFunc>;

    SetterFuncPtr Setter;
    GetterFuncPtr Getter;

    CFunctionalScriptField() = delete;
    CFunctionalScriptField(GetterFunc getter, SetterFunc setter, bool readonly = false);
    CFunctionalScriptField(GetterFunc getter); //only readonly

	void SetObject(sol::object obj) override;
    sol::object GetObject(sol::state_view s) override;
};

class CScriptFieldsManager //manages __index and __newindex
{
public:
    class CKeyBase
    {
    public:
        virtual ~CKeyBase() = default;
        virtual bool operator==(const sol::object& obj) const { return false; }
        virtual bool operator==(const CKeyBase* obj) const { return false; }

        virtual std::string GetString() const { return std::string(); }
        virtual std::size_t Hash() const { return 0; }
    };

    template<typename T>
    class CKey : public CKeyBase
    {
    public:
        T Value;
        CKey(T val) : Value(val) {} //TODO is ref needed?

        bool operator==(const sol::object& obj) const override
        {
            if constexpr (std::is_same_v<T, sol::object>)
            {
                return obj.valid() && lua_equal(obj, Value);
            }
            else
            {
                return obj.valid() && obj.is<T>() && obj.as<T>() == Value;
            }
        }

        bool operator==(const CKeyBase* other) const override
        {
            const CKey<T>* otherKey = dynamic_cast<const CKey<T>*>(other);
            return otherKey && otherKey->Value == Value;
        }

        std::string GetString() const override
        {
            if constexpr (std::is_same_v<T, sol::object>)
            {
                if(Value.template is<std::string>())
                {
                    return "sol::object (string): " + Value.template as<std::string>();
                }
                else if(Value.template is<int>())
                {
                    return "sol::object (int): " + std::to_string(Value.template as<int>());
                }
                return "sol::object";
            }
            else if constexpr (std::is_same_v<T, sol::table>)
            {
                return "sol::table";
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                return Value;
            }
            else if constexpr (std::is_same_v<T, std::wstring>)
            {
                return "std::wstring";
            }
            else
            {
                return std::to_string(Value);
            }
        }

        std::size_t Hash() const override
        {
            if constexpr (std::is_same_v<T, sol::object>)
            {
                if (!Value.valid())
                {
                    return 0;
                }

                sol::type t = Value.get_type();
                switch (t)
                {
                    case sol::type::number:
                        return std::hash<double>()(Value.template as<double>());
                    case sol::type::string:
                        return std::hash<std::string>()(Value.template as<std::string>());
                    case sol::type::boolean:
                        return std::hash<bool>()(Value.template as<bool>());
                    case sol::type::userdata:
                    case sol::type::lightuserdata:
                    case sol::type::table:
                    case sol::type::function:
                    {
                        lua_State* L = Value.lua_state();
                        Value.push();
                        const void* ptr = lua_topointer(L, -1);
                        lua_pop(L, 1);
                        return std::hash<const void*>()(ptr);
                    }
                    case sol::type::nil:
                    default:
                        return 0;
                }
            }
            else
            {
                return std::hash<T>()(Value);
            }
        }
    };

    using CKeyPtr = std::unique_ptr<CKeyBase>;

    struct CKeyPtrHash
    {
        std::size_t operator()(const CKeyPtr& k) const
        {
            return k ? k->Hash() : 0;
        }
    };

    struct CKeyPtrEqual
    {
        bool operator()(const CKeyPtr& lhs, const CKeyPtr& rhs) const
        {
            if (!lhs || !rhs) { return lhs == rhs; }
            return *lhs == rhs.get();
        }
    };

    using Optional = std::optional<sol::object>;
    using IndexFunc = std::function<Optional(sol::table, sol::object, sol::this_state)>;
    using NewIndexFunc = std::function<std::optional<sol::object>(sol::table, sol::object, sol::object, sol::this_state)>;

    bool FieldCreationAbility = true;
    std::unordered_map<CKeyPtr, CKeyPtr, CKeyPtrHash, CKeyPtrEqual> Aliases;
    std::unordered_map<CKeyPtr, std::unique_ptr<CScriptFieldBase>, CKeyPtrHash, CKeyPtrEqual> Fields;

    std::vector<IndexFunc> IndexCheckers;
    std::vector<NewIndexFunc> NewIndexCheckers;

    CScriptFieldBase* AddField(std::unique_ptr<CKeyBase>&& key, std::unique_ptr<CScriptFieldBase>&& ptr)
    {
        if (!ptr || !key) { return nullptr; }

        auto ret = Fields.emplace(std::make_pair(std::move(key), std::move(ptr)));
        return ret.second ? ret.first->second.get() : nullptr;
    }

    template<typename T>
    CScriptFieldBase* AddField(const T& key, std::unique_ptr<CScriptFieldBase>&& ptr)
    {
        auto normalized_key = normalize_string(key);
        using KeyType = decltype(normalized_key);

        auto key_ptr = std::make_unique<CKey<KeyType>>(normalized_key);
        return AddField(std::unique_ptr<CKeyBase>(std::move(key_ptr)), std::move(ptr));
    }

    auto FindAliasForObject(sol::object obj)
    {
        return std::find_if(Aliases.begin(), Aliases.end(), [obj](auto& kv) -> bool
        {
            return (kv.first && obj.valid()) && (*kv.first) == obj;
        });
    }

    auto GetFieldForObject(sol::object obj)
    {
        auto alias = FindAliasForObject(obj);
        if(alias != Aliases.end())
        {
            auto& aliased_key = alias->second;

            auto aliased_it = std::find_if(Fields.begin(), Fields.end(), [&aliased_key, obj](auto& kv) -> bool
            {
                return (kv.first && obj.valid()) && ((*kv.first) == aliased_key.get());
            });

            if(aliased_it != Fields.end()) { return aliased_it; }
        }

        auto it = std::find_if(Fields.begin(), Fields.end(), [obj](auto& kv) -> bool
        {
            return (kv.first && obj.valid()) && (*kv.first) == obj;
        });

        return it;
    }

    auto FindAlias(CKeyBase* alias)
    {
        return std::find_if(Aliases.begin(), Aliases.end(), [&alias](auto& kv) -> bool
        {
            return (kv.first && alias) && (*kv.first) == alias;
        });
    }

    auto GetFieldIterator(CKeyBase* key)
    {
        if(!key) { return Fields.end(); }

        auto alias = FindAlias(key);
        if(alias != Aliases.end())
        {
            auto& aliased_key = alias->second;
            auto aliased_it = std::find_if(Fields.begin(), Fields.end(), [&aliased_key](auto& kv) -> bool
            {
                return (kv.first && aliased_key && kv.second) && ((*kv.first) == aliased_key.get() && kv.second.operator bool());
            });

            if(aliased_it != Fields.end()) { return aliased_it; }
        }

        auto it = std::find_if(Fields.begin(), Fields.end(), [&key](auto& kv) -> bool
        {
            return (kv.first && kv.second && key) && ((*kv.first) == key && kv.second.operator bool());
        });

        return it;
    }

    auto GetFieldIterator(std::unique_ptr<CKeyBase>&& key)
    {
        return GetFieldIterator(key.get());
    }

    template<typename T>
    auto GetFieldIterator(const T& key)
    {
        auto key_ptr = std::make_unique<CKey<T>>(key);
        return GetFieldIterator(static_cast<CKeyBase*>(key_ptr.get()));
    }

    CScriptFieldBase* GetField(CKeyBase* key)
    {
        auto it = GetFieldIterator(key);
        return (it != Fields.end()) ? it->second.get() : nullptr;
    }

    CScriptFieldBase* GetField(std::unique_ptr<CKeyBase>&& key)
    {
        return GetField(key.get());
    }

    template<typename T>
    CScriptFieldBase* GetField(const T& key)
    {
        auto key_ptr = std::make_unique<CKey<T>>(key);
        return GetField(static_cast<CKeyBase*>(key_ptr.get()));
    }

    void SetMetaFunctions(sol::table meta_table)
    {
        meta_table["__index"] = [this](sol::table table, sol::object key, sol::this_state s) -> sol::object
        {
            auto it = GetFieldForObject(key);

            if(it != Fields.end())
            {
                return it->second->GetObject(sol::state_view(s));
            }

            for(auto& checker : IndexCheckers)
            {
                auto ret = checker(table, key, s);

                if(ret.has_value())
                {
                    return ret.value();
                }
            }

            return sol::lua_nil;
        };
        
        meta_table["__newindex"] = [this](sol::table table, sol::object key, sol::object value, sol::this_state s) -> void
        {
            auto it = GetFieldForObject(key);
            
            if(it != Fields.end() && !it->second->ReadOnly)
            {
                return it->second->SetObject(value);
            }

            bool checkerFound = false;
            for(auto& checker : NewIndexCheckers)
            {
                auto ret = checker(table, key, value, s);

                if(ret.has_value())
                {
                    ret.value() = value;
                    checkerFound = true;
                }
            }

            if(!checkerFound && it == Fields.end() && FieldCreationAbility)
            {
                //auto field = std::make_unique<CScriptField<sol::object>>(value);
                //AddField(key, std::move(field));

                table.raw_set(key, value);
            }
        };
    }

    bool AddAlias(std::unique_ptr<CKeyBase>&& alias, std::unique_ptr<CKeyBase>&& target_key)
    {
        if(!alias || !target_key || FindAlias(alias.get()) != Aliases.end()) { return false; }

        auto ret = Aliases.emplace(std::make_pair(std::move(alias), std::move(target_key)));
        return ret.second;
    }

    template<typename TypeAlias>
    bool AddAlias(const TypeAlias& alias, std::unique_ptr<CKeyBase>&& target_key)
    {
        auto alias_ptr = std::make_unique<CKey<TypeAlias>>(alias);
        return AddAlias(std::unique_ptr<CKeyBase>(std::move(alias_ptr)), std::move(target_key));
    }

    template<typename TypeTarget>
    bool AddAlias(std::unique_ptr<CKeyBase>&& alias, const TypeTarget& target_key)
    {
        auto target_ptr = std::make_unique<CKey<TypeTarget>>(target_key);
        return AddAlias(std::move(alias), std::unique_ptr<CKeyBase>(std::move(target_ptr)));
    }

    template<typename TypeAlias, typename TypeTarget>
    bool AddAlias(const TypeAlias& alias, const TypeTarget& target_key)
    {
        auto alias_ptr = std::make_unique<CKey<TypeAlias>>(alias);
        auto target_ptr = std::make_unique<CKey<TypeTarget>>(target_key);

        return AddAlias(std::unique_ptr<CKeyBase>(std::move(alias_ptr)), std::unique_ptr<CKeyBase>(std::move(target_ptr)));
    }

    bool IsAliasExist(CKeyBase* alias)
    {
        return FindAlias(alias) != Aliases.end();
    }

    bool IsAliasExist(std::unique_ptr<CKeyBase>&& alias)
    {
        return FindAlias(alias.get()) != Aliases.end();
    }

    template<typename TypeAlias>
    bool IsAliasExist(const TypeAlias& alias)
    {
        auto alias_ptr = std::make_unique<CKey<TypeAlias>>(alias);
        return FindAlias(alias_ptr.get()) != Aliases.end();
    }

    bool DeleteAlias(std::unique_ptr<CKeyBase>&& alias)
    {
        auto it = FindAlias(alias.get());
        if(it == Aliases.end()) { return false; }

        Aliases.erase(it);
        return true;
    }

    bool DeleteAlias(CKeyBase* alias)
    {
        auto it = FindAlias(alias);
        if(it == Aliases.end()) { return false; }

        Aliases.erase(it);
        return true;
    }

    template<typename TypeAlias>
    bool DeleteAlias(const TypeAlias& alias)
    {
        auto alias_ptr = std::make_unique<CKey<TypeAlias>>(alias);
        auto it = FindAlias(alias_ptr.get());
        if(it == Aliases.end()) { return false; }

        Aliases.erase(it);
        return true;
    }

    bool IsFieldExist(std::unique_ptr<CKeyBase>&& key)
    {
        return GetFieldIterator(static_cast<CKeyBase*>(key.get())) != Fields.end();
    }

    bool IsFieldExist(CKeyBase* key)
    {
        return GetFieldIterator(key) != Fields.end();
    }

    template<typename T>
    bool IsFieldExist(const T& key)
    {
        auto key_ptr = std::make_unique<CKey<T>>(key);
        return GetFieldIterator(static_cast<CKeyBase*>(key_ptr.get())) != Fields.end();
    }

    void AddIndexChecker(const IndexFunc& func)
    {
        IndexCheckers.push_back(func);
    }

    void AddNewIndexChecker(const NewIndexFunc& func)
    {
        NewIndexCheckers.push_back(func);
    }

    static std::string GetDefaultKey() { return "__fields_manager"; }

    static bool HasFieldsManager(sol::table tabl)
    {
        sol::object fieldsMan_obj = tabl[GetDefaultKey()];
        return fieldsMan_obj.valid();
    }

    sol::table CreateMetaTable(sol::table def_table)
    {
        sol::table meta = sol::state_view(def_table.lua_state()).create_table();

        SetMetaFunctions(meta);
        def_table[sol::metatable_key] = meta;

        return meta;
    }

    static std::shared_ptr<CScriptFieldsManager> CreateFieldsManager(sol::table tabl, bool instantMeta = false)
    {
        sol::object fieldsMan = tabl.get<sol::object>(GetDefaultKey());
        if(fieldsMan != sol::lua_nil) { return fieldsMan.as<std::shared_ptr<CScriptFieldsManager>>(); }

        auto fieldsManager = std::make_shared<CScriptFieldsManager>();
        tabl[GetDefaultKey()] = fieldsManager;

        if(instantMeta)
        {
            fieldsManager->CreateMetaTable(tabl);
        }
        return fieldsManager;
    }

    static std::pair<std::shared_ptr<CScriptFieldsManager>, bool> ValidateFieldsManager(sol::table tabl, bool instantMeta = false)
    {
        sol::object fieldsMan_obj = tabl[GetDefaultKey()];
        if(fieldsMan_obj.valid())
        {
            return  { fieldsMan_obj.as<std::shared_ptr<CScriptFieldsManager>>(), true };
        }
        return { CreateFieldsManager(tabl, instantMeta), false };
    }
};

using CScriptFieldsManagerPtr = std::shared_ptr<CScriptFieldsManager>;