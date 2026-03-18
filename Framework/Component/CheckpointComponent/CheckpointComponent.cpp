#include "CheckpointComponent.h"
#include"../GameObject.h"
#include"../PlayerStatsComponent/PlayerStatsComponent.h"
#include"../ColliderComponent/ColliderComponent.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/Scene/GameScene/GameManager/GameManager.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../MagicCircleComponent/MagicCircleComponent.h"
#include"../GlowPartComponent/GlowPartComponent.h"
#include"../Src/Application/Scene/GameScene/GameScene.h"
#include"../Src/Application/Effect/Particle/ParticleSystem.h"

void CheckpointComponent::Awake()
{
}

void CheckpointComponent::Start()
{
	if (auto glow = m_owner->GetComponent<GlowPartComponent>())
	{
		m_activeColor = glow->GetGlowColor();
		m_activeBlinkSpeed = glow->GetBlinkSpeed();

		//初期状態はOFF
		glow->SetGlowActive(false);
		glow->SetEnableBlink(false);
	}
}

void CheckpointComponent::OnCollision(const CollisionInfo& info)
{
	if (!info.otherObject)return;

	if (m_isActivated) return;

	if (auto playerStats = info.otherObject->GetComponent<PlayerStatsComponent>())
	{
		ActivateCheckpoint(playerStats);
	}
}

void CheckpointComponent::ActivateCheckpoint(const std::shared_ptr<PlayerStatsComponent>& playerStats)
{
	if (m_isActivated) return;
	m_isActivated = true;

	//GameManagerに現在のチェックポイントとして登録
	GameManager::Instance().SetActiveCheckpoint(m_owner->shared_from_this());

	//起動音
	//KdAudioManager::Instance().Play("Asset/Sound/CheckpointActivate.wav", false);

	//発光演出ON
	if (auto glow = m_owner->GetComponent<GlowPartComponent>())
	{
		glow->SetGlowActive(true);
		glow->SetEnableBlink(true);
	}

	//キラキラエフェクト発生
	if (m_pGameScene)
	{
		if (auto pSys = m_pGameScene->GetParticleSystem("Sparcle"))
		{
			auto trans = m_owner->GetComponent<TransformComponent>();
			Math::Vector3 emitPos = trans->GetPos() + Math::Vector3(0, 1.5f, 0);

			pSys->Emit(emitPos, 30, Math::Vector3::Up, 1.0f);
		}
	}

	if (playerStats)
	{
		//このブロックの上面座標を計算してリスポーン地点とする
		auto selfTransform = m_owner->GetComponent<TransformComponent>();
		auto selfCollider = m_owner->GetComponent<ColliderComponent>();
		if (!selfTransform || !selfCollider || !selfCollider->GetShape()) return;

		const auto& aabb = selfCollider->GetShape()->GetBoundingBox();
		Math::Vector3 newRespawnPos = selfTransform->GetPos();
		newRespawnPos.y += aabb.Center.y + aabb.Extents.y;

		//リスポーン地点更新
		playerStats->SetInitialPos(newRespawnPos);
	}
}

void CheckpointComponent::Deactivate()
{
	m_isActivated = false;

	if (auto glow = m_owner->GetComponent<GlowPartComponent>())
	{
		glow->SetGlowActive(false);
		glow->SetEnableBlink(false);
	}
}

void CheckpointComponent::SetViewModel(const std::shared_ptr<GameViewModel>& viewModel)
{
	m_wpViewModel = viewModel;

	if (auto spViewModel = m_wpViewModel.lock())
	{
		//GameSceneへのポインタを保持しておく(パーティクル呼び出し用)
		m_pGameScene = dynamic_cast<GameScene*>(spViewModel->GetScene());
	}
	else
	{
		m_pGameScene = nullptr;
	}
}