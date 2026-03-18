#include "JumpBlockComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../RigidbodyComponent/RigidbodyComponent.h"
#include"../GravityComponent/GravityComponent.h"
#include"../Src/Application/main.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../../Editor/EditorGizmoContext.h"
#include"../Src/Application/Scene/GameScene/GameScene.h"
#include"../IdComponent/IdComponent.h"
#include"../Src/Application/Effect/Particle/ParticleSystem.h"
#include"../InputComponent/PlayerInputComponent.h"
#include"../GlowPartComponent/GlowPartComponent.h"
#include"../BouncerComponent/BouncerComponent.h"

void JumpBlockComponent::Awake()
{
	m_currentState = State::Idle;
}

void JumpBlockComponent::Start()
{
	m_transform = m_owner->GetComponent<TransformComponent>();

	if (auto glow = m_owner->GetComponent<GlowPartComponent>())
	{
		Math::Vector3 dir = m_jumpDirection;
		if (dir.LengthSquared() > 0.001f) //ゼロ除算対策
		{
			glow->SetFloatAnimation(dir, 5.0f, 0.3f);
		}
		else
		{
			//方向がゼロに近い場合はデフォルトの上方向へ
			glow->SetFloatAnimation(Math::Vector3::Up, 5.0f, 0.3f);
		}
	}
}

void JumpBlockComponent::Update()
{
	if (m_currentState == State::Idle)return;

	float deltatime = Application::Instance().GetDeltaTime();
	m_chargeTimer += deltatime;

	//チャージ中はプレイヤーを停止させる
	if (auto rigid = m_wpPlayerRigidBody.lock())
	{
		rigid->SetVelocity(Math::Vector3::Zero);
	}
	else
	{
		m_currentState = State::Idle;
		return;
	}

	//チャージ完了(発射)
	if (m_chargeTimer >= m_chargeDuration)
	{
		//発射パーティクル生成
		if (m_pGameScene)
		{
			ParticleSystem* pJSys = m_pGameScene->GetParticleSystem("Jump");
			ParticleSystem* pSSys = m_pGameScene->GetParticleSystem("Sockwave");

			if (pJSys)
			{
				Math::Vector3 emitPos = m_transform->GetPos() + Math::Vector3(0.0f, 0.5f, 0.0f);
				float spread = 0.3f;

				pJSys->Emit(emitPos, 30, m_jumpDirection, spread);
			}

			if (pSSys)
			{
				Math::Vector3 emitPos = m_transform->GetPos() + Math::Vector3(0.0f, 2.0f, 0.0f);

				pSSys->Emit(emitPos, 50, Math::Vector3::Up, 0.0f);
			}
		}

		//発射音
		//KdAudioManager::Instance().Play("Asset/Sound/JumpBlockShot.wav", false);

		if (auto rigid = m_wpPlayerRigidBody.lock())
		{
			//プレイヤーの操作を有効化
			if (auto inputComp = rigid->GetOwner()->GetComponent<PlayerInputComponent>())
			{
				inputComp->EnableInput(true);
			}

			rigid->SetVelocity(Math::Vector3::Zero); //既存の速度をリセットして発射ベクトルのみを適用
			rigid->AddForce(m_jumpDirection * m_jumpForce);
		}

		m_currentState = State::Idle;
		m_wpPlayerRigidBody.reset();
	}
}

void JumpBlockComponent::OnCollision(const CollisionInfo& info)
{
	if (!m_transform || !info.otherObject)return;

	if (m_currentState == State::Charging)return;

	//上方向からの接触のみ判定
	if (info.contactNormal.y > 0.7f)
	{
		if (auto rigid = info.otherObject->GetComponent<RigidbodyComponent>())
		{
			//動的オブジェクト(主にプレイヤー)の場合のみ反応
			if (rigid->m_type == RigidbodyType::Dynamic)
			{
				if (auto inputComp = info.otherObject->GetComponent<PlayerInputComponent>())
				{
					//発射まで操作不能にする
					inputComp->EnableInput(false);

					//速度をゼロにしてブロック上に固定
					rigid->SetVelocity(Math::Vector3::Zero);

					m_currentState = State::Charging;
					m_chargeTimer = 0.0f;

					m_wpPlayerRigidBody = rigid;

					//バウンスアニメーション再生
					if (auto bouncer = m_owner->GetComponent<BouncerComponent>())
					{
						bouncer->OnJump(m_chargeDuration);
					}
				}

			}
		}
	}
}

void JumpBlockComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Jump Block Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		ImGui::DragFloat3("Jump Direction", &m_jumpDirection.x, 0.01f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Jump Force", &m_jumpForce, 1.0f, 0.0f, 1000.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Charge Duration", &m_chargeDuration, 0.01f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		// いずれかのUIのドラッグが終了した瞬間
		if (itemDeactivated)
		{
			RequestStateChangeCommand();
		}
	}
}

void JumpBlockComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("JumpBlockComponent"))return;

	const auto& JumpBLockData = data.at("JumpBlockComponent");

	if (JumpBLockData.contains("jumpDirection"))
	{
		JsonHelper::GetVector3(JumpBLockData, "jumpDirection", m_jumpDirection, { 0.0f,4.0f,0.0f });
	}

	if (JumpBLockData.contains("jumpForce"))
	{
		m_jumpForce = JsonHelper::GetFloat(JumpBLockData, "jumpForce", 100.0f);
	}

	if (JumpBLockData.contains("jumpDuration"))
	{
		m_chargeDuration = JsonHelper::GetFloat(JumpBLockData, "jumpDuration", 0.5f);
	}
}

nlohmann::json JumpBlockComponent::ToJson() const
{
	nlohmann::json j;

	j["jumpDirection"] = { m_jumpDirection.x,m_jumpDirection.y,m_jumpDirection.z };
	j["jumpForce"] = m_jumpForce;
	j["jumpDuration"] = m_chargeDuration;

	return j;
}

bool JumpBlockComponent::OnDrawGizmos(const EditorGizmoContext& context, GameScene& scene)
{
	bool isTransformGizmoUse = false;
	if (auto transform = m_owner->GetComponent<TransformComponent>())
	{
		isTransformGizmoUse = transform->OnDrawGizmos(context, scene);
	}

	bool isTargetGizmoUse = false;

	ImGuizmo::PushID("targetGizmo");

	//ターゲット位置をワールド座標で取得してマトリックス作成
	Math::Matrix targetMat = Math::Matrix::CreateTranslation(GetTargetPos());

	bool isObjectDragActive = false;

	//ギズモ描画と操作受付
	ImGuizmo::Manipulate(
		(float*)context.viewMat, (float*)context.projMat,
		ImGuizmo::TRANSLATE, ImGuizmo::WORLD, (float*)&targetMat
	);

	if (ImGuizmo::IsUsing())
	{
		if (!m_isTargetGizmoDragging)
		{
			isTargetGizmoUse = true;
			m_isTargetGizmoDragging = true;
		}
		//操作された座標をローカル座標(ジャンプ方向)に戻して保存
		auto newPos = targetMat.Translation();
		SetTargetPos(newPos);
	}
	else if (m_isTargetGizmoDragging)
	{
		RequestStateChangeCommand();
		m_isTargetGizmoDragging = false;
	}

	ImGuizmo::PopID();

	//本体からターゲット位置への線を引く
	if (auto debugWire = scene.GetDebugWire())
	{
		debugWire->AddDebugLine(GetOwnerPos(), GetTargetPos(), kGreenColor);
	}

	return isTransformGizmoUse || isTargetGizmoUse;
}

void JumpBlockComponent::RequestStateChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}

		if (auto glow = m_owner->GetComponent<GlowPartComponent>())
		{
			glow->SetFloatAnimation(m_jumpDirection, 5.0f, 0.3f);
		}
	}
}

void JumpBlockComponent::SetViewModel(const std::shared_ptr<GameViewModel>& viewModel)
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

void JumpBlockComponent::SetTargetPos(const Math::Vector3& pos)
{
	if (m_transform)
	{
		//ワールド座標をローカル座標に変換して保存(ジャンプ方向として使用)
		Math::Matrix invOwnerMat = m_transform->GetMatrix().Invert();
		m_jumpDirection = Math::Vector3::Transform(pos, invOwnerMat);
	}
}

const Math::Vector3 JumpBlockComponent::GetTargetPos()const
{
	//ローカル座標(ジャンプ方向)をワールド座標に変換して返す
	if (m_transform)
	{
		return Math::Vector3::Transform(m_jumpDirection, m_transform->GetMatrix());
	}
	return Math::Vector3::Zero;
}

const Math::Vector3& JumpBlockComponent::GetOwnerPos() const
{
	if (m_transform) return m_transform->GetPos();
	return Math::Vector3::Zero;
}