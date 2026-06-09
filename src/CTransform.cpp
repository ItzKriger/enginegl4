#include "CTransform.h"
#include "U_Scripting.h"

CRotationEulerRepresentation::CRotationEulerRepresentation(CTransformBase& transform) : m_transform(transform) {}
CQuatEulerRepresentation::CQuatEulerRepresentation(glm::quat& quat) : m_quat(quat) {}

void CEulerRepresentationBase::m_wrapEulerToQuat(const CAngles& euler)
{
	glm::quat q = glm::quat(glm::vec3(euler.x.asRadians(), euler.y.asRadians(), euler.z.asRadians()));
    m_setQuat(q);
}

CAngles CEulerRepresentationBase::m_wrapQuatToEuler() const
{
	glm::vec3 euler_glm = glm::eulerAngles(m_getQuat());
	return euler_glm;
}

CAngles CEulerRepresentationBase::GetRotation() const
{
	return m_wrapQuatToEuler();
}

CAngle CEulerRepresentationBase::GetRotationX() const
{
	return m_wrapQuatToEuler().x;
}

CAngle CEulerRepresentationBase::GetRotationY() const
{
	return m_wrapQuatToEuler().y;
}

CAngle CEulerRepresentationBase::GetRotationZ() const
{
	return m_wrapQuatToEuler().z;
}

void CEulerRepresentationBase::SetRotation(const CAngles& rot)
{
	m_wrapEulerToQuat(rot);
}

void CEulerRepresentationBase::SetRotationX(CAngle x)
{
	CAngles rot = m_wrapQuatToEuler();
	m_wrapEulerToQuat(CAngles(x, rot.y, rot.z));
}

void CEulerRepresentationBase::SetRotationY(CAngle y)
{
	CAngles rot = m_wrapQuatToEuler();
	m_wrapEulerToQuat(CAngles(rot.x, y, rot.z));
}

void CEulerRepresentationBase::SetRotationZ(CAngle z)
{
	CAngles rot = m_wrapQuatToEuler();
	m_wrapEulerToQuat(CAngles(rot.x, rot.y, z));
}

void CEulerRepresentationBase::ResetRotation()
{
	m_wrapEulerToQuat(CAngles(0.0f, 0.0f, 0.0f));
}

CQuatEulerRepresentation CTransformBase::CMeasurePack::GetEulerRotation() { return CQuatEulerRepresentation(Rotation); }

CEulerRepresentationBase::operator CAngles() const
{
    return m_wrapQuatToEuler();
}

CTransformBase& CRotationEulerRepresentation::GetTransform()
{
	return m_transform;
}

void CRotationEulerRepresentation::m_setQuat(const glm::quat& q)
{
    return m_transform.SetRotation(q);
}

glm::quat CRotationEulerRepresentation::m_getQuat() const
{
    return m_transform.GetRotation();
}

void CQuatEulerRepresentation::m_setQuat(const glm::quat& q)
{
    m_quat = q;
}

glm::quat CQuatEulerRepresentation::m_getQuat() const
{
    return m_quat;
}

glm::quat& CQuatEulerRepresentation::GetQuaternion()
{
    return m_quat;
}

CTransformBase::CTransformBase(const CMeasurePack& pack) : CTransformBase()
{
    SetPRS(pack);
}

glm::vec3 CTransformBase::GetPosition() const
{
    return m_getPosition();
}

glm::quat CTransformBase::GetRotation() const
{
    return m_getRotation();
}

glm::vec3 CTransformBase::GetScale() const
{
    return m_getScale();
}

void CTransformBase::SetPosition(const glm::vec3& pos)
{
    if(GetPosition() == pos) { return; }

    CMeasurePack old = GetPRS();
    m_setPosition(pos);

    OnTransformChanged(old, this);
}

void CTransformBase::SetRotation(const glm::quat& rot)
{
    if(GetRotation() == rot) { return; }

    CMeasurePack old = GetPRS();
    m_setRotation(rot);

    OnTransformChanged(old, this);
}

void CTransformBase::SetScale(const glm::vec3& scl)
{
    if(GetScale() == scl) { return; }

    CMeasurePack old = GetPRS();
    m_setScale(scl);

    OnTransformChanged(old, this);
}

glm::vec3 CTransformBase::GetForwardVector() const
{
    static const glm::vec3 forw = glm::vec3(0, 0, -1);
    return glm::mat3_cast(GetRotation()) * forw;
}

glm::vec3 CTransformBase::GetRightVector() const
{
    static const glm::vec3 right = glm::vec3(1, 0, 0);
    return glm::mat3_cast(GetRotation()) * right;
}

glm::vec3 CTransformBase::GetUpVector() const
{
    static const glm::vec3 up = glm::vec3(0, 1, 0);
    return glm::mat3_cast(GetRotation()) * up;
}

CRotationEulerRepresentation CTransformBase::GetEulerRotation() { return CRotationEulerRepresentation(*this); }

void CTransformBase::SetFromMatrix(const glm::mat4x4 matrix)
{
    auto pos = glm::vec3(matrix[3]);

    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(matrix[0]));
    scale.y = glm::length(glm::vec3(matrix[1]));
    scale.z = glm::length(glm::vec3(matrix[2]));

    glm::mat4 rotationMatrix = matrix;

    rotationMatrix[0] /= scale.x;
    rotationMatrix[1] /= scale.y;
    rotationMatrix[2] /= scale.z;

    glm::quat rotation = glm::quat_cast(rotationMatrix);
    SetPRS({ pos, rotation, scale });
}

glm::mat4 CTransformBase::GetModelMatrix() const
{
	glm::mat4 translate_matrix = glm::translate(glm::mat4(1.0f), GetPosition());
    glm::mat4 rotation_matrix = glm::mat4_cast(GetRotation());
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), GetScale());
    return translate_matrix * rotation_matrix * scale_matrix;
}

void CTransformBase::ResetTransform()
{
	SetPRS(glm::vec3(0.0f, 0.0f, 0.0f), glm::identity<glm::quat>(), glm::vec3(1.0f, 1.0f, 1.0f));
}

CTransformBase::CMeasurePack CTransformBase::GetPRS() const
{
	return { GetPosition(), GetRotation(), GetScale() };
}

CTransformBase& CTransformBase::operator=(const CTransformBase& rhs)
{
    SetPRS(rhs.GetPRS());
    return *this;
}

CTransform& CTransform::operator=(const CTransform& rhs)
{
    SetPRS(rhs.GetPRS());
    return *this;
}

glm::mat4 CTransformBase::operator*(const CTransformBase& rhs) const
{
	return GetModelMatrix() * rhs.GetModelMatrix();
}

void CTransformBase::SetPRS(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
{
    CMeasurePack old = GetPRS();
    
    m_setPosition(pos);
    m_setRotation(rot);
    m_setScale(scl);

    OnTransformChanged(old, this);
}

void CTransformBase::SetPRS(const CMeasurePack& pack)
{
    CMeasurePack old = GetPRS();
    
    m_setPosition(pack.Position);
    m_setRotation(pack.Rotation);
    m_setScale(pack.Scale);

    OnTransformChanged(old, this);
}

void CTransformBase::SetPR(const glm::vec3& pos, const glm::quat& rot)
{
    CMeasurePack old = GetPRS();
    
    m_setPosition(pos);
    m_setRotation(rot);

    OnTransformChanged(old, this);
}

void CTransformBase::SetPS(const glm::vec3& pos, const glm::vec3& scl)
{
    CMeasurePack old = GetPRS();
    
    m_setPosition(pos);
    m_setScale(scl);

    OnTransformChanged(old, this);
}

void CTransformBase::SetRS(const glm::quat& rot, const glm::vec3& scl)
{
    CMeasurePack old = GetPRS();
    
    m_setRotation(rot);
    m_setScale(scl);

    OnTransformChanged(old, this);
}

CTransformBase::CTransformBase()
{
    auto scForward = std::make_unique<CFunctionalScriptField>(
    [this](sol::state_view st) -> sol::object
    {
        return sol::make_object_userdata<glm::vec3>(st, GetForwardVector());
        //return ScriptUtils::CreateSharedObject<glm::vec3>(st, GetForwardVector()).second;
    });

    auto scRight = std::make_unique<CFunctionalScriptField>(
    [this](sol::state_view st) -> sol::object
    {
        return sol::make_object_userdata<glm::vec3>(st, GetRightVector());
        //return ScriptUtils::CreateSharedObject<glm::vec3>(st, GetRightVector()).second;
    });

    auto scUp = std::make_unique<CFunctionalScriptField>(
    [this](sol::state_view st) -> sol::object
    {
        return sol::make_object_userdata<glm::vec3>(st, GetUpVector());
        //return ScriptUtils::CreateSharedObject<glm::vec3>(st, GetUpVector()).second;
    });
    
    auto scModelMatrix = std::make_unique<CFunctionalScriptField>(
    [this](sol::state_view st) -> sol::object
    {
        return ScriptUtils::ToObject<glm::mat4>(GetModelMatrix(), st); //TODO shared_ptr of matrix
    });

    ScriptFields.AddField("forward", std::move(scForward));
    ScriptFields.AddField("right", std::move(scRight));
    ScriptFields.AddField("up", std::move(scUp));

    ScriptFields.AddField("modelMatrix", std::move(scModelMatrix));

    OnTransformChanged += [this](CMeasurePack old, CTransformBase* _this) { m_Changed = true; };
}

bool CTransformBase::AcknowledgeChange() const
{
    bool ret = m_Changed;
    m_Changed = false;
    return ret;
}

std::unique_ptr<CScriptFieldBase> CTransform::V_SetupPositionProp()
{
    return std::make_unique<CFunctionalScriptField>
    (
        [this](sol::state_view st) -> sol::object
        {
            return ScriptUtils::ToObject(this->m_position, st);
        },
        [this](sol::object obj) -> void
        {
            SetPosition(ScriptUtils::FromObject<glm::vec3>(obj));
        }
    );
}

std::unique_ptr<CScriptFieldBase> CTransform::V_SetupRotationProp()
{
    return std::make_unique<CFunctionalScriptField>
    (
        [this](sol::state_view st) -> sol::object
        {
            return ScriptUtils::ToObject(this->m_rotation, st);
        },
        [this](sol::object obj) -> void
        {
            SetRotation(ScriptUtils::FromObject<glm::quat>(obj));
        }
    );
}

std::unique_ptr<CScriptFieldBase> CTransform::V_SetupScaleProp()
{
    return std::make_unique<CFunctionalScriptField>
    (
        [this](sol::state_view st) -> sol::object
        {
            return ScriptUtils::ToObject(this->m_scale, st);
        },
        [this](sol::object obj) -> void
        {
            SetScale(ScriptUtils::FromObject<glm::vec3>(obj));
        }
    );
}

bool CTransformBase::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    if(!ScriptFields.IsFieldExist<std::string>("position"))
    {
        auto scPosition = V_SetupPositionProp(); 
        auto scRotation = V_SetupRotationProp();
        auto scScale = V_SetupScaleProp();

        ScriptFields.AddField("position", std::move(scPosition));
        ScriptFields.AddField("rotation", std::move(scRotation));
        ScriptFields.AddField("scale", std::move(scScale));
    }

    table.set_function("resetTransform", [this]() { ResetTransform(); });

    sol::table onTransformChanged = OnTransformChanged.GetScriptTable(state);
    table["onTransformChanged"] = onTransformChanged;

    ScriptFields.CreateMetaTable(table);
    return true;
}

glm::vec3 CTransform::m_getPosition() const
{
    return m_position;
}

glm::quat CTransform::m_getRotation() const
{
    return m_rotation;
}

glm::vec3 CTransform::m_getScale() const
{
    return m_scale;
}

void CTransform::m_setPosition(const glm::vec3& pos)
{
    for(size_t i = 0; i < pos.length(); i++) { if(std::isnan(pos[i])) { return; } }
    m_position = pos;
}

void CTransform::m_setRotation(const glm::quat& qua)
{
    for(size_t i = 0; i < qua.length(); i++) { if(std::isnan(qua[i])) { return; } }
    m_rotation = qua;
}

void CTransform::m_setScale(const glm::vec3& scl)
{
    for(size_t i = 0; i < scl.length(); i++) { if(std::isnan(scl[i])) { return; } }
    m_scale = scl;
}

CTransform::CTransform(const CTransform& other) : m_position(other.m_position), m_rotation(other.m_rotation), m_scale(other.m_scale) {}

CTransformBoolDelta GetTransformBoolDelta(const CTransformBase::CMeasurePack& oldprs, CTransformBase* _this)
{
    return
    {
        oldprs.Position != _this->GetPosition(),
        oldprs.Rotation != _this->GetRotation(),
        oldprs.Scale != _this->GetScale()
    };
}

CTransformBase::CMeasurePack GetTransformDelta(const CTransformBase::CMeasurePack& oldprs, CTransformBase* _this)
{
    return
    {
        _this->GetPosition() - oldprs.Position,
        _this->GetRotation() - oldprs.Rotation,
        _this->GetScale() - oldprs.Scale
    };
}
