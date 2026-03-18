#pragma once
#include"../Component.h"
#include"../ITriggerReceiver.h"

class GameObject;
class GameViewModel;
class GameScene;

class TransferBlockComponent :public Component, public ITriggerReceiver
{
public:
	void Awake()override;
	void Start()override;
	void Update()override;
	void OnTriggerEnter(const CollisionInfo& info)override;

	void SetPartner(const std::shared_ptr<GameObject>& partner) { m_wpPartner = partner; }
	std::shared_ptr<GameObject> GetPartner() { return m_wpPartner.lock(); }
	int GetTransferID()const { return m_transfarID; }
	void SetTransferID(int id) { m_transfarID = id; }
	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel);

	void OnInspect()override;

	virtual void Configure(const nlohmann::json& data);

	nlohmann::json ToJson() const override;

	void UpdateTeleportSequence();
	void	StartTeleportSequence(const std::shared_ptr<GameObject>& player);

	const char* GetComponentName()const override { return "TransferBlockComponent"; }

private:
	enum class TeleportState
	{
		Idle,
		Dissolving,//プレイヤーが消える
		CameraMoving,//カメラが移動する
		Reconstructing,//プレイヤーが現れる
		Cooldown//クールダウン
	};

	TeleportState m_teleportState = TeleportState::Idle;
	float	m_teleportTimer = 0.0f;

	std::weak_ptr<GameObject> m_wpPlayer;
	Math::Vector3 m_destinationPos; //移動先の座標

	int m_transfarID = 0;
	std::weak_ptr<GameObject> m_wpPartner;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	GameScene* m_pGameScene = nullptr;
	float m_coolDown = 0.0f;
};