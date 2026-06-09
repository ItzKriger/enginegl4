#pragma once
#include "CEntity.h"
#include "glm/glm.hpp"

class ENT_Player : public CEntity
{
public:
    void V_Init() override;

    void StepMove(const glm::vec3& _move);
    void DetectGround();

    void Jump();
    void V_Update() override;

    bool onGround = false;
    glm::vec3 groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 userMovement = glm::vec3(0.0f, 0.0f, 0.0f);

    DEFINE_ENTITY();
};
