#pragma once
#include"../Component.h"
#include"../ICollisionReceiver.h"

class TransformComponent;
class RigidbodyComponent;
class GameViewModel;
class GameScene;

class JumpBlockComponent :public Component, public ICollisionReceiver
{
public:
	JumpBlockComponent() { m_updatePriority = -101; }
	void Awake()override;
	void Start()override;
	void Update()override;
	void OnCollision(const CollisionInfo& info)override;
	void OnInspect()override;
	void Configure(const nlohmann::json& data);
	nlohmann::json ToJson() const override;
	const char* GetComponentName()const override { return "JumpBlockComponent"; }
	bool OnDrawGizmos(const EditorGizmoContext& context, GameScene& scene)override;
	void RequestStateChangeCommand();
	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel);

	//セッター/ゲッター
	void SetTargetPos(const Math::Vector3& pos);
	const Math::Vector3 GetTargetPos()const;

	const Math::Vector3& GetJumpDirection()const { return m_jumpDirection; }
	void SetJumpDirection(const Math::Vector3& direction) { m_jumpDirection = direction; }

	const Math::Vector3& GetOwnerPos()const;

	const float& GetJumpForce()const { return m_jumpForce; }
	void SetJumpForce(const float& force) { m_jumpForce = force; }

	const float& GetChargeDuration() { return m_chargeDuration; }
	void SetChargeDuration(const float& duration) { m_chargeDuration = duration; }

private:
	enum class State
	{
		Idle,
		Charging
	};
	State m_currentState = State::Idle;

	std::shared_ptr<TransformComponent> m_transform;
	std::weak_ptr<RigidbodyComponent> m_wpPlayerRigidBody;

	//パラメータ
	Math::Vector3 m_jumpDirection = Math::Vector3{ 0.0f,4.0f,0.0f }; //デフォルトは真上
	float m_jumpForce = 100.0f; //デフォルトのジャンプ力

	//--チャージ設定--
	float	m_chargeTimer = 0.0f;//チャージ開始からの経過時間
	float m_chargeDuration = 0.5f;//チャージ時間
	const float m_flickerSpeed = 50.0f;//明滅速度
	const float m_baseBrightness = 0.5f;//基本の明るさ
	const float m_flickerBrightness = 1.5f;//最大の明るさ

	bool m_isTargetGizmoDragging = false;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	GameScene* m_pGameScene = nullptr;
};