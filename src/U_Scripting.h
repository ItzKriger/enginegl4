#pragma once
#include "sol/sol.hpp"
#include <string>
#include <functional>
#include <tuple>
#include <memory>

#include "CAngle.h"
#include "U_Angles.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/geometric.hpp"
#include "U_General.h"
#include "U_Log.h"
#include "U_Types.h"
#include "CColor.h"
#include "CColorInt.h"
#include "CScriptFieldsManager.h"
#include "CTransform.h"
#include "CWrapableBase.h"

#include "SDL3/SDL_events.h"

namespace ScriptUtils
{
    namespace ReadOnly
    {
        template<typename T>
        sol::table Create(const T& value, sol::state_view s)
        {
            sol::table ret = s.create_table();
            sol::table meta = s.create_table();
    
            meta.set_function("__index", [&value](sol::this_state) -> const T&
            {
                return value;
            });
        
            meta.set_function("__newindex", [](sol::this_state, sol::object, sol::object) {});
    
            ret[sol::metatable_key] = meta;
            return ret;
        }
    
        template<typename T>
        sol::table CreateFunctional(const std::function<T()>& func, sol::state_view s)
        {
            sol::table ret = s.create_table();
            sol::table meta = s.create_table();
    
            meta.set_function("__index", [&func](sol::this_state) -> T
            {
                return func();
            });
    
            meta.set_function("__newindex", [](sol::this_state, sol::object, sol::object) {});
    
            ret[sol::metatable_key] = meta;
            return ret;
        }
    }

    namespace Args
    {
        template <typename... Args, std::size_t... I>
        auto extract_args_impl(sol::variadic_args args, std::index_sequence<I...>)
        {
            return std::make_tuple(args.template get<Args>(I)...);
        }

        template <typename... Args>
        auto extract_args(sol::variadic_args args)
        {
            return extract_args_impl<Args...>(args, std::index_sequence_for<Args...>{});
        }
    }

    sol::state& GetStateReference(lua_State* state);
    std::shared_ptr<sol::state> GetStateSharedPtr(lua_State* state);

    template<typename T>
    T FromObject(sol::object obj)
    {
        if constexpr (is_glm_vec_v<T> || is_glm_mat_v<T> || is_glm_quat_v<T> || std::is_same_v<T, CAngle> || std::is_same_v<T, CAngles> || std::is_same_v<T, SDL_Event>)
        {
            if (obj.is<T>())
            {
                T& v = obj.as<T&>();
                return v;
            }
            return T{};
        }
        else if constexpr (std::is_same_v<T, CTransform>)
        {
            if(obj.is<sol::table>())
            {
                auto tabl = obj.as<sol::table>();
                sol::object type = tabl["__type"];

                if(type.valid() && type.as<std::string>() == "transform")
                {
                    auto opos = tabl["position"];
                    auto orot = tabl["rotation"];
                    auto oscl = tabl["scale"];

                    auto pos = opos.get<glm::vec3>();
                    auto rot = orot.get<glm::quat>();
                    auto scl = oscl.get<glm::vec3>();

                    CTransform ret;
                    ret.SetPRS({ pos, rot, scl });

                    return ret;
                }
            }
        }
        else if constexpr (std::is_same_v<T, CAngle>)
        {
            if(obj.is<sol::table>())
            {
                auto table = obj.as<sol::table>();
                sol::object type = table["type"];

                if(type.valid() && type.as<std::string>() == "angle")
                {
                    sol::object degrees = table["degrees"];
                    if(degrees.valid())
                    {
                        return CAngle::degrees(degrees.as<float>());
                    }
                }
            }
            else
            {
                return CAngle::degrees(obj.as<float>());
            }
        }
        else if constexpr (std::is_same_v<T, CAngles>)
        {
            if(obj.is<sol::table>())
            {
                auto table = obj.as<sol::table>();

                auto first = table[1];
                auto second = table[2];
                auto third = table[3];

                if(first.valid() && second.valid() && third.valid())
                {
                    return CAngles(FromObject<CAngle>(first), FromObject<CAngle>(second), FromObject<CAngle>(third));
                }
            }
            return T{};
        }
        else if constexpr (is_glm_quat_v<T> || std::is_same_v<T, CColor> || std::is_same_v<T, CColorInt>)
        {
            if(obj.is<sol::table>())
            {
                auto table = obj.as<sol::table>();
                sol::object mgr = table[CScriptFieldsManager::GetDefaultKey()];

                if(!mgr.valid())
                {
                    return T{};
                }

                T ret{};
                auto fieldsMan = mgr.as<std::shared_ptr<CScriptFieldsManager>>();
                auto nativeField = fieldsMan->GetField<std::string>("__ref_obj");

                auto casted = dynamic_cast<CProxyScriptField<T>*>(nativeField);
                if(!casted)
                {
                    return ret;
                }

                ret = casted->Value;
                return ret;
            }
            return T{};
        }
        else if constexpr (is_glm_mat_v<T>)
        {
            if (obj.is<sol::table>())
            {
                auto table = obj.as<sol::table>();
                T ret{};

                for (size_t x = 0; x < T::length(); x++)
                {
                    sol::object colObj = table[x + 1];
                    if (!colObj.is<sol::table>())
                    {
                        continue;
                    }

                    sol::table colTable = colObj.as<sol::table>();

                    for (size_t y = 0; y < T::col_type::length(); y++)
                    {
                        sol::object cellObj = colTable[y + 1];
                        if (cellObj.valid())
                        {
                            ret[x][y] = FromObject<typename T::value_type>(cellObj);
                        }
                    }
                }

                return ret;
            }

            return T{};
        }
        else
        {
            if(!obj.is<T>()) { return T{}; }
            return obj.as<T>();
        }
        return T{};
    }

    template<typename T>
    void RegisterMatrixType(sol::state_view state)
    {
        size_t typehash = typeid(T).hash_code();
        std::string _typename = GetTypeName<T>();

        using VT = typename T::value_type;


    }

    template<typename T>
    void RegisterQuaternionType(sol::state_view state)
    {
        size_t typehash = typeid(T).hash_code();
        std::string _typename = GetTypeName<T>();

        using VT = typename T::value_type;

        if(!ScriptStates::IsTypeRegistered(state, _typename))
        {
            sol::usertype<T> utype;

            utype = state.new_usertype<T>
            (
                _typename,
                sol::constructors
                <
                    T(),
                    T(VT, VT, VT, VT)
                >(),

                sol::meta_function::to_string, [](const T& v)
                {
                    return "quat(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "," + std::to_string(v.w) + ")";
                }
            );

            utype["normalize"] = [](T& v) -> T& { v = glm::normalize(v); return v; };
            utype["normalized"] = [](const T& v) { return glm::normalize(v); };
            utype["zero"] = [](T& v) -> T& { v = T{}; return v; };

            utype[sol::meta_function::multiplication] = [](const T& a, const T& b) { return a * b; };
            utype[sol::meta_function::unary_minus] = [](const T& a) { return -a; };

            utype[sol::meta_function::index] = [](T& v, int i) -> VT&
            {
                //Log::Instance() << "i is " << i << Log::Endl;
                if(i < 1 || i > T::length()) { return reinterpret_cast<VT*>(&v)[0]; }
                return reinterpret_cast<VT*>(&v)[i - 1];
            };

            utype[sol::meta_function::new_index] = [](T& v, int i, VT value)
            {
                //Log::Instance() << "i is " << i << Log::Endl;
                if(i < 1 || i > T::length()) { return; }
                reinterpret_cast<VT*>(&v)[i - 1] = value;
            };

            utype["x"] = &T::x;
            utype["r"] = &T::x;

            if constexpr (T::length() >= 2)
            {
                utype["y"] = &T::y;
                utype["g"] = &T::y;
            }

            if constexpr (T::length() >= 3)
            {
                utype["z"] = &T::z;
                utype["b"] = &T::z;
            }
            if constexpr (T::length() >= 4)
            {
                utype["w"] = &T::w;
                utype["a"] = &T::w;
            }

            state[_typename]["dimensions"] = T::length();

            utype["type"] = sol::overload
            (
                [_typename]() -> std::string { return _typename; },
                [_typename](const T& v) -> std::string { return _typename; },
                [_typename](T& v) -> std::string { return _typename; }
            );

            ScriptStates::RegisterType(state, _typename, typehash);
        }
    }

    template<typename T>
    void RegisterAngleType(sol::state_view state)
    {
        size_t typehash = typeid(T).hash_code();
        std::string _typename = GetTypeName<T>();

        using VT = typename T::value_type;

        if(!ScriptStates::IsTypeRegistered(state, _typename))
        {
            sol::usertype<T> utype;

            utype = state.new_usertype<T>
            (
                _typename,
                sol::constructors
                <
                    T(),
                    T(VT)
                >(),

                sol::meta_function::to_string, [](const T& v)
                {
                    return std::to_string(v.asRadians());
                }
            );

            {
                utype["asDegrees"] = [](const T& v) -> float { return v.asDegrees(); };
                utype["asRadians"] = [](const T& v) -> float { return v.asRadians(); };
                utype["asDegrees180"] = [](const T& v) -> float { return v.asDegrees180(); };
                utype["asRadians180"] = [](const T& v) -> float { return v.asRadians180(); };

                utype["clamp360deg"] = [](T& v, float degmin, float degmax) -> T& { v.clamp360deg(degmin, degmax); return v; };
                utype["clamp180deg"] = [](T& v, float degmin, float degmax) -> T& { v.clamp180deg(degmin, degmax); return v; };
                utype["clamp360rad"] = [](T& v, float radmin, float radmax) -> T& { v.clamp360rad(radmin, radmax); return v; };
                utype["clamp180rad"] = [](T& v, float radmin, float radmax) -> T& { v.clamp180rad(radmin, radmax); return v; };

                utype["clamped360deg"] = [](const T& v, float degmin, float degmax) -> T { T ret = v; ret.clamp360deg(degmin, degmax); return ret; };
                utype["clamped180deg"] = [](const T& v, float degmin, float degmax) -> T { T ret = v; ret.clamp180deg(degmin, degmax); return ret; };
                utype["clamped360rad"] = [](const T& v, float radmin, float radmax) -> T { T ret = v; ret.clamp360rad(radmin, radmax); return ret; };
                utype["clamped180rad"] = [](const T& v, float radmin, float radmax) -> T { T ret = v; ret.clamp180rad(radmin, radmax); return ret; };

                utype["setDegrees"] = [](T& v, float deg) -> T& { v.setDegrees(deg); return v; };
                utype["setRadians"] = [](T& v, float rad) -> T& { v.setRadians(rad); return v; };

                utype[sol::meta_function::unary_minus] = [](const T& a) { return -a; };
                utype[sol::meta_function::addition] = sol::overload
                (
                    [](const T& a, const T& b) { return a + b; },
                    [](const T& a, VT s) { return a + T::radians(s); },
                    [](VT s, const T& a) { return T::radians(s) + a; }
                );

                utype[sol::meta_function::subtraction] = sol::overload
                (
                    [](const T& a, const T& b) { return a - b; },
                    [](const T& a, VT s) { return a - T::radians(s); },
                    [](VT s, const T& a) { return T::radians(s) - a; }
                );

                utype[sol::meta_function::multiplication] = sol::overload
                (
                    [](const T& a, const T& b) { return a * b; },
                    [](const T& a, VT s) { return a * T::radians(s); },
                    [](VT s, const T& a) { return T::radians(s) * a; }
                );

                utype[sol::meta_function::division] = sol::overload
                (
                    [](const T& a, const T& b) { return a / b; },
                    [](const T& a, VT s) { return a / T::radians(s); },
                    [](VT s, const T& a) { return T::radians(s) / a; }
                );
            }

            state[_typename]["degrees"] = [](float degr) -> T { return T::degrees(degr); };
            state[_typename]["radians"] = [](float rads) -> T { return T::radians(rads); };

            utype["type"] = sol::overload
            (
                [_typename]() -> std::string { return _typename; },
                [_typename](const T& v) -> std::string { return _typename; },
                [_typename](T& v) -> std::string { return _typename; }
            );

            ScriptStates::RegisterType(state, _typename, typehash);
        }
    }

    template<typename T>
    void RegisterAnglesType(sol::state_view state)
    {
        size_t typehash = typeid(T).hash_code();
        std::string _typename = GetTypeName<T>();

        using VT = typename T::value_type;

        if(!ScriptStates::IsTypeRegistered(state, _typename))
        {
            sol::usertype<T> utype;

            utype = state.new_usertype<T>
            (
                _typename,
                sol::constructors
                <
                    T(),
                    T(VT, VT, VT)
                >(),

                sol::meta_function::to_string, [](const T& v)
                {
                    return "angles(" + std::to_string(v.x.asRadians()) + "," + std::to_string(v.y.asRadians()) + "," + std::to_string(v.z.asRadians()) + ")";
                }
            );

            utype["toQuat"] = [](const T& v) -> glm::quat
            {
		        return AngleUtils::AnglesToQuat(v);
            };

            utype[sol::meta_function::index] = [](T& v, int i) -> VT&
            {
                //Log::Instance() << "i is " << i << Log::Endl;
                if(i < 1 || i > T::length()) { return reinterpret_cast<VT*>(&v)[0]; }
                return reinterpret_cast<VT*>(&v)[i - 1];
            };

            utype[sol::meta_function::new_index] = [](T& v, int i, VT value)
            {
                //Log::Instance() << "i is " << i << Log::Endl;
                if(i < 1 || i > T::length()) { return; }
                reinterpret_cast<VT*>(&v)[i - 1] = value;
            };

            utype["x"] = &T::x;
            utype["r"] = &T::x;

            if constexpr (T::length() >= 2)
            {
                utype["y"] = &T::y;
                utype["g"] = &T::y;
            }

            if constexpr (T::length() >= 3)
            {
                utype["z"] = &T::z;
                utype["b"] = &T::z;
            }

            state[_typename]["dimensions"] = T::length();

            utype["type"] = sol::overload
            (
                [_typename]() -> std::string { return _typename; },
                [_typename](const T& v) -> std::string { return _typename; },
                [_typename](T& v) -> std::string { return _typename; }
            );

            ScriptStates::RegisterType(state, _typename, typehash);
        }
    }

    template<typename T>
    void RegisterSdlEventType(sol::state_view state)
    {
        size_t typehash = typeid(T).hash_code();
        std::string _typename = GetTypeName<T>();

        //using VT = typename T::value_type;

        SDL_Event ev;

        sol::usertype<T> utype;

        if(!ScriptStates::IsTypeRegistered(state, _typename))
        {
            utype = state.new_usertype<T>
            (
                _typename,
                sol::constructors
                <
                    T()
                >(),

                sol::meta_function::to_string, [](const T& v)
                {
                    return "sdlevent";
                }
            );

            utype["type"] = &T::type;

            utype["__type"] = sol::overload
            (
                [_typename]() -> std::string { return _typename; },
                [_typename](const T& v) -> std::string { return _typename; },
                [_typename](T& v) -> std::string { return _typename; }
            );

            ScriptStates::RegisterType(state, _typename, typehash);
        }
    }

    template<typename T>
    void RegisterVectorType(sol::state_view state)
    {
        size_t typehash = typeid(T).hash_code();
        std::string _typename = GetTypeName<T>();

        using VT = typename T::value_type;

        if(!ScriptStates::IsTypeRegistered(state, _typename))
        {
            sol::usertype<T> utype;

            if constexpr (T::length() == 1)
            {
                utype = state.new_usertype<T>
                (
                    _typename,
                    sol::constructors
                    <
                        T(),
                        T(VT)
                    >(),

                    sol::meta_function::to_string, [](const T& v)
                    {
                        return "vec1(" + std::to_string(v.x) + ")";
                    }
                );
            }
            else if constexpr (T::length() == 2)
            {
                utype = state.new_usertype<T>
                (
                    _typename,
                    sol::constructors
                    <
                        T(),
                        T(VT, VT)
                    >(),

                    sol::meta_function::to_string, [](const T& v)
                    {
                        return "vec2(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")";
                    }
                );
            }
            else if constexpr (T::length() == 3)
            {
                utype = state.new_usertype<T>
                (
                    _typename,
                    sol::constructors
                    <
                        T(),
                        T(VT, VT, VT)
                    >(),

                    sol::meta_function::to_string, [](const T& v)
                    {
                        return "vec3(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
                    }
                );
            }
            else if constexpr (T::length() == 4)
            {
                utype = state.new_usertype<T>
                (
                    _typename,
                    sol::constructors
                    <
                        T(),
                        T(VT, VT, VT, VT)
                    >(),

                    sol::meta_function::to_string, [](const T& v)
                    {
                        return "vec4(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "," + std::to_string(v.w) + ")";
                    }
                );
            }

            utype["length"] = [](const T& v) { return glm::length(v); };
            utype["normalize"] = [](T& v) -> T& { v = glm::normalize(v); return v; };
            utype["normalized"] = [](const T& v) { return glm::normalize(v); };
            utype["dot"] = [](const T& a, const T& b) { return glm::dot(a, b); };
            utype["zero"] = [](T& v) -> T& { v = T{}; return v; };
            utype[sol::meta_function::unary_minus] = [](const T& a) { return -a; };
            utype[sol::meta_function::addition] = sol::overload
            (
                [](const T& a, const T& b) { return a + b; },
                [](const T& a, VT s) { return a + s; },
                [](VT s, const T& a) { return s + a; }
            );

            utype[sol::meta_function::subtraction] = sol::overload
            (
                [](const T& a, const T& b) { return a - b; },
                [](const T& a, VT s) { return a - s; },
                [](VT s, const T& a) { return s - a; }
            );

            utype[sol::meta_function::multiplication] = sol::overload
            (
                [](const T& a, const T& b) { return a * b; },
                [](const T& a, VT s) { return a * s; },
                [](VT s, const T& a) { return s * a; }
            );

            utype[sol::meta_function::division] = sol::overload
            (
                [](const T& a, const T& b) { return a / b; },
                [](const T& a, VT s) { return a / s; },
                [](VT s, const T& a) { return s / a; }
            );

            utype[sol::meta_function::index] = [](T& v, int i) -> VT&
            {
                //Log::Instance() << "i is " << i << Log::Endl;
                if(i < 1 || i > T::length()) { return reinterpret_cast<VT*>(&v)[0]; }
                return reinterpret_cast<VT*>(&v)[i - 1];
            };

            utype[sol::meta_function::new_index] = [](T& v, int i, VT value)
            {
                //Log::Instance() << "i is " << i << Log::Endl;
                if(i < 1 || i > T::length()) { return; }
                reinterpret_cast<VT*>(&v)[i - 1] = value;
            };

            utype["x"] = &T::x;
            utype["r"] = &T::x;

            if constexpr (T::length() >= 2)
            {
                utype["y"] = &T::y;
                utype["g"] = &T::y;
            }

            if constexpr (T::length() >= 3)
            {
                utype["z"] = &T::z;
                utype["b"] = &T::z;
            }
            if constexpr (T::length() >= 4)
            {
                utype["w"] = &T::w;
                utype["a"] = &T::w;
            }   

            if constexpr (T::length() == 3)
            {
                utype["cross"] = [](const T& a, const T& b) { return glm::cross(a, b); };
            }

            state[_typename]["getLength"] = state[_typename]["length"];
            state[_typename]["dimensions"] = T::length();

            utype["type"] = sol::overload
            (
                [_typename]() -> std::string { return _typename; },
                [_typename](const T& v) -> std::string { return _typename; },
                [_typename](T& v) -> std::string { return _typename; }
            );

            ScriptStates::RegisterType(state, _typename, typehash);
        }
    }

    template<typename T>
    void RegisterType(sol::state_view state)
    {
        if constexpr (is_glm_vec_v<T>)
        {
            return RegisterVectorType<T>(state);
        }
        else if constexpr (is_glm_mat_v<T>)
        {
            return RegisterMatrixType<T>(state);
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            return RegisterQuaternionType<T>(state);
        }
        else if constexpr (std::is_same_v<T, CAngle>)
        {
            return RegisterAngleType<T>(state);
        }
        else if constexpr (std::is_same_v<T, CAngles>)
        {
            return RegisterAnglesType<T>(state);
        }
        else if constexpr (std::is_same_v<T, SDL_Event>)
        {
            return RegisterSdlEventType<T>(state);
        }
    }

    template<typename T>
    sol::object ToObject(const T& val, sol::state_view state)
    {
        /*
        ConvertFunc<Type, std::int8_t>(registry);
        ConvertFunc<Type, std::uint8_t>(registry);
        ConvertFunc<Type, std::int16_t>(registry);
        ConvertFunc<Type, std::uint16_t>(registry);
        ConvertFunc<Type, std::int32_t>(registry);
        ConvertFunc<Type, std::uint32_t>(registry);
        ConvertFunc<Type, std::int64_t>(registry);
        ConvertFunc<Type, std::uint64_t>(registry);
        ConvertFunc<Type, float>(registry);
        ConvertFunc<Type, double>(registry);
        ConvertFunc<Type, CAngle>(registry);
        ConvertFunc<Type, CAngles>(registry);
        ConvertFunc<Type, glm::quat>(registry);
        ConvertFunc<Type, glm::vec2>(registry);
        ConvertFunc<Type, glm::vec3>(registry);
        ConvertFunc<Type, glm::vec4>(registry);
        ConvertFunc<Type, std::string>(registry);
        ConvertFunc<Type, std::wstring>(registry);
        */

        //->

        /*
        numeric
        floating-point
        CAngle
        CAngles
        glm::quat
        glm::vec2
        glm::vec3
        glm::vec4
        strings (std::string && std::wstring)
        */

        T& val_non_const = const_cast<T&>(val);
        if constexpr (std::is_same_v<T, CAngle> || std::is_same_v<T, CAngles>)
        {
            return sol::make_object_userdata(state, val_non_const);
            //return val_non_const.GetScriptTable(state);
        }
        else if constexpr (is_glm_vec_v<T>)
        {
            //RegisterVectorType<T>(state);
            return sol::make_object_userdata(state, val_non_const);
        }
        else if constexpr (is_glm_quat_v<T>)
        {
            return sol::make_object_userdata(state, val_non_const);
        }
        else if constexpr (is_glm_mat_v<T>)
        {
            sol::table tabl = state.create_table();

            for (size_t x = 0; x < T::length(); x++)
            {
                sol::table column = state.create_table();

                for (size_t y = 0; y < T::col_type::length(); y++)
                {
                    column[y + 1] = static_cast<typename T::value_type>(val[x][y]);
                }

                tabl[x + 1] = column;
            }

            tabl.set("type", GetTypeName<T>());
            return tabl;
        }
        else
        {
            return sol::make_object<T>(state, val);
        }
    }

    template<typename T>
    std::pair<std::shared_ptr<T>, sol::table> CreateSharedObject(sol::state_view state)
    {
        auto shared = std::make_shared<T>();

        sol::table tabl = ScriptUtils::ToObject(*shared, state);
        tabl["__obj"] = shared;

        return { shared, tabl };
    }

    template<typename T>
    std::pair<std::shared_ptr<T>, sol::table> CreateSharedObject(sol::state_view state, const T& defval)
    {
        auto pair = CreateSharedObject<T>(state);
        (*pair.first) = defval;

        return pair;
    }

    CWrapableBase* ObjectToWrapable(sol::object obj);
    std::string ObjectToString(sol::object obj);
    std::wstring ObjectToWstring(sol::object obj);
}
