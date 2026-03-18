#include "SinkingBlockComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../IdComponent/IdComponent.h"
#include"../../ImGuizmo/ImGuizmo.h"
#include"../../Editor/EditorGizmoContext.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../GlowPartComponent/GlowPartComponent.h"
#include"../ParticleEmitterComponent/ParticleEmitterComponent.h"

void SinkingBlockComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("SinkingBlockComponent"))return;

	const auto& SinkBlockData = data.at("SinkingBlockComponent");

	JsonHelper::GetVector3(SinkBlockData, "initial_pos", m_initialPos, m_initialPos);
	m_maxSinkDistance = JsonHelper::GetFloat(SinkBlockData, "max_sink_distance", m_maxSinkDistance);
	m_acceleration = JsonHelper::GetFloat(SinkBlockData, "acceleration", m_acceleration);
	m_riseSpeed = JsonHelper::GetFloat(SinkBlockData, "rise_speed", m_riseSpeed);
}

void SinkingBlockComponent::Awake()
{
	m_currentState = State::Idle;
}

void SinkingBlockComponent::Start()
{
	m_wpTransform = m_owner->GetComponent<TransformComponent>();
	m_wpGlow = m_owner->GetComponent<GlowPartComponent>();
	m_wpEmitter = m_owner->GetComponent<ParticleEmitterComponent>();

	//Configureで読み込めていない場合のために現在の配置位置を初期位置として保存
	if (auto transform = m_wpTransform.lock())
	{
		m_initialPos = transform->GetPos();
	}

	if (auto glow = m_wpGlow.lock())
	{
		m_defaultBlinkSpeed = glow->GetBlinkSpeed();

		if (m_defaultBlinkSpeed < 0.1f)m_defaultBlinkSpeed = 2.0f;
	}
}

void SinkingBlockComponent::Update()
{
	auto transform = m_wpTransform.lock();
	if (!transform)return;
	float deltatime = Application::Instance().GetDeltaTime();

	//エディタモードなどで位置がずれていたら初期位置に強制リセット
	if (SceneManager::Instance().GetCurrentMode() != SceneManager::SceneMode::Game)
	{
		if (transform->GetPos() != m_initialPos)
		{
			transform->SetPos(m_initialPos);
		}
	}

	//プレイヤー離脱検知タイマーの更新
	if (m_playerOffTimer > 0.0f)
	{
		m_playerOffTimer -= deltatime;
	}

	if (m_playerOffTimer <= 0.0f)
	{
		m_isPlayerOnTop = false;
	}

	Math::Vector3 currentPos = transform->GetPos();
	//振動オフセットを除去して計算用座標にする
	currentPos -= m_shakeOffset;
	State nextState = m_currentState;

	//--playerが乗っている時(沈む処理)--
	if (m_isPlayerOnTop)
	{
		Math::Vector3 sunkPos = m_initialPos;
		sunkPos.y -= m_maxSinkDistance;

		//まだ底についていないなら沈む
		if (currentPos.y > sunkPos.y)
		{
			nextState = State::Sinking;
			//加速度的に落下速度を上げる
			m_currentSpeed += m_acceleration * deltatime;
			currentPos.y -= m_currentSpeed * deltatime;

			//行き過ぎ補正
			if (currentPos.y < sunkPos.y)
			{
				currentPos.y = sunkPos.y;
				nextState = State::Idle;
			}
			transform->SetPos(currentPos);
		}
	}

	//--playerが乗っていない時(上昇・復帰処理)--
	else
	{
		m_currentSpeed = 0;
		//初期位置より下がっているなら上昇
		if (currentPos.y < m_initialPos.y)
		{
			nextState = State::Rising;
			currentPos.y += m_riseSpeed * deltatime;

			//行き過ぎ補正
			if (currentPos.y > m_initialPos.y)
			{
				currentPos.y = m_initialPos.y;
				nextState = State::Idle;
			}
		}
	}

	m_currentState = nextState;

	//--演出処理--
	bool isDanger = (m_currentState == State::Sinking);

	//沈んでいる間は激しく点滅させる
	if (auto glow = m_wpGlow.lock())
	{
		if (isDanger)
		{
			glow->SetBlinkSpeed(m_defaultBlinkSpeed * 5.0f);
			glow->SetEnableBlink(true);
		}
		else
		{
			glow->SetBlinkSpeed(m_defaultBlinkSpeed);
			glow->SetEnableBlink(true);
		}
	}

	//沈んでいる間は粉塵エフェクトを出す
	if (auto emitter = m_wpEmitter.lock())
	{
		emitter->SetActive(m_currentState == State::Sinking);
	}

	//沈んでいる間は振動させる
	m_shakeOffset = Math::Vector3::Zero;

	if (isDanger)
	{
		m_shakeOffset.x = KdRandom::GetFloat(-m_shakeMagnitude, m_shakeMagnitude);
		m_shakeOffset.y = KdRandom::GetFloat(-m_shakeMagnitude, m_shakeMagnitude);
		m_shakeOffset.z = KdRandom::GetFloat(-m_shakeMagnitude, m_shakeMagnitude);
	}

	//最終的な座標(基本位置 + 振動)をセット
	transform->SetPos(currentPos + m_shakeOffset);
}

void SinkingBlockComponent::OnCollision(const CollisionInfo& info)
{
	//プレイヤーが上から接触した時のみフラグを立てる
	if (info.otherObject && info.otherObject->GetName() == "Player" && info.contactNormal.y > 0.7f)
	{
		m_isPlayerOnTop = true;
		m_playerOffTimer = 0.1f;
	}
}

void SinkingBlockComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Sinking Block Compoent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		//--初期座標--
		ImGui::DragFloat3("Initial Pos", &m_initialPos.x, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--最大下降距離--
		ImGui::DragFloat("Max Sink Distance", &m_maxSinkDistance, 0.1f, 0.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--加速度--
		ImGui::DragFloat("Acceleration", &m_acceleration, 0.1f, 0.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--上昇スピード--
		ImGui::DragFloat("Rise Speed", &m_riseSpeed, 0.1f, 0.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//いずれかのウィジェットウィジェットのドラッグが終了した瞬間
		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
}

nlohmann::json SinkingBlockComponent::ToJson() const
{
	nlohmann::json j;
	j["initial_pos"] = { m_initialPos.x,m_initialPos.y,m_initialPos.z };
	j["max_sink_distance"] = m_maxSinkDistance;
	j["acceleration"] = m_acceleration;
	j["rise_speed"] = m_riseSpeed;

	return j;
}

bool SinkingBlockComponent::OnDrawGizmos(const EditorGizmoContext& context, GameScene& scene)
{
	bool isUsingInitial = false;

	ImGuizmo::PushID("initialPosGizmo");

	Math::Matrix initialMat = Math::Matrix::CreateTranslation(m_initialPos);

	ImGuizmo::Manipulate(
		(float*)context.viewMat, (float*)context.projMat,
		ImGuizmo::TRANSLATE, ImGuizmo::WORLD, (float*)&initialMat
	);

	if (ImGuizmo::IsUsing())
	{
		if (!m_isInitialGizmoDragging)
		{
			isUsingInitial = true;
			m_isInitialGizmoDragging = true;
		}
		auto newPos = initialMat.Translation();
		SetInitialPos(newPos);
	}
	else if (m_isInitialGizmoDragging)
	{
		RequestTransformChangeCommand();
		m_isInitialGizmoDragging = false;
	}

	ImGuizmo::PopID();

	return isUsingInitial;
}

void SinkingBlockComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}