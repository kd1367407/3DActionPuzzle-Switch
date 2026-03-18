#pragma once
#include "../Component.h"

class PlayerStatsComponent;
class TransformComponent;
class RigidbodyComponent;

//奈落判定コンポーネント

class VoidOutComponent:public Component
{
public:
	void Awake()override;
	void Start()override;
	void PostUpdate()override;

	const char* GetComponentName() const override { return "VoidOutComponent"; }

private:
	std::weak_ptr<PlayerStatsComponent> m_wpStats;
	std::weak_ptr<TransformComponent> m_wpTrans;
	std::weak_ptr<RigidbodyComponent> m_wpRigid;

	float m_voidTimer = 0.0f;
	const float m_voidLimitTime = 2.0f;

	const float	m_rayDistance = 1000.0f;
};