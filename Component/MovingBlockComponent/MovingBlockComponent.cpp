#include "MovingBlockComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../RigidbodyComponent/RigidbodyComponent.h"
#include"../IdComponent/IdComponent.h"
#include"../Src/Application/main.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"
#include"../Src/Application/GameViewModel.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../../ImGuizmo/ImGuizmo.h"
#include"../../Editor/EditorGizmoContext.h"
#include"../Src/Application/Scene/GameScene/GameScene.h"
#include"../Src/Application/Effect/Particle/ParticleSystem.h"
#include"../ParticleEmitterComponent/ParticleEmitterComponent.h"

void MovingBlockComponent::Awake()
{
}

void MovingBlockComponent::Start()
{
	m_transform = m_owner->GetComponent<TransformComponent>();
	m_wpEmitter = m_owner->GetComponent<ParticleEmitterComponent>();
}

void MovingBlockComponent::Update()
{
	if (!m_isActive)return;
	if (!m_transform)return;

	//進捗度(0.0～1.0)を更新
	float speed = 1.0f / m_duration;
	float deltaTime = Application::Instance().GetDeltaTime();
	if (m_isRevarse)
	{
		m_progress -= speed * deltaTime;
	}
	else
	{
		m_progress += speed * deltaTime;
	}

	//往復運動(PingPong)のための折り返し処理
	if (m_progress >= 1.0f)
	{
		m_progress = 1.0f;
		m_isRevarse = true;
	}
	else if (m_progress <= 0.0f)
	{
		m_progress = 0.0f;
		m_isRevarse = false;
	}

	//イージング関数を通して滑らかな動きにする
	float easedProgress = EaseInOutSine(m_progress);

	//開始点と終了点を線形補間(Lerp)して現在位置を決定
	Math::Vector3 prevPos = m_transform->GetPos();
	Math::Vector3 newPos = Math::Vector3::Lerp(m_startPos, m_endPos, easedProgress);
	m_transform->SetPos(newPos);

	//移動量に応じたパーティクル放出制御
	if (auto emitter = m_wpEmitter.lock())
	{
		Math::Vector3 velocity = newPos - prevPos;

		if (velocity.LengthSquared() > 0.0001f)
		{
			emitter->SetActive(true);

			//進行方向の逆に向かってパーティクルを放出(軌跡のように見せる)
			Math::Vector3 trailDir = -velocity;
			emitter->SetBaseDirection(trailDir);
		}
		else
		{
			//停止中は放出しない
			emitter->SetActive(false);
		}
	}
}

void MovingBlockComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("MovingBlockComponent"))return;

	const auto& MovingBlockData = data.at("MovingBlockComponent");

	m_isActive = JsonHelper::GetBool(MovingBlockData, "active", false);
	m_duration = JsonHelper::GetFloat(MovingBlockData, "duration", 2.0f);

	if (MovingBlockData.contains("startPos"))
	{
		JsonHelper::GetVector3(MovingBlockData, "startPos", m_startPos, { 0,2.0,0 });
	}

	if (MovingBlockData.contains("endPos"))
	{
		const auto& endPosValue = MovingBlockData["endPos"];
		//値が文字列かチェック
		if (endPosValue.is_string())
		{
			std::string endPosStr = endPosValue.get<std::string>();

			//相対座標指定: "startPos(x,y,z)" 形式の場合、startPosからのオフセットとして計算
			if (endPosStr.rfind("startPos(", 0) == 0)
			{
				Math::Vector3 offset;
				sscanf_s(endPosStr.c_str(), "startPos(%f,%f,%f)", &offset.x, &offset.y, &offset.z);

				m_endPos = m_startPos + offset;
			}
		}
		else if (endPosValue.is_array())//値が配列の場合は絶対座標として読み込む
		{
			JsonHelper::GetVector3(MovingBlockData, "endPos", m_endPos, m_endPos);
		}
	}
}

nlohmann::json MovingBlockComponent::ToJson() const
{
	nlohmann::json j;

	j["active"] = m_isActive;
	j["duration"] = m_duration;
	j["startPos"] = { m_startPos.x,m_startPos.y,m_startPos.z };
	j["endPos"] = { m_endPos.x,m_endPos.y,m_endPos.z };

	return j;
}

void MovingBlockComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Moving Block Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;
		ImGui::DragFloat3("Start Position", &m_startPos.x);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat3("End Position", &m_endPos.x);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Duration (seconds)", &m_duration, 0.1f, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		if (itemDeactivated)
		{
			RequestStateChangeCommand();
		}
	}
}

bool MovingBlockComponent::OnDrawGizmos(const EditorGizmoContext& context, GameScene& scene)
{
	bool isUsingStart = false;
	bool isUsingEnd = false;

	//--startPos用ギズモ--
	ImGuizmo::PushID("startPosGizmo");

	Math::Matrix startMat = Math::Matrix::CreateTranslation(m_startPos);

	ImGuizmo::Manipulate(
		(float*)context.viewMat, (float*)context.projMat,
		ImGuizmo::TRANSLATE, ImGuizmo::WORLD, (float*)&startMat
	);

	if (ImGuizmo::IsUsing())
	{
		if (!m_isStartGizmoDragging)
		{
			isUsingStart = true;
			m_isStartGizmoDragging = true;
		}
		auto newPos = startMat.Translation();
		SetStartPos(newPos);
	}
	else if (m_isStartGizmoDragging)
	{
		RequestStateChangeCommand();
		m_isStartGizmoDragging = false;
	}

	ImGuizmo::PopID();

	//--endPos用ギズモ--
	ImGuizmo::PushID("endPosGizmo");

	Math::Matrix endPosMat = Math::Matrix::CreateTranslation(m_endPos);

	ImGuizmo::Manipulate(
		(float*)context.viewMat, (float*)context.projMat,
		ImGuizmo::TRANSLATE, ImGuizmo::WORLD, (float*)&endPosMat
	);

	if (ImGuizmo::IsUsing())
	{
		if (!m_isEndGizmoDragging)
		{
			isUsingEnd = true;
			m_isEndGizmoDragging = true;
		}
		auto newPos = endPosMat.Translation();
		SetEndPos(newPos);
	}
	else if (m_isEndGizmoDragging)
	{
		RequestStateChangeCommand();
		m_isEndGizmoDragging = false;
	}

	ImGuizmo::PopID();

	//始点と終点を結ぶデバッグラインを描画
	if (auto debugWire = scene.GetDebugWire())
	{
		debugWire->AddDebugLine(m_startPos, m_endPos, kGreenColor);
	}

	return isUsingStart || isUsingEnd;
}

void MovingBlockComponent::SetViewModel(const std::shared_ptr<GameViewModel>& viewModel)
{
	m_wpViewModel = viewModel;
}

void MovingBlockComponent::RequestStateChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}