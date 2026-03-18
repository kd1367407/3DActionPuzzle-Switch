#include "TransferBlockComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../RigidbodyComponent/RigidbodyComponent.h"
#include"../BlockDataComponent/BlockDataComponent.h"
#include"../ColliderComponent/ColliderComponent.h"
#include"../ColliderComponent/Shape.h"
#include"../Src/Application/main.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../InputComponent/PlayerInputComponent.h"
#include"../DissolveComponent/DissolveComponent.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/Scene/BaseScene/BaseScene.h"
#include"../CameraComponent/TPSCameraComponent/TPSCameraComponent.h"
#include"../ICollisionReceiver.h"
#include"../Src/Application/Scene/GameScene/GameScene.h"
#include"../Src/Application/Effect/Particle/ParticleSystem.h"

void TransferBlockComponent::Awake()
{
}

void TransferBlockComponent::Start()
{
	//データコンポーネントがなければ追加してTransferタイプを設定
	if (auto dataComp = m_owner->GetComponent<BlockDataComponent>(); !dataComp)
	{
		auto newDataComp = m_owner->GetComponent<BlockDataComponent>();
		newDataComp->SetType(BlockType::Transfer);
	}
}

void TransferBlockComponent::Update()
{
	if (m_coolDown > 0.0f)
	{
		m_coolDown -= Application::Instance().GetDeltaTime();
	}

	if (m_teleportState != TeleportState::Idle)
	{
		UpdateTeleportSequence();
	}
}

void TransferBlockComponent::OnTriggerEnter(const CollisionInfo& info)
{
	//クールダウン中、または相手がplayer以外は処理しない
	if (m_coolDown > 0.0f || !info.otherObject || info.otherObject->GetName() != "Player") return;

	if (m_teleportState != TeleportState::Idle)return;

	//パートナー(移動先)がいなければ無視
	auto partner = m_wpPartner.lock();
	if (!partner)return;

	StartTeleportSequence(info.otherObject);
}

void TransferBlockComponent::SetViewModel(const std::shared_ptr<GameViewModel>& viewModel)
{
	m_wpViewModel = viewModel;

	if (auto spViewModel = m_wpViewModel.lock())
	{
		m_pGameScene = dynamic_cast<GameScene*>(spViewModel->GetScene());
	}
	else
	{
		m_pGameScene = nullptr;
	}
}

void TransferBlockComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Transfer Compoent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int id = m_transfarID;
		if (ImGui::InputInt("TransferID", &id))
		{
			m_transfarID = id;
		}

		//ID変更完了時にViewModelにペアリング更新を依頼
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (auto spViewModel = m_wpViewModel.lock())
			{
				spViewModel->PairTransferBlocks();
			}
		}

		std::string partnerName = "None";
		if (auto spPartner = m_wpPartner.lock())
		{
			partnerName = spPartner->GetName();
		}
		ImGui::InputText("Partner", &partnerName[0], partnerName.size(), ImGuiInputTextFlags_ReadOnly);
	}
}

void TransferBlockComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("TransferBlockComponent"))return;

	const auto& transferBlockData = data.at("TransferBlockComponent");

	m_transfarID = JsonHelper::GetInt(transferBlockData, "transferID", 0);
}

nlohmann::json TransferBlockComponent::ToJson() const
{
	nlohmann::json j;
	j["transferID"] = m_transfarID;

	return j;
}

void TransferBlockComponent::UpdateTeleportSequence()
{
	float	deltatime = Application::Instance().GetDeltaTime();
	auto player = m_wpPlayer.lock();

	if (!player)
	{
		m_teleportState = TeleportState::Idle;
		return;
	}

	switch (m_teleportState)
	{
	case TransferBlockComponent::TeleportState::Dissolving:
		//プレイヤーがディゾルブで消えていくフェーズ
		m_teleportTimer -= deltatime;

		//吸い込みエフェクト発生
		if (player && m_teleportTimer > 0.0f)
		{
			if (m_pGameScene)
			{
				if (auto pSys = m_pGameScene->GetParticleSystem("Implosion"))
				{
					Math::Vector3 center = player->GetComponent<TransformComponent>()->GetPos();
					center.y += 0.5f;

					pSys->Emit(center, 5, Math::Vector3::Zero, 0.0f);
				}
			}
		}

		//消滅完了したら移動処理へ
		if (m_teleportTimer < 0.0f)
		{
			//player移動
			if (auto transform = player->GetComponent<TransformComponent>())
			{
				transform->SetPos(m_destinationPos);
			}

			//カメラ強制移動(カットを割らずにカメラだけ高速移動させる演出)
			if (auto scene = SceneManager::Instance().GetCurrentScene())
			{
				if (auto camera = scene->GetActiveCamera())
				{
					if (auto tpsCam = std::dynamic_pointer_cast<TPSCameraComponent>(camera))
					{
						tpsCam->SetForceMoveTarget(m_destinationPos, 0.5f);
						m_teleportTimer = 0.5f;
					}
				}
			}
			m_teleportState = TeleportState::CameraMoving;
		}
		break;
	case TransferBlockComponent::TeleportState::CameraMoving:
		//カメラ移動待ちフェーズ
		m_teleportTimer -= deltatime;

		//カメラ移動完了したら再構築開始
		if (m_teleportTimer <= 0.0f)
		{
			if (auto dissolve = player->GetComponent<DissolveComponent>())
			{
				dissolve->FadeIn();
				m_teleportTimer = dissolve->GetFadeDuration();
			}
			m_teleportState = TeleportState::Reconstructing;
		}
		break;
	case TransferBlockComponent::TeleportState::Reconstructing:
		//プレイヤーが現れるフェーズ
		m_teleportTimer -= deltatime;

		//再構築完了したら操作可能にする
		if (m_teleportTimer <= 0.0f)
		{
			if (auto input = player->GetComponent<PlayerInputComponent>())
			{
				input->EnableInput(true);
			}

			//連続テレポートを防ぐためにクールダウンを設定
			m_coolDown = 2.0f;

			//転送先のブロックもクールダウンさせる(双方向テレポート対策)
			if (auto partner = m_wpPartner.lock())
			{
				if (auto pComp = partner->GetComponent<TransferBlockComponent>())
				{
					pComp->m_coolDown = 2.0f;
				}
			}

			m_teleportState = TeleportState::Idle;
		}
		break;
	}
}

void TransferBlockComponent::StartTeleportSequence(const std::shared_ptr<GameObject>& player)
{
	m_wpPlayer = player;
	auto partner = m_wpPartner.lock();
	if (!partner)return;

	auto partnerTransform = partner->GetComponent<TransformComponent>();
	if (!partnerTransform)return;

	//ワープ先はパートナーブロックの正面(Forward)方向に少し離れた位置
	Math::Vector3 partnerPos = partnerTransform->GetPos();

	//行列のBackward()はモデルの前方を指すことが多い(Z軸正方向)
	Math::Vector3 partnerFoward = partnerTransform->GetMatrix().Backward();
	partnerFoward.Normalize();

	m_destinationPos = partnerPos + (partnerFoward * 2.0f);
	m_destinationPos.y = partnerPos.y;


	//テレポート中はplayerの操作を禁止
	if (auto input = player->GetComponent<PlayerInputComponent>())
	{
		input->EnableInput(false);
	}

	//物理挙動を停止(慣性を消す)
	if (auto rigid = player->GetComponent<RigidbodyComponent>())
	{
		rigid->SetVelocity(Math::Vector3::Zero);
	}

	//ディゾルブ演出開始
	if (auto dissolve = player->GetComponent<DissolveComponent>())
	{
		dissolve->FadeOut();
		m_teleportTimer = dissolve->GetFadeDuration();
	}
	else
	{
		m_teleportTimer = 0.5f; //ディゾルブがない場合の保険
	}
	m_teleportState = TeleportState::Dissolving;
}