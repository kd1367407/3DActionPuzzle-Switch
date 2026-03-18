#include "ScalingBlockComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/main.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../IdComponent/IdComponent.h"
#include"../RigidbodyComponent/RigidbodyComponent.h"
#include"../ColliderComponent/ColliderComponent.h"
#include"../MagicCircleComponent/MagicCircleComponent.h"
#include"../RenderComponent/RenderComponent.h"

void ScalingBlockComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("ScalingBlockComponent"))return;

	const auto& scalingBlockData = data.at("ScalingBlockComponent");

	JsonHelper::GetVector3(scalingBlockData, "scale_axis", m_scaleAxis, m_scaleAxis);
	m_scaleAmount = JsonHelper::GetFloat(scalingBlockData, "scale_amount", m_scaleAmount);
	m_scaleSpeed = JsonHelper::GetFloat(scalingBlockData, "scale_speed", m_scaleSpeed);
	JsonHelper::GetVector3(scalingBlockData, "min_emissive", m_minEmissiveColor, m_minEmissiveColor);
	JsonHelper::GetVector3(scalingBlockData, "max_emissive", m_maxEmissiveColor, m_maxEmissiveColor);
}

void ScalingBlockComponent::Awake()
{
}

void ScalingBlockComponent::Start()
{
	m_wpTransform = m_owner->GetComponent<TransformComponent>();
	m_magicCircle = m_owner->GetComponent<MagicCircleComponent>();
	m_wpRender = m_owner->GetComponent<RenderComponent>();

	//開始時のスケールを基本スケールとして記憶
	if (auto transform = m_wpTransform.lock())
	{
		m_baseScale = transform->GetScale();
		m_lastScale = m_baseScale;
	}

	if (auto collider = m_owner->GetComponent<ColliderComponent>())
	{
		if (auto shape = collider->GetShape())
		{
			//コライダーの全高を計算(中心から上端までの距離*2)
			m_colliderFullHeight = shape->GetBoundingBox().Extents.y * 2.0f;
		}
	}
}

void ScalingBlockComponent::Update()
{
	auto transform = m_wpTransform.lock();
	if (!transform)return;

	float deltatime = Application::Instance().GetDeltaTime();

	//プレイヤー離脱タイマー更新
	if (m_playerOffTimer > 0.0f)
	{
		m_playerOffTimer -= deltatime;
	}

	if (m_playerOffTimer <= 0.0f)
	{
		m_isPlayerOnTop = false;
		m_wpPlayer.reset();
	}

	m_elapsedTime += deltatime * m_scaleSpeed;

	//sin波(-1.0 ~ 1.0)を計算
	float sinWave = sin(m_elapsedTime);

	//拡縮率を0.0 ~ 1.0の範囲に変換(演出用)
	float t = (sinWave + 1.0f) * 0.5f;

	//新しいスケール計算: 基本サイズ + (軸 * 拡縮量 * 変動値)
	Math::Vector3 newScale = m_baseScale + (m_scaleAxis * m_scaleAmount * sinWave);

	//スケールが0以下になると描画や物理演算がおかしくなるためクランプ
	newScale.x = std::max(newScale.x, 0.1f);
	newScale.y = std::max(newScale.y, 0.1f);
	newScale.z = std::max(newScale.z, 0.1f);

	//前回フレームからのYスケール変化量
	float scaleDeltaY = newScale.y - m_lastScale.y;

	//--プレイヤーとの同期処理--
	//縮むとき(scaleDeltaY < 0)にプレイヤーが乗っている場合、プレイヤーも一緒に下げる
	//これを行わないと、物理演算の更新タイミングによってはプレイヤーが一瞬宙に浮いてしまう
	if (m_isPlayerOnTop && scaleDeltaY < 0.0f)
	{
		if (auto player = m_wpPlayer.lock())
		{
			if (auto playerRigid = player->GetComponent<RigidbodyComponent>())
			{
				//スケール変化量にコライダーの高さを掛けて、実際の沈下距離を算出
				float moveAmount = scaleDeltaY * m_colliderFullHeight;
				playerRigid->m_additionalMovement.y += moveAmount;
			}
		}
	}

	transform->SetScale(newScale);

	m_lastScale = newScale;

	//現在のサイズに応じた発光色を計算
	Math::Vector3 currentEmissive = Math::Vector3::Lerp(m_minEmissiveColor, m_maxEmissiveColor, t);
	currentEmissive = Math::Vector3::Max(Math::Vector3(0.5f), currentEmissive);

	//魔法陣の演出同期(大きくなるほど速く回るなど)
	if (m_magicCircle)
	{
		m_magicCircle->SetEmissiveColor(currentEmissive);

		float rotSpeedMultiplier = 1.0f + (t * 2.0f);//最大3倍速

		m_magicCircle->SetRotationSpeedMultiplier(rotSpeedMultiplier);
	}
}

void ScalingBlockComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Scaling Block Compoent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		ImGui::DragFloat3("Scale Axis", &m_scaleAxis.x, 0.01f, 0.0f, 1.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Scale Amount", &m_scaleAmount, 0.05f, 0.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Scale Speed", &m_scaleSpeed, 0.05f, 0.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::Separator();
		ImGui::Text("Visual Effects");
		// 色編集UI
		itemDeactivated |= ImGui::ColorEdit3("Min Emissive", &m_minEmissiveColor.x);
		itemDeactivated |= ImGui::ColorEdit3("Max Emissive", &m_maxEmissiveColor.x);

		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
}

void ScalingBlockComponent::OnCollision(const CollisionInfo& info)
{
	//プレイヤーが上から接触した場合のみ追従フラグを立てる
	if (info.otherObject && info.otherObject->GetName() == "Player" && info.contactNormal.y > 0.7f)
	{
		m_isPlayerOnTop = true;
		m_playerOffTimer = 0.1f;
		m_wpPlayer = info.otherObject;
	}
}

void ScalingBlockComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}