#pragma once

#include "CAngle.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CScriptFieldsManager.h"
#include "CCallbackHandler.h"
#include "CScriptObject.h"

class CEulerRepresentationBase
{
public:
	virtual ~CEulerRepresentationBase() = default;

	virtual void m_setQuat(const glm::quat& q) = 0;
	virtual glm::quat m_getQuat() const = 0;

	CAngles GetRotation() const;

	CAngle GetRotationX() const;
	CAngle GetRotationY() const;
	CAngle GetRotationZ() const;

	void SetRotation(const CAngles& rot);
	void SetRotation(CAngle x, CAngle y, CAngle z);

	void SetRotationX(CAngle x);
	void SetRotationY(CAngle y);
	void SetRotationZ(CAngle z);

	void ResetRotation();

    operator CAngles() const;
private:
	void m_wrapEulerToQuat(const CAngles& euler);
	CAngles m_wrapQuatToEuler() const;
};

class CTransformBase;
class CRotationEulerRepresentation : public CEulerRepresentationBase
{
public:
	CRotationEulerRepresentation(CTransformBase& transform);

	void m_setQuat(const glm::quat& q) override;
	glm::quat m_getQuat() const override;

	CTransformBase& GetTransform();
private:
	CTransformBase& m_transform;
};

class CQuatEulerRepresentation : public CEulerRepresentationBase
{
public:
	CQuatEulerRepresentation(glm::quat& quat);

	void m_setQuat(const glm::quat& q) override;
	glm::quat m_getQuat() const override;

	glm::quat& GetQuaternion();
private:
	glm::quat& m_quat;
};

class CTransformBase : public CScriptObject //TODO matrix caching
{
public:
	struct CMeasurePack
	{
		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat Rotation = glm::identity<glm::quat>();
		glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

		CQuatEulerRepresentation GetEulerRotation();
	};

	CTransformBase();
	CTransformBase(const CMeasurePack& pack);

    virtual glm::vec3 m_getPosition() const = 0;
    virtual glm::quat m_getRotation() const = 0;
    virtual glm::vec3 m_getScale() const = 0;

    virtual void m_setPosition(const glm::vec3& pos) = 0;
    virtual void m_setRotation(const glm::quat& qua) = 0;
    virtual void m_setScale(const glm::vec3& scl) = 0;

	glm::vec3 GetPosition() const;
	glm::quat GetRotation() const;
	glm::vec3 GetScale() const;

	void SetPosition(const glm::vec3& pos);
	void SetRotation(const glm::quat& rot);
	void SetScale(const glm::vec3& scl);

	glm::vec3 GetForwardVector() const;
	glm::vec3 GetRightVector() const;
	glm::vec3 GetUpVector() const;

	CRotationEulerRepresentation GetEulerRotation();

	void SetFromMatrix(const glm::mat4x4 matrix);
	glm::mat4 GetModelMatrix() const;
	void ResetTransform();

	void SetPRS(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl);
	void SetPRS(const CMeasurePack& pack);
	void SetPR(const glm::vec3& pos, const glm::quat& rot);
	void SetPS(const glm::vec3& pos, const glm::vec3& scl);
	void SetRS(const glm::quat& rot, const glm::vec3& scl);

	CMeasurePack GetPRS() const;
	CCallbackHandler<void, CMeasurePack, CTransformBase*> OnTransformChanged;

	CTransformBase& operator=(const CTransformBase& rhs);
	glm::mat4 operator*(const CTransformBase& rhs) const;

	bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
	
	virtual std::unique_ptr<CScriptFieldBase> V_SetupPositionProp() = 0;
	virtual std::unique_ptr<CScriptFieldBase> V_SetupRotationProp() = 0;
	virtual std::unique_ptr<CScriptFieldBase> V_SetupScaleProp() = 0;

	bool AcknowledgeChange() const;

	CScriptFieldsManager ScriptFields;
private:
	mutable bool m_Changed = true;
};

class CTransform : public CTransformBase
{
public:
    CTransform() = default;
    CTransform(const CTransform& other);

	CTransform& operator=(const CTransform& rhs);

	std::unique_ptr<CScriptFieldBase> V_SetupPositionProp() override;
	std::unique_ptr<CScriptFieldBase> V_SetupRotationProp() override;
	std::unique_ptr<CScriptFieldBase> V_SetupScaleProp() override;

    glm::vec3 m_getPosition() const override;
    glm::quat m_getRotation() const override;
    glm::vec3 m_getScale() const override;

    void m_setPosition(const glm::vec3& pos) override;
    void m_setRotation(const glm::quat& qua) override;
    void m_setScale(const glm::vec3& scl) override;
private:
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::quat m_rotation = glm::identity<glm::quat>();
	glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

struct CTransformBoolDelta
{
	bool Position = true;
	bool Rotation = true;
	bool Scale = true;
};

CTransformBoolDelta GetTransformBoolDelta(const CTransformBase::CMeasurePack& oldprs, CTransformBase* _this);
CTransformBase::CMeasurePack GetTransformDelta(const CTransformBase::CMeasurePack& oldprs, CTransformBase* _this);