#pragma once
#include "SDL3/SDL.h"

#include <string>
#include <cstdint>
#include <memory>

#include "CModelBase.h"
#include "CTransform.h"
#include "CCamera.h"
#include "CDrawable.h"

#define LINK_RENDERER_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CRendererBase> __renderer_initter_ ## name = CFactoryInitter<_class, CRendererBase>(#name, std::function<void(CFactoryInitter<_class, CRendererBase>*)>([](CFactoryInitter<_class, CRendererBase>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->RenderersFactory; }));
#define DEFINE_RENDERER() std::string GetType() const override

class CRendererBase
{
public:
    virtual ~CRendererBase() = default;

    virtual void Init(SDL_Window* window) = 0;
    virtual size_t GetGenericFlag() const = 0;
    virtual void ClearWindow() = 0;
    virtual void Display() = 0;

    virtual void SetupCamera(std::shared_ptr<CCamera> camera);
    virtual void DrawModel(std::shared_ptr<CModelBase> model, const CTransform& transform);

    void Update();
    virtual void V_Update();

    virtual std::shared_ptr<CDrawable> CreateAnimatedDrawable() const;
    virtual std::shared_ptr<CDrawable> CreateNonAnimatedDrawable() const;

    virtual std::string GetModelType() const; //TODO maybe better solution?
    virtual std::string GetMaterialType() const; //TODO maybe better solution?
    virtual std::string GetType() const;
};
