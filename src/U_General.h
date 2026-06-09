#pragma once
#include <type_traits>
#include <string>
#include <glm/glm.hpp>

//circular dependency warning:
//don't include U_String
//don't include U_Types

#define REINTERPRET_AS(var, type) (*reinterpret_cast<type*>(&(var)))

template <class T, class U>
struct are_same_template : std::conjunction<
    std::is_same<typename std::remove_cvref<T>::type, typename std::remove_cvref<U>::type>,
    std::is_same<T, U>> {};

//1 - r, 2 - g, 3 - b, 4 - a
std::string GetLuaVectorColorIndex(size_t index);
//1 - x, 2 - y, 3 - z, 4 - w
std::string GetLuaVectorCharIndex(size_t index);
//0 - r, 1 - g, 2 - b, 3 - a
std::string GetVectorColorIndex(size_t index);
//0 - x, 1 - y, 2 - z, 3 - w
std::string GetVectorCharIndex(size_t index);

constexpr glm::vec3 DefaultGravity = glm::vec3(0.0f, -500.0f, 0.0f);

bool IsClient();
bool IsServer();
bool IsConnectedClient();

#define GET_CONST_FIELD(type, _class, expr) const_cast<const type>(const_cast<_class*>(this)->expr);