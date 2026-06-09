#include "CTypesTransform.h"
#include "CTypesRegistering.h"

#include "CScriptingEngine.h"
#include "CTransform.h"

void CScriptingEngine::RegisterTransformTypes()
{
    State->new_usertype<CEulerRepresentationBase>
    (
        "CEulerRepresentationBase",
        sol::no_constructor,
        "GetRotation", &CEulerRepresentationBase::GetRotation,
        "GetRotationX", &CEulerRepresentationBase::GetRotationX,
        "GetRotationY", &CEulerRepresentationBase::GetRotationY,
        "GetRotationZ", &CEulerRepresentationBase::GetRotationZ,
        "SetRotation", [](CEulerRepresentationBase& repr, const CAngles& rot) { repr.SetRotation(rot); },
        "SetRotationX", &CEulerRepresentationBase::SetRotationX,
        "SetRotationY", &CEulerRepresentationBase::SetRotationY,
        "SetRotationZ", &CEulerRepresentationBase::SetRotationZ,
        "ResetRotation", &CEulerRepresentationBase::ResetRotation,
        "Rotation", sol::property
        (
            [](CEulerRepresentationBase& repr) { return repr.GetRotation(); },
            [](CEulerRepresentationBase& repr, const CAngles& rot) { return repr.SetRotation(rot); }
        )
    );

    State->new_usertype<CRotationEulerRepresentation>
    (
        "CRotationEulerRepresentation",
        sol::no_constructor,
        sol::base_classes, sol::bases<CEulerRepresentationBase>(),
        "GetTransform", &CRotationEulerRepresentation::GetTransform,
        "Transform", sol::property([](CRotationEulerRepresentation& repr) -> CTransformBase& { return repr.GetTransform(); })
    );

    State->new_usertype<CQuatEulerRepresentation>
    (
        "CQuatEulerRepresentation",
        sol::no_constructor,
        sol::base_classes, sol::bases<CEulerRepresentationBase>(),
        "GetQuaternion", &CQuatEulerRepresentation::GetQuaternion,
        "Quaternion", sol::property([](CQuatEulerRepresentation& repr) -> glm::quat& { return repr.GetQuaternion(); })
    );

    State->new_usertype<CTransformBase::CMeasurePack>
    (
        "CMeasurePack",
        sol::no_constructor,
        "Position", &CTransformBase::CMeasurePack::Position,
        "Rotation", &CTransformBase::CMeasurePack::Rotation,
        "Scale", &CTransformBase::CMeasurePack::Scale,
        "EulerRotation", sol::property([](CTransformBase::CMeasurePack& mpack) -> CQuatEulerRepresentation { return mpack.GetEulerRotation(); })
    );

    State->new_usertype<CTransformBoolDelta>
    (
        "CTransformBoolDelta",
        "Position", &CTransformBoolDelta::Position,
        "Rotation", &CTransformBoolDelta::Rotation,
        "Scale", &CTransformBoolDelta::Scale
    );

    State->new_usertype<CTransformBase>
    (
        "CTransformBase",
        sol::no_constructor,
        "GetPosition", &CTransformBase::GetPosition,
        "GetRotation", &CTransformBase::GetRotation,
        "GetScale", &CTransformBase::GetScale,
        "SetPosition", &CTransformBase::SetPosition,
        "SetRotation", &CTransformBase::SetRotation,
        "SetScale", &CTransformBase::SetScale,
        "GetForwardVector", &CTransformBase::GetForwardVector,
        "GetRightVector", &CTransformBase::GetRightVector,
        "GetUpVector", &CTransformBase::GetUpVector,
        "GetEulerRotation", &CTransformBase::GetEulerRotation,
        "SetFromMatrix", &CTransformBase::SetFromMatrix,
        "GetModelMatrix", &CTransformBase::GetModelMatrix,
        "ResetTransform", &CTransformBase::ResetTransform,
        "SetPRS", sol::overload
        (
            [](CTransformBase& trans, const CTransformBase::CMeasurePack& pack) { trans.SetPRS(pack); },
            [](CTransformBase& trans, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl) { trans.SetPRS(pos, rot, scl); },
            [](CTransformBase& trans, const CTransformBase& _trans) { trans.SetPRS(_trans.GetPRS()); },
            [](CTransformBase& trans, const CTransform& _trans) { trans.SetPRS(_trans.GetPRS()); }
        ),
        "SetPR", &CTransformBase::SetPR,
        "SetPS", &CTransformBase::SetPS,
        "SetRS", &CTransformBase::SetRS,
        "GetPRS", &CTransformBase::GetPRS,
        "AcknowledgeChange", &CTransformBase::AcknowledgeChange,
        "OnTransformChanged", sol::property([](CTransformBase& trans) -> HANDLER
        {
            return trans.OnTransformChanged;
        }),
        "GetBoolDelta", [](CTransformBase& trans, const CTransformBase::CMeasurePack& oldprs) { return GetTransformBoolDelta(oldprs, &trans); },
        "GetTransformDelta", [](CTransformBase& trans, const CTransformBase::CMeasurePack& oldprs) { return GetTransformDelta(oldprs, &trans); },

        "Forward", sol::property([](CTransformBase& trans) { return trans.GetForwardVector(); }),
        "Right", sol::property([](CTransformBase& trans) { return trans.GetRightVector(); }),
        "Up", sol::property([](CTransformBase& trans) { return trans.GetUpVector(); }),
        "Position", sol::property
        (
            [](CTransformBase& trans) { return trans.GetPosition(); },
            [](CTransformBase& trans, const glm::vec3& newpos) { return trans.SetPosition(newpos); }
        ),
        "Rotation", sol::property
        (
            [](CTransformBase& trans) { return trans.GetRotation(); },
            [](CTransformBase& trans, const glm::quat& newrot) { return trans.SetRotation(newrot); }
        ),
        "Scale", sol::property
        (
            [](CTransformBase& trans) { return trans.GetScale(); },
            [](CTransformBase& trans, const glm::vec3& newscl) { return trans.SetScale(newscl); }
        )
    );

    State->new_usertype<CTransform>
    (
        "CTransform",
        sol::no_constructor,
        sol::base_classes, sol::bases<CTransformBase>()
    );
}
