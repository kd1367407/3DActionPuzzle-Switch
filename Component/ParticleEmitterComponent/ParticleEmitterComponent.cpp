#include "ParticleEmitterComponent.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/Scene/GameScene/GameScene.h"
#include"../Src/Application/main.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../Src/Application/Effect/Particle/ParticleSystem.h"
#include"../Src/Application/GameViewModel.h"
#include"../IdComponent/IdComponent.h"
#include"../Src/Framework/JsonConversion/JsonConversion.h"

void ParticleEmitterComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("ParticleEmitterComponent")) return;

	const auto& emitterData = data.at("ParticleEmitterComponent");

	m_systemName = JsonHelper::GetString(emitterData, "system_name", m_systemName);
	m_emitCount = JsonHelper::GetInt(emitterData, "emit_count", m_emitCount);
	m_emitFrequency = JsonHelper::GetFloat(emitterData, "emit_frequency", m_emitFrequency);
	m_spread = JsonHelper::GetFloat(emitterData, "spread", m_spread);
	JsonHelper::GetVector3(emitterData, "base_direction", m_baseDir, m_baseDir);

	m_offsets.clear();
	//複数オフセットの読み込み
	if (emitterData.contains("offsets") && emitterData.at("offsets").is_array())
	{
		for (const auto& offsetJson : emitterData.at("offsets"))
		{
			m_offsets.push_back(offsetJson.get<Math::Vector3>());
		}
	}
	//単一オフセット(互換性維持)
	else if (emitterData.contains("offset"))
	{
		Math::Vector3 singleOffset;
		JsonHelper::GetVector3(emitterData, "offset", singleOffset, Math::Vector3::Zero);
		m_offsets.push_back(singleOffset);
	}

	//最低でも1つは(0,0,0)を入れておく
	if (m_offsets.empty())
	{
		m_offsets.push_back(Math::Vector3::Zero);
	}

	m_timer = 0.0f;
}

void ParticleEmitterComponent::Start()
{
	m_transform = m_owner->GetComponent<TransformComponent>();

	if (!m_pGameScene)
	{
		if (auto baseScene = SceneManager::Instance().GetCurrentScene())
		{
			//GameSceneであればキャストしてポインタを保持(パーティクルシステムへのアクセス用)
			m_pGameScene = dynamic_cast<GameScene*>(baseScene);
		}
	}
}

void ParticleEmitterComponent::Update()
{
	if (!m_transform || !m_pGameScene) return;

	if (!m_isActive) return;

	//タイマー更新
	m_timer -= Application::Instance().GetDeltaTime();

	if (m_timer <= 0.0f)
	{
		m_timer = m_emitFrequency;

		//パーティクルシステム取得
		ParticleSystem* pSys = m_pGameScene->GetParticleSystem(m_systemName);
		if (!pSys)return;

		//オーナーの行列を取得
		const Math::Matrix& worldMat = m_transform->GetMatrix();

		//放出方向をローカルからワールドへ変換
		Math::Vector3 worldDir = Math::Vector3::TransformNormal(m_baseDir, worldMat);
		worldDir.Normalize();

		//登録されている全てのオフセット位置からパーティクル放出
		for (const auto& localOffset : m_offsets)
		{
			//オフセット位置をワールド座標へ変換
			Math::Vector3 emitPos = Math::Vector3::Transform(localOffset, worldMat);

			pSys->Emit(emitPos, m_emitCount, worldDir, m_spread);
		}
	}
}

nlohmann::json ParticleEmitterComponent::ToJson() const
{
	nlohmann::json j;
	j["system_name"] = m_systemName;
	j["emit_count"] = m_emitCount;
	j["emit_frequency"] = m_emitFrequency;
	j["base_direction"] = m_baseDir;
	j["spread"] = m_spread;
	j["offsets"] = m_offsets;
	return j;
}

void ParticleEmitterComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Particle Emitter Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		ImGui::InputInt("Emit Count", &m_emitCount);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Emit Frequency (sec)", &m_emitFrequency, 0.01f, 0.0f, 5.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat("Spread", &m_spread, 0.01f, 0.0f, 2.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat3("Base Direction", &m_baseDir.x, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::SeparatorText("Offsets");
		int offsetIndexToRemove = -1;

		//オフセット配列の編集UI
		for (int i = 0; i < m_offsets.size(); ++i)
		{
			ImGui::PushID(i);

			if (ImGui::DragFloat3("##Offset", &m_offsets[i].x, 0.1f))
			{
				//値は直接 m_offsets[i] に書き込まれる
			}
			itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

			ImGui::SameLine();
			//削除ボタン
			if (ImGui::Button("X"))
			{
				offsetIndexToRemove = i;
			}

			ImGui::PopID();
		}

		//削除処理
		if (offsetIndexToRemove != -1)
		{
			m_offsets.erase(m_offsets.begin() + offsetIndexToRemove);
			itemDeactivated = true;
			offsetIndexToRemove = -1;
		}

		//追加ボタン
		if (ImGui::Button("Add Offset"))
		{
			m_offsets.push_back(Math::Vector3::Zero);
			itemDeactivated = true;
		}

		if (itemDeactivated)
		{
			RequestParamChangeCommand();
		}
	}
}

void ParticleEmitterComponent::RequestParamChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

void ParticleEmitterComponent::SetViewModel(std::shared_ptr<GameViewModel> vm)
{
	m_wpViewModel = vm;
	if (auto spViewModel = m_wpViewModel.lock())
	{
		m_pGameScene = dynamic_cast<GameScene*>(spViewModel->GetScene());
	}
	else
	{
		m_pGameScene = nullptr;
	}
}