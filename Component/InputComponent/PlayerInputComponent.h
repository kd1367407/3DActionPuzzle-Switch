#pragma once
#include"../Component.h"

class CommandInvoker;
class RigidbodyComponent;
class GravityComponent;
class TPSCameraComponent;
class RenderComponent;

class PlayerInputComponent :public Component
{
public:
	PlayerInputComponent() { m_updatePriority = 100; }//rigidとgravityが必要なのでそれらより後に実行

	void Awake()override;
	void Start()override;

	void Update()override;

	const char* GetComponentName()const override { return "PlayerInputComponent"; }

	void EnableInput(bool enable) { m_inputEnabled = enable; }

	//操作命令を送る先のInvokerを設定
	void SetInvoker(std::shared_ptr<CommandInvoker> invoker);

	void SetCamera(std::shared_ptr<TPSCameraComponent> spCamera) { m_wpCamera = spCamera; }

	//外部からアニメーションをさせる
	void PlayAction();

private:
	enum class AnimState
	{
		Idle,
		Walk,
		Jump,
		Action
	};

	void ChangeAnimation(AnimState nextState);
	void UpdateAnimation();

	std::shared_ptr<CommandInvoker> m_invoker;
	std::shared_ptr<RigidbodyComponent> m_rigid;
	std::shared_ptr<GravityComponent> m_gravity;
	std::weak_ptr<TPSCameraComponent> m_wpCamera;
	std::weak_ptr<RenderComponent> m_wpRender;

	KdAnimator m_animator;
	AnimState m_currState = AnimState::Idle;

	bool m_inputEnabled = true;
	float m_coyoteTimeCounter = 0.0f;
	float m_maxSpeed = 0.0f;
	float m_moveForce = 0.0f;
	float m_controlForce = 0.0f;
	float m_brakeForce = 0.0f;
	float m_jumpPower = 0.0f;
	float m_coyoteTimeDuration = 0.0f;
};