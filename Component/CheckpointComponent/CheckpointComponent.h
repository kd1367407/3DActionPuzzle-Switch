#pragma once
#include"../Component.h"
#include"../ICollisionReceiver.h"

class PlayerStatsComponent;
class GameViewModel;
class GameScene;

class CheckpointComponent :public Component, public ICollisionReceiver
{
public:
	void Awake()override;
	void Start() override;
	void OnCollision(const CollisionInfo& info) override;

	void ActivateCheckpoint(const std::shared_ptr<PlayerStatsComponent>& playerStats);
	void Deactivate();

	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel);


	const char* GetComponentName() const override { return "CheckpointComponent"; }

private:
	bool m_isActivated = false;

	std::weak_ptr<GameViewModel> m_wpViewModel;
	GameScene* m_pGameScene = nullptr;

	Math::Vector3 m_activeColor = Math::Vector3::Zero;
	float m_activeBlinkSpeed = 0.0f;
};