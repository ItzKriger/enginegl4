#pragma once
#include "CTransform.h"
#include "CWrapable.h"

class CCamera
{
public:
    CCamera();

    CTransform Transform;
    CWrapable<float> FieldOfView, NearPlane, FarPlane, Aspect;

    void RetrieveAspect();

    bool AcknowledgeProjChange() const;
    bool AcknowledgeViewChange() const;

    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetProjectionMatrix() const;
private:
    void UpdateProj();
    void UpdateView();

    mutable bool m_projUpdated = true;
    mutable bool m_viewUpdated = true;

    glm::mat4 m_View, m_Proj;
};
