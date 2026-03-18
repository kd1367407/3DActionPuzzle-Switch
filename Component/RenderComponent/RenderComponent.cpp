#include "RenderComponent.h"
#include"../TransformComponent/TransformComponent.h"
//#include"../IdComponent/IdComponent.h"
#include"../GameObject.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"

void RenderComponent::Awake()
{
}

void RenderComponent::Start()
{
	m_transform = m_owner->GetComponent<TransformComponent>();

	if (!m_spTargetModel)
	{
		SetTargetModel("Asset/Models/UI/TargetCursor.gltf");
	}
}

void RenderComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("RenderComponent"))return;

	const auto& renderData = data.at("RenderComponent");

	if (renderData.contains("model"))
	{
		std::string modelPath = JsonHelper::GetString(renderData, "model");
		if (!modelPath.empty())
		{
			SetModel(modelPath);
		}
	}
}

nlohmann::json RenderComponent::ToJson() const
{
	nlohmann::json j;
	j["model"] = m_modelPath;

	return j;
}

void RenderComponent::DrawLit()
{
	if (!m_enable) return;
	if ((m_spModel || m_spModelWork) && m_transform)
	{
		if (m_owner && m_owner->GetName()._Starts_with("Particle_"))
		{
			return;
		}

		//ディゾルブ処理が有効ならシェーダーパラメータを設定
		if (m_isDissolving)
		{
			KdShaderManager::Instance().m_StandardShader.SetDissolve(m_dissolveThreshold, &m_dissolveEdgeRange, &m_dissolveEdgeColor, &m_resolution);
		}

		Math::Vector3 finalEmissive = m_useCustomEmissive ? m_emissiveColor : Math::Vector3::Zero;

		if (m_spModelWork)
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModelWork, m_transform->GetMatrix(), kWhiteColor, finalEmissive);
		}
		else if (m_spModel)
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_transform->GetMatrix(), kWhiteColor, finalEmissive);
		}

		//選択時またはホバー時にターゲットカーソルを描画
		if (m_highlightState == HighlightState::Hoverd || m_highlightState == HighlightState::Selected)
		{
			if (m_spTargetModel)
			{
				//カーソルはカリングなしで描画して見やすくする
				KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);
				KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spTargetModel, m_transform->GetMatrix());
				KdShaderManager::Instance().UndoRasterizerState();
			}
		}
	}
}

void RenderComponent::DrawBright()
{
	if (!m_enable) return;
	//何かしらのハイライトが設定されている場合のみ描画(発光パス)
	bool shouldDraw = (m_highlightState != HighlightState::None) || (m_useCustomEmissive && (m_emissiveColor.LengthSquared() > 0.01f));

	if (shouldDraw && (m_spModel || m_spModelWork) && m_transform)
	{
		auto& shader = KdShaderManager::Instance().m_StandardShader;

		//現在のシェーダーのマテリアル情報を取得して保存し、一時的に書き換える
		KdStandardShader::cbMaterial originalMaterial = shader.GetMaterialCB();
		KdStandardShader::cbMaterial customMaterial = originalMaterial;

		if (m_isDissolving)
		{
			shader.SetDissolve(m_dissolveThreshold, &m_dissolveEdgeRange, &m_dissolveEdgeColor, &m_resolution);
		}

		if (m_highlightState == HighlightState::Selected)
		{
			//選択時は黄色く発光させる
			customMaterial.Emissive = { 1.5f, 1.0f, 0.0f };
			shader.SetMaterialCB(customMaterial);

			//ブロック本体を描画
			if (m_spModelWork)
			{
				shader.DrawModel(*m_spModelWork, m_transform->GetMatrix());
			}
			else if (m_spModel)
			{
				shader.DrawModel(*m_spModel, m_transform->GetMatrix());
			}
		}
		else if (m_useCustomEmissive && m_emissiveColor.LengthSquared() > 0.01f)
		{
			//カスタム発光色が設定されている場合
			customMaterial.Emissive = m_emissiveColor;
			shader.SetMaterialCB(customMaterial);

			if (m_spModelWork) shader.DrawModel(*m_spModelWork, m_transform->GetMatrix());
			else if (m_spModel) shader.DrawModel(*m_spModel, m_transform->GetMatrix());
		}

		//処理が終わったら、他のオブジェクトに影響しないようにマテリアル情報を元に戻す
		shader.SetMaterialCB(originalMaterial);
	}
}

void RenderComponent::GenerateDepthMapFromLight()
{
	if (!m_enable) return;
	if (m_spModelWork && m_transform)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModelWork, m_transform->GetMatrix());
	}
}

void RenderComponent::SetModel(const std::shared_ptr<KdModelData>& model)
{
	m_spModel = model;

	if (model)
	{
		m_modelPath = model->GetFilePath();
	}
	else
	{
		m_modelPath.clear();
	}

	if (m_spModelWork)
	{
		m_spModelWork->SetModelData(m_spModel);
	}
}

void RenderComponent::SetModel(const std::string& path)
{
	if (!path.empty())
	{
		m_spModel = KdAssets::Instance().m_modeldatas.GetData(path);
		m_modelPath = path;

		if (m_spModelWork)
		{
			m_spModelWork->SetModelData(m_spModel);
		}
	}
	else
	{
		m_spModel.reset();
		m_modelPath.clear();
	}
}

void RenderComponent::SetTargetModel(const std::string& path)
{
	if (!path.empty())
	{
		m_spTargetModel = KdAssets::Instance().m_modeldatas.GetData(path);
	}
}

void RenderComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Render Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (m_modelPath.empty())
		{
			ImGui::Text("Model Path: N/A");
		}
		else
		{
			std::string_view path_view = m_modelPath;
			ImGui::InputText("Model Path", (char*)path_view.data(), path_view.size(), ImGuiInputTextFlags_ReadOnly);
			//ImGui::TextWrapped("Model Path", (char*)path_view.data(), path_view.size(), ImGuiInputTextFlags_ReadOnly);
		}
	}
}

void RenderComponent::SetEmissiveColor(const Math::Vector3& color)
{
	m_emissiveColor = color;
	m_useCustomEmissive = true;
}

void RenderComponent::ResetEmissiveColor()
{
	m_emissiveColor = Math::Vector3::Zero;
	m_useCustomEmissive = false;
}

std::shared_ptr<KdModelWork> RenderComponent::GetModelWork()
{
	if (!m_spModelWork && m_spModel)
	{
		m_spModelWork = std::make_shared<KdModelWork>();
		m_spModelWork->SetModelData(m_spModel);
	}

	return m_spModelWork;
}

void RenderComponent::SetDissolve(float threshold, float range, const Math::Vector3& color, float resolution)
{
	m_isDissolving = (threshold > 0.0f);
	m_dissolveThreshold = threshold;
	m_dissolveEdgeRange = range;
	m_dissolveEdgeColor = color;
	m_resolution = resolution;
}