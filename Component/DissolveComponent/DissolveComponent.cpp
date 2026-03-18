#include "DissolveComponent.h"
#include"../Src/Application/main.h"
#include"../GameObject.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../RenderComponent/RenderComponent.h"
#include"../IdComponent/IdComponent.h"
#include"../Src/Application/GameViewModel.h"
#include"../GearRotateComponent/GearRotateComponent.h"
#include"../GlowPartComponent/GlowPartComponent.h"
#include"../BouncerComponent/BouncerComponent.h"

void DissolveComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("DissolveComponent"))return;
	const auto& dData = data.at("DissolveComponent");

	m_fadeDuration = JsonHelper::GetFloat(dData, "fadeDuration", m_fadeDuration);
	JsonHelper::GetVector3(dData, "edgeColor", m_edgeColor, m_edgeColor);
	m_edgeRange = JsonHelper::GetFloat(dData, "edgeRange", m_edgeRange);
	m_blockResolution = JsonHelper::GetFloat(dData, "blockResolution", m_blockResolution);
}

void DissolveComponent::Awake()
{
	m_state = State::Idle;
	m_currentThreshold = 0.0f;
}

void DissolveComponent::Start()
{
	m_wpRenderComp = m_owner->GetComponent<RenderComponent>();
	m_wpGlowComp = m_owner->GetComponent<GlowPartComponent>();
	m_wpGearComp = m_owner->GetComponent<GearRotateComponent>();
	m_wpBouncer = m_owner->GetComponent<BouncerComponent>();

	//描画が始まる前に初期値を入れておく
	if (auto renderComp = m_wpRenderComp.lock())
	{
		renderComp->SetDissolve(0.0f, m_edgeRange, m_edgeColor, m_blockResolution);
	}
	if (auto gearComp = m_wpGearComp.lock())
	{
		gearComp->SetDissolve(0.0f, m_edgeRange, m_edgeColor, m_blockResolution);
	}
	if (auto glowComp = m_wpGlowComp.lock())
	{
		glowComp->SetDissolve(0.0f, m_edgeRange, m_edgeColor, m_blockResolution);
	}
	if (auto bouncerComp = m_wpBouncer.lock())
	{
		bouncerComp->SetDissolve(0.0f, m_edgeRange, m_edgeColor, m_blockResolution);
	}
}

void DissolveComponent::Update()
{
	if (m_state == State::Idle)return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	float	deltatime = Application::Instance().GetDeltaTime();
	m_timer += deltatime;

	//消滅中
	if (m_state == State::FadingOut)
	{
		//経過時間から0.0～1.0のしきい値を計算
		m_currentThreshold = m_timer / m_fadeDuration;

		//消滅完了
		if (m_currentThreshold >= 1.0f)
		{
			m_currentThreshold = 1.0f;
			m_state = State::Idle;

			if (OnFadeOutComplete)
			{
				OnFadeOutComplete();
			}
		}
	}
	//出現中
	else if (m_state == State::FadingIn)
	{
		//出現時は1.0から0.0へ向かう
		m_currentThreshold = 1.0f - (m_timer / m_fadeDuration);

		//出現完了
		if (m_currentThreshold <= 0.0f)
		{
			m_currentThreshold = 0.0f;
			m_state = State::Idle;

			if (OnFadeInComplete)
			{
				OnFadeInComplete();
			}
		}
	}

	//各描画コンポーネントに現在のディゾルブ値を適用
	if (auto renderComp = m_wpRenderComp.lock())
	{
		renderComp->SetDissolve(m_currentThreshold, m_edgeRange, m_edgeColor, m_blockResolution);
	}

	if (auto glow = m_wpGlowComp.lock())
	{
		glow->SetDissolve(m_currentThreshold, m_edgeRange, m_edgeColor, m_blockResolution);
	}

	if (auto gear = m_wpGearComp.lock())
	{
		gear->SetDissolve(m_currentThreshold, m_edgeRange, m_edgeColor, m_blockResolution);
	}

	if (auto bouncer = m_wpBouncer.lock())
	{
		bouncer->SetDissolve(m_currentThreshold, m_edgeRange, m_edgeColor, m_blockResolution);
	}
}

nlohmann::json DissolveComponent::ToJson() const
{
	nlohmann::json j;
	j["fadeDuration"] = m_fadeDuration;
	j["edgeColor"] = { m_edgeColor.x, m_edgeColor.y, m_edgeColor.z };
	j["edgeRange"] = m_edgeRange;
	j["blockResolution"] = m_blockResolution;
	return j;
}

void DissolveComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Dissolve Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		itemDeactivated |= ImGui::DragFloat("Fade Duration", &m_fadeDuration, 0.01f, 0.1f, 5.0f);
		itemDeactivated |= ImGui::DragFloat("Edge Range", &m_edgeRange, 0.01f, 0.0f, 1.0f);
		itemDeactivated |= ImGui::ColorEdit3("Edge Color", &m_edgeColor.x);
		itemDeactivated |= ImGui::DragFloat("Block Resolution", &m_blockResolution, 0.5f, 0.0f, 256.0f);

		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
}

void DissolveComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

void DissolveComponent::FadeOut()
{
	if (m_state != State::Idle)return;
	m_state = State::FadingOut;
	m_timer = 0.0f;
}

void DissolveComponent::FadeIn()
{
	if (m_state != State::Idle) return;
	m_state = State::FadingIn;
	m_timer = 0.0f;
}