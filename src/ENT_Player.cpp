#include "ENT_Player.h"
#include "CEngine.h"
#include "U_ShortAPI.h"
#include "CPhysicalEntity.h"
#include "CDrawableEntity.h"
#include "CGame.h"
#include "CTransformable.h"
#include "glm/gtx/norm.hpp"

#include "CBulletPhysics.h"

void ENT_Player::V_Init()
{
    if(!IsConnectedClient())
    {
        AddInitParam("modelname", new CWrapable<std::string>("gordon.emdl"));
        AddInitParam("spaceid", new CWrapable<int>(0));
        AddInitParam("phys_worldid", new CWrapable<int>(0));
        AddInitParam("phys_collision", new CWrapable<std::string>("player.ecol"));
        AddInitParam("phys_type", new CWrapable<std::string>("ghost"));
        AddInitParam("phys_mass", new CWrapable<float>(0.0f));
    }

    SetEntityModel(this);
    Components.CreateComponent<CPhysicalEntity>(false);

    auto drawable = Components.GetComponentTyped<CDrawableEntity>();
    auto physical = Components.GetComponentTyped<CPhysicalEntity>();

    if(drawable)
    {
        drawable->Init();
    }

    if(physical)
    {
        physical->Init();
    }
}

void ENT_Player::V_Update()
{
    float deltaTime = CEngine::GetInstance()->Time.GetDeltaTime();
    auto physical = Components.GetComponentTyped<CPhysicalEntity>();

    if(!physical || !physical->WorldUnit) { return; }

    if(!onGround)
    {
        velocity = physical->WorldUnit->GetWorld()->GetGravity() * deltaTime;
    }

    if(glm::length(userMovement) > 0.001f)
    {
        velocity += userMovement;
    }

    glm::vec3 move = velocity * deltaTime; //applying deltaTime two times?
    StepMove(move);

    DetectGround();
}

void ENT_Player::DetectGround()
{
    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto& physEngine = game->PhysicsEngine;

    auto physical = Components.GetComponentTyped<CPhysicalEntity>();
    auto transformable = Components.GetComponentTyped<CTransformable>();

    auto _thisUnit = physical->WorldUnit;
    auto thisUnit = dynamic_cast<CGhostBody*>(_thisUnit);
    auto thisRigidUnit = dynamic_cast<CRigidBody*>(_thisUnit);
    auto thisKinematicUnit = dynamic_cast<CKinematicBody*>(_thisUnit);

    //Log::Instance() << "_thisUnit: " << _thisUnit << Log::Endl;
    //Log::Instance() << "thisUnit: " << thisUnit << Log::Endl;
    //Log::Instance() << "thisRigidUnit: " << thisRigidUnit << Log::Endl;
    //Log::Instance() << "thisKinematicUnit: " << thisKinematicUnit << Log::Endl;

    CTransform start = transformable->Transform;
    CTransform end = start;

    auto world = physEngine->GetWorld(0);

    onGround = false;
    groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);

    //btWorld->BulletWorld->updateSingleAabb(_btGhost);
    //btWorld->BulletWorld->performDiscreteCollisionDetection();

    auto overlaps = world->V_GetGhostOverlaps(thisUnit);
    int numOverlaps = overlaps->GetNumOverlappingUnits();

    glm::vec3 upVector(0.0f, 1.0f, 0.0f);

    float kGroundEpsilon = 0.05f;

    float bestDot = -1.0f;
    glm::vec3 bestNormal = upVector;

    for (int i = 0; i < numOverlaps; ++i)
    {
        auto overlapUnit = overlaps->GetOverlappingUnit(i);
        auto overlapRigid = dynamic_cast<CRigidBody*>(overlapUnit);

        if(!overlapRigid) { continue; }
        auto manifolds = overlaps->GetContactManifolds(overlapUnit);

        //Log::Instance() << "Overlap #" << i << " with nmanifolds: " << manifolds.size() << " overlapping rigid " << overlapRigid->GetCollision()->Name << Log::Endl;

        for (int m = 0; m < manifolds.size(); ++m)
        {
            auto& manifold = manifolds.at(m);
            int ncontacts = manifold->GetNumContacts();

            //Log::Instance() << "Manifold #" << i << ":" << m << " with ncontacts: " << manifold->GetNumContacts() << Log::Endl;

            for (int p = 0; p < ncontacts; ++p)
            {
                //Log::Instance() << "Contact #" << i << ":" << m << ":" << p << Log::Endl;
                
                auto contact = manifold->GetContact(p);

                Log::Instance() << "Distance: " << contact->GetDistance() << Log::Endl;
                //Log::Instance() << "NormalWorldOnB: " << contact->GetNormalWorldOnB() << Log::Endl;

                if (contact->GetDistance() < kGroundEpsilon)
                {
                    glm::vec3 n = contact->GetNormalWorldOnB();
                    if(manifold->GetUnitB() == physical->WorldUnit)
                    {
                        n = -n;
                    }

                    float dot = glm::dot(n, glm::vec3(0.0f, 1.0f, 0.0f));
                    if (dot > 0.6f && dot > bestDot)
                    {
                        bestDot = dot;
                        bestNormal = n;
                    }
                }
            }
        }
    }

    if (bestDot > 0.6f)
    {
        onGround = true;
        groundNormal = bestNormal;

        Log::Instance() << "ONGROUND\n";
        velocity.y = 0.0f;
    }
}

size_t moves = 0;

void ENT_Player::StepMove(const glm::vec3& _move)
{
    moves++;
    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto& physEngine = game->PhysicsEngine;

    auto physical = Components.GetComponentTyped<CPhysicalEntity>();
    auto transformable = Components.GetComponentTyped<CTransformable>();

    CTransform start = transformable->Transform;
    CTransform end = start;

    end.SetPosition(start.GetPosition() + velocity);

    auto world = physEngine->GetWorld(0);
    std::vector<CWorldUnit*> excludeList;

    excludeList.push_back(physical->WorldUnit);

    auto result = world->V_ConvexSweep(physical->WorldUnit->GetCollision(), start, end, nullptr, std::move(excludeList));

    if(result->HasHit())
    {
        auto& hit = result->GetHit();

        //Log::Instance() << "HIT at point " << hit.HitPointWorld << " to unit " << ((hit.HitUnit && hit.HitUnit->GetCollision()) ? hit.HitUnit->GetCollision()->Name : "nullptr") << " with normal " << hit.HitNormalWorld << Log::Endl;
        float fraction = hit.HitFraction;
        glm::vec3 hitNormal = hit.HitNormalWorld;

        glm::vec3 newPos = start.GetPosition() + _move * fraction;

        glm::vec3 remaining = _move * (1.0f - fraction);
        glm::vec3 slide = remaining - hitNormal * glm::dot(remaining, hitNormal);

        if (glm::length2(slide) > 0.0001f && moves <= 100)
        {
            StepMove(slide);
        }
        else
        {
            //end.SetPosition(newPos);
            //transformable->Transform.SetPRS(end.GetPRS());
            //Log::Instance() << "Should move2 to " << end.GetPosition() << " with velocity " << velocity << Log::Endl;
        }
    }
    else
    {
        transformable->Transform.SetPRS(end.GetPRS());
        //Log::Instance() << "Should move to " << end.GetPosition() << " with velocity " << velocity <<  Log::Endl;
    }
    moves--;
}

LINK_ENTITY_TO_CLASS(ENT_Player, player);