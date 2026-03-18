#include "VoidOutComponent.h"
#include "../GameObject.h"
#include "../PlayerStatsComponent/PlayerStatsComponent.h"
#include "../TransformComponent/TransformComponent.h"
#include "../RigidbodyComponent/RigidbodyComponent.h"
#include"../Src/Application/System/PhysicsSystem.h"

void VoidOutComponent::Awake()
{
}

void VoidOutComponent::Start()
{
    m_wpStats = m_owner->GetComponent<PlayerStatsComponent>();
    m_wpTrans = m_owner->GetComponent<TransformComponent>();
    m_wpRigid = m_owner->GetComponent<RigidbodyComponent>();
}

void VoidOutComponent::PostUpdate()
{
    auto transform = m_wpTrans.lock();
    if (!transform)return;

    //^‰º‚ÉƒŒƒC‚ð”ò‚Î‚·
    RayInfo ray;
    ray.m_start = transform->GetPos();
    ray.m_dir = { 0, -1, 0 }; // ^‰º
    ray.m_maxDistance = m_rayDistance;

    RayResult result;
    bool hitStage = PhysicsSystem::Instance().Raycast(ray, result, LayerAll, m_owner);

    if (hitStage)
    {
        m_voidTimer = 0.0f;
    }
    else
    {
        float dt = Application::Instance().GetDeltaTime();
        m_voidTimer += dt;

        if (m_voidTimer >= m_voidLimitTime)
        {
            if (auto stats = m_wpStats.lock())
            {
                transform->SetPos(stats->GetInitialPos());

                if (auto rigid = m_wpRigid.lock())
                {
                    rigid->SetVelocity(Math::Vector3::Zero);
                    rigid->m_force = Math::Vector3::Zero;
                }

                m_voidTimer = 0.0f;
                Application::Instance().AddLog("Void Out Respawn!");
            }
        }
    }
}
