#include "U_ShortAPI.h"
#include "CEntity.h"
#include "CTransformable.h"
#include "CEngine.h"
#include "CWindowManager.h"
#include "CDrawableEntity.h"
#include "CResourcesManager.h"
#include "CDrawableModel.h"
#include "CScenesManager.h"
#include "CServerDrawable.h"

void SetEntityDrawable(CEntity* ent, const std::filesystem::path& mdl, bool animated)
{
    if(!ent->Components.IsComponentPresent<CTransformable>())
    {
        ent->Components.CreateComponent<CTransformable>();
    }

    std::shared_ptr<CDrawable> drawable;
    std::string modelType = "servermodel";

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();

    if(CEngine::GetInstance()->Components.IsComponentPresent<CWindowManager>())
    {
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
        auto _winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
        auto& _renderer = _winman->Renderer;

        if(_renderer)
        {
            drawable = animated ? _renderer->CreateAnimatedDrawable() : _renderer->CreateNonAnimatedDrawable();
            modelType = _renderer->GetModelType();
        }
    }

    if(!drawable)
    {
        drawable = std::make_shared<CServerDrawable>(); //TODO animated and non animated variants
    }

    if(!ent->Components.IsComponentPresent<CDrawableEntity>())
    {
        ent->Components.CreateComponent<CDrawableEntity>(false);
    }
    auto ent_drawable = ent->Components.GetComponentTyped<CDrawableEntity>();

    Log::Instance() << "SetEntityDrawable mdl.string is " << mdl.string() << Log::Endl;

    auto _model = resman->GetOrCreate(modelType, mdl.string());
    auto model = std::dynamic_pointer_cast<CModelBase>(_model);

    auto mdl_drawable = std::dynamic_pointer_cast<CDrawableModel>(drawable);
    mdl_drawable->SetModel(model);

    ent_drawable->Drawable = drawable;
    //ent_drawable->Init(); //shouldn't really init right here
}

void AddDrawableToSpace(std::shared_ptr<CDrawable> drawable, int spaceid)
{
    auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
    if(scenesMan)
    {
        auto _mainScene = scenesMan->ActiveScene;

        if(_mainScene.lock())
        {
            auto mainScene = _mainScene.lock();

            if(spaceid >= 0 && spaceid < mainScene->SpacesManager.Spaces.size())
            {
                auto space = mainScene->SpacesManager.Spaces.at(spaceid).get();
                space->AddDrawable(drawable);
            }
        }
    }
}

std::shared_ptr<CDrawable> CreateAnimatedDrawable()
{
    std::shared_ptr<CDrawable> ret;
    if(CEngine::GetInstance()->Components.IsComponentPresent<CWindowManager>())
    {
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
        auto _winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
        auto& _renderer = _winman->Renderer;

        if(_renderer)
        {
            ret =  _renderer->CreateAnimatedDrawable();
        }
    }

    if(!ret)
    {
        ret = std::make_shared<CServerDrawable>();
    }
    return ret;
}

std::shared_ptr<CDrawable> CreateNonAnimatedDrawable()
{
    std::shared_ptr<CDrawable> ret;
    if(CEngine::GetInstance()->Components.IsComponentPresent<CWindowManager>())
    {
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
        auto _winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
        auto& _renderer = _winman->Renderer;

        if(_renderer)
        {
            ret =  _renderer->CreateNonAnimatedDrawable();
        }
    }

    if(!ret)
    {
        ret = std::make_shared<CServerDrawable>(); //TODO animated and non animated variants
    }
    return ret;
}

//TODO create linking class for space -> drawable or drawable -> space (maybe map?)
std::vector<size_t> GetDrawableSpaces(CDrawable* drawable) //TODO EXTREMELY NON PERFORMANT
{
    std::vector<size_t> ret;

    auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
    if(scenesMan)
    {
        for(auto& scene : scenesMan->Scenes)
        {
            for(size_t i = 0; i < scene->SpacesManager.Spaces.size(); i++)
            {
                auto& space = scene->SpacesManager.Spaces[i];
                for(auto& _dr : space->Drawables)
                {
                    if(_dr.get() == drawable)
                    {
                        ret.push_back(i);
                        break;
                    }
                }
            }
        }
    }
    return ret;
}

void SetEntityModel(CEntity* ent, const std::string& modelname, int spaceid) //TODO automatically decide space
{
    auto mdlname_opt = ent->GetInitParam<std::string>("modelname");
    auto spaceid_opt = ent->GetInitParam<int>("spaceid");

    if(!modelname.empty())
    {
        SetEntityDrawable(ent, modelname);
    }
    else if(mdlname_opt.has_value())
    {
        auto val = mdlname_opt.value();
        SetEntityDrawable(ent, val);
    }

    auto drawable = ent->Components.GetComponentTyped<CDrawableEntity>();
    if(drawable && drawable->Drawable)
    {
        if(spaceid > -1)
        {
            AddDrawableToSpace(drawable->Drawable, spaceid);
        }
        else if(spaceid_opt.has_value())
        {
            auto spaceIndex = spaceid_opt.value();
            AddDrawableToSpace(drawable->Drawable, spaceIndex);
        }
        drawable->Init();
    }
}

void SetFieldsManager(sol::table _table, std::function<void(std::shared_ptr<CScriptFieldsManager>)> _fields_setter)
{
    auto fieldsMan_pair = CScriptFieldsManager::ValidateFieldsManager(_table);
	auto fieldsMan = fieldsMan_pair.first;

    if(_fields_setter)
    {
        _fields_setter(fieldsMan);
    }
    
	fieldsMan->FieldCreationAbility = true;
	if(!fieldsMan_pair.second) { fieldsMan->CreateMetaTable(_table); }
}