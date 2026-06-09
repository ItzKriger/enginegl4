#pragma once
#include <filesystem>
#include <memory>
#include <vector>
#include <functional>
#include "sol/sol.hpp"
#include "CScriptFieldsManager.h"

using FieldsManPtr = std::shared_ptr<CScriptFieldsManager>;

class CEntity;
class CDrawable;

void SetEntityDrawable(CEntity* ent, const std::filesystem::path& mdl, bool animated = true);
void AddDrawableToSpace(std::shared_ptr<CDrawable> drawable, int spaceid = 0); //into active scene
std::shared_ptr<CDrawable> CreateAnimatedDrawable();
std::shared_ptr<CDrawable> CreateNonAnimatedDrawable();
std::vector<size_t> GetDrawableSpaces(CDrawable* drawable);
void SetEntityModel(CEntity* ent, const std::string& modelname = {}, int spaceid = -1);
void SetFieldsManager(sol::table _table, std::function<void(std::shared_ptr<CScriptFieldsManager>)> _fields_setter = {});
