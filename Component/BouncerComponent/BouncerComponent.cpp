#include "BouncerComponent.h"
#include"../TransformComponent/TransformComponent.h"
#include"../GameObject.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../IdComponent/IdComponent.h"
#include"../Src/Application/main.h"

void BouncerComponent::Awake()
{
}

void BouncerComponent::Start()
{
	m_ownerTransform = m_owner->GetComponent<TransformComponent>();
	m_currentDrawOffset = m_baseOffset;
}

void BouncerComponent::Update()
{
	if (m_isAnimating)
	{
		m_animTimer += Application::Instance().GetDeltaTime();

		//アニメーションの進捗率(0.0->1.0)
		float progress = m_animTimer / m_animationDuration;

		//終了判定
		if (progress >= 1.0f)
		{
			m_isAnimating = false;
			m_animTimer = 0.0f;
			m_currentDrawOffset = m_baseOffset;
		}
		else
		{
			//サイン波(0->1->0)を使って、バネが一度縮んでから元の位置に戻る動きを表現
			float wave = sin(progress * DirectX::XM_PI);

			//基準位置からY軸方向に下げる(沈み込む)
			m_currentDrawOffset = m_baseOffset;
			m_currentDrawOffset.y -= m_stroke * wave;
		}
	}
	else
	{
		m_currentDrawOffset = m_baseOffset;
	}
}

void BouncerComponent::DrawLit()
{
	if (!m_spModel || !m_ownerTransform) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	const Math::Matrix& ownerMat = m_ownerTransform->GetMatrix();

	//アニメーション用のオフセット行列を作成
	Math::Matrix transMat = Math::Matrix::CreateTranslation(m_currentDrawOffset);

	//オフセット行列(ローカル移動) * オーナー行列(ワールド配置) の順で合成
	Math::Matrix finalMat = transMat * ownerMat;

	if (m_isDissolving)
	{
		shader.SetDissolve(m_dissolveThreshold, &m_dissolveEdgeRange, &m_dissolveEdgeColor, &m_resolution);
	}

	shader.DrawModel(*m_spModel, finalMat);
}

void BouncerComponent::OnJump(float duration)
{
	//アニメーション開始フラグを立てる
	m_isAnimating = true;
	m_animTimer = 0.0f;
	m_animationDuration = duration + 0.1f;//少し余裕を持たせる
}

void BouncerComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("BouncerComponent")) return;
	const auto& myData = data.at("BouncerComponent");

	std::string path = JsonHelper::GetString(myData, "model");
	if (!path.empty()) SetModel(path);

	JsonHelper::GetVector3(myData, "position", m_baseOffset, m_baseOffset);

	m_stroke = JsonHelper::GetFloat(myData, "stroke", m_stroke);
}

nlohmann::json BouncerComponent::ToJson() const
{
	nlohmann::json j;
	j["model"] = m_modelPath;
	j["stroke"] = m_stroke;
	j["offset"] = { m_baseOffset.x,m_baseOffset.y,m_baseOffset.z };

	return j;
}

void BouncerComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("GearRotate Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		if (!m_modelPath.empty()) ImGui::Text("Model: %s", m_modelPath.c_str());

		ImGui::DragFloat("Stroke Amout", &m_stroke, 1.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::DragFloat3("Offset", &m_baseOffset.x, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//アニメーション確認ボタン
		if (ImGui::Button("Test Jump"))
		{
			OnJump(m_animationDuration);
		}

		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
}

void BouncerComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

void BouncerComponent::SetModel(const std::string& path)
{
	m_spModel = KdAssets::Instance().m_modeldatas.GetData(path);
	m_modelPath = path;
}