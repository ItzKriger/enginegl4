#include "CCamera.h"
#include "CEngine.h"

#include "CWindowManager.h"

CCamera::CCamera() : FieldOfView(110.0f), Aspect(1.0f), NearPlane(0.5f), FarPlane(10000.0f)
{
    auto changedView = [this](CTransformBase::CMeasurePack oldPack, CTransformBase* _this) -> void
    {
        UpdateView();
    };

    auto changedProj = [this](CTypedWrapableBase<float>* _this, float oldValue, float newValue) -> void
    {
        UpdateProj();
    };

    RetrieveAspect();

    FieldOfView.OnValueChanged += changedProj;
    Aspect.OnValueChanged += changedProj;
    NearPlane.OnValueChanged += changedProj;
    FarPlane.OnValueChanged += changedProj;

    Transform.OnTransformChanged += changedView;

    UpdateProj();
    UpdateView();
}

void CCamera::UpdateProj()
{
    m_Proj = glm::perspective(glm::radians(FieldOfView.GetValue()), Aspect.GetValue(), NearPlane.GetValue(), FarPlane.GetValue());
    m_projUpdated = true;

    //Log::Instance() << "m_Proj = glm::perspective(glm::radians(" << FieldOfView.GetValue() << "), " << Aspect.GetValue() << ", " << NearPlane.GetValue() << ", " << FarPlane.GetValue() << ");\n";
}

void CCamera::UpdateView()
{
    auto vecToStr = [](const glm::vec3& vec) -> std::string
    {
        return "{ " + StringUtils::ToStr(vec.x) + ", " + StringUtils::ToStr(vec.y) + ", " + StringUtils::ToStr(vec.z) + " }";
    };

    m_View = glm::lookAt(Transform.GetPosition(), Transform.GetPosition() + Transform.GetForwardVector(), Transform.GetUpVector());
    m_viewUpdated = true;

    //Log::Instance() << "m_View = glm::lookAt(" << vecToStr(Transform.GetPosition()) << ", " << vecToStr(Transform.GetPosition()) << " + " << vecToStr(Transform.GetForwardVector()) << 
    //    " = " << vecToStr(Transform.GetPosition() + Transform.GetForwardVector()) << ", " << vecToStr(Transform.GetUpVector()) << ");\n";
}

void CCamera::RetrieveAspect()
{
    float aspect = 1.0f;
    COMPONENT_CALL_GET(aspect, CWindowManager, GetWindowAspect());

    Aspect.SetValue(aspect);
    //Aspect.SetValue(CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>()->GetWindowAspect());
    //Log::Instance() << "Aspect.SetValue(" << CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>()->GetWindowAspect() << ");\n";
}

const glm::mat4& CCamera::GetViewMatrix() const
{
    return m_View;
}

const glm::mat4& CCamera::GetProjectionMatrix() const
{
    return m_Proj;
}

bool CCamera::AcknowledgeProjChange() const
{
    bool ret = m_projUpdated;
    m_projUpdated = false;
    return ret;
}

bool CCamera::AcknowledgeViewChange() const
{
    bool ret = m_viewUpdated;
    m_viewUpdated = false;
    return ret;
}
