#include "GlowPartComponent.h"
#include "../TransformComponent/TransformComponent.h"
#include "../Src/Application/JsonHelper/JsonHelper.h"
#include"../GameObject.h"
#include"../IdComponent/IdComponent.h"

void GlowPartComponent::Awake()
{
}

void GlowPartComponent::Start()
{
	m_ownerTransform = m_owner->GetComponent<TransformComponent>();

	//互換性維持のため、設定がない場合はデフォルトを1つ追加
	if (m_instances.empty()) {
		AddGlowInstance("Default", Math::Vector3::Zero, Math::Vector3::Zero, Math::Vector3(1, 1, 1));
	}
}

void GlowPartComponent::Update()
{
	float deltatime = Application::Instance().GetDeltaTime();
	m_animTimer += deltatime;
}

void GlowPartComponent::DrawLit()
{
	if (!m_spModel || !m_ownerTransform) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;

	//--アニメーション計算--
	Math::Vector3 animOffset = Math::Vector3::Zero;

	//浮遊アニメーション
	if (m_enableFloat)
	{
		//Sin波を0.0～1.0の範囲に変換して滑らかな往復運動を作る
		float wave = (sin(m_animTimer * m_floatSpeed) + 1.0f) * 0.5f;

		Math::Vector3 dir = m_floatDirection;
		if (dir.LengthSquared() == 0) dir = Math::Vector3::Up;

		animOffset = dir * (wave * m_floatAmplitude);
	}

	//自転アニメーション
	float animAngle = 0.0f;
	if (m_enableRotate)
	{
		animAngle = m_animTimer * m_rotateSpeed;
	}

	//点滅アニメーション
	Math::Vector3 finalColor = m_glowColor;
	if (m_enableBlink)
	{
		//Sin波を取得
		float wave = sin(m_animTimer * m_blinkSpeed);

		//Sin波を正なら1、負なら0にして矩形波(パカパカ点滅)にする
		float blink = (wave > 0.0f) ? 1.0f : 0.0f;

		//強度を調整
		float intensity = 0.0f + (blink * 1.5);

		finalColor *= intensity;
	}

	Math::Matrix ownerMat = m_ownerTransform->GetMatrix();

	if (m_isDissolving)
	{
		shader.SetDissolve(m_dissolveThreshold, &m_dissolveEdgeRange, &m_dissolveEdgeColor, &m_resolution);
	}

	//各パーツの描画
	for (const auto& inst : m_instances)
	{
		Math::Matrix finalMat = Math::Matrix::Identity;
		Math::Matrix rotMat = Math::Matrix::Identity;

		//回転行列の計算
		if (m_enableFloat && m_isDirectional && m_floatDirection.LengthSquared() > 0.001f)
		{
			//特定方向を向くモード(JumpBlockの矢印など)
			Math::Vector3 targetDir = m_floatDirection;

			//ジンバルロック対策:上方向ベクトルとターゲット方向が平行に近い場合はUpベクトルを前方にする
			Math::Vector3 upVec = Math::Vector3::Up;
			if (abs(targetDir.Dot(upVec)) > 0.99f)
			{
				upVec = Math::Vector3::Forward;
			}

			//ターゲット方向を向く回転行列を作成 
			rotMat = Math::Matrix::CreateWorld(Math::Vector3::Zero, targetDir, upVec);

			//さらに自転を加える場合
			if (m_enableRotate)
			{
				Math::Matrix spin = Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(animAngle));
				rotMat = spin * rotMat;
			}
		}
		else
		{
			//通常回転モード
			Math::Matrix rotAnim = Math::Matrix::Identity;//自転

			if (m_enableRotate && m_rotateAxis.LengthSquared() > 0.001f)
			{
				rotAnim = Math::Matrix::CreateFromAxisAngle(m_rotateAxis, DirectX::XMConvertToRadians(animAngle));
			}

			//手動設定された基本姿勢
			Math::Matrix rotBase = Math::Matrix::CreateFromYawPitchRoll(
				DirectX::XMConvertToRadians(inst.baseRotation.y),
				DirectX::XMConvertToRadians(inst.baseRotation.x),
				DirectX::XMConvertToRadians(inst.baseRotation.z)
			);

			rotMat = rotAnim * rotBase;
		}

		//拡縮行列
		Math::Matrix scaleMat = Math::Matrix::CreateScale(inst.scale);

		//移動行列(オフセット+アニメーション移動)
		Math::Matrix transMat = Math::Matrix::CreateTranslation(inst.offset + animOffset);

		//行列合成
		finalMat = scaleMat * rotMat * transMat * ownerMat;

		//描画
		KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);
		shader.DrawModel(*m_spModel, finalMat, kWhiteColor, finalColor);
		KdShaderManager::Instance().UndoRasterizerState();
	}
}

void GlowPartComponent::DrawBright()
{
	if (!m_spModel || !m_ownerTransform) return;

	//発光フラグがOFFならBrightパスでは描画しない
	if (!m_isGlowActive) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;

	//--アニメーション計算(DrawLitと同様)--
	Math::Vector3 animOffset = Math::Vector3::Zero;

	if (m_enableFloat)
	{
		float wave = (sin(m_animTimer * m_floatSpeed) + 1.0f) * 0.5f;

		Math::Vector3 dir = m_floatDirection;
		if (dir.LengthSquared() == 0) dir = Math::Vector3::Up;

		animOffset = dir * (wave * m_floatAmplitude);
	}

	float animAngle = 0.0f;
	if (m_enableRotate)
	{
		animAngle = m_animTimer * m_rotateSpeed;
	}

	Math::Vector3 finalColor = m_glowColor;
	if (m_enableBlink)
	{
		float wave = sin(m_animTimer * m_blinkSpeed);
		float blink = (wave > 0.0f) ? 1.0f : 0.0f;
		float intensity = 0.0f + (blink * 1.5);
		finalColor *= intensity;
	}

	Math::Matrix ownerMat = m_ownerTransform->GetMatrix();

	if (m_isDissolving)
	{
		shader.SetDissolve(m_dissolveThreshold, &m_dissolveEdgeRange, &m_dissolveEdgeColor, &m_resolution);
	}

	for (const auto& inst : m_instances)
	{
		Math::Matrix finalMat = Math::Matrix::Identity;
		Math::Matrix rotMat = Math::Matrix::Identity;

		if (m_enableFloat && m_isDirectional && m_floatDirection.LengthSquared() > 0.001f)
		{
			Math::Vector3 targetDir = m_floatDirection;
			Math::Vector3 upVec = Math::Vector3::Up;
			if (abs(targetDir.Dot(upVec)) > 0.99f)
			{
				upVec = Math::Vector3::Forward;
			}

			rotMat = Math::Matrix::CreateWorld(Math::Vector3::Zero, targetDir, upVec);

			if (m_enableRotate)
			{
				Math::Matrix spin = Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(animAngle));
				rotMat = spin * rotMat;
			}
		}
		else
		{
			Math::Matrix rotAnim = Math::Matrix::Identity;

			if (m_enableRotate && m_rotateAxis.LengthSquared() > 0.001f)
			{
				rotAnim = Math::Matrix::CreateFromAxisAngle(m_rotateAxis, DirectX::XMConvertToRadians(animAngle));
			}

			Math::Matrix rotBase = Math::Matrix::CreateFromYawPitchRoll(
				DirectX::XMConvertToRadians(inst.baseRotation.y),
				DirectX::XMConvertToRadians(inst.baseRotation.x),
				DirectX::XMConvertToRadians(inst.baseRotation.z)
			);

			rotMat = rotAnim * rotBase;
		}

		Math::Matrix scaleMat = Math::Matrix::CreateScale(inst.scale);
		Math::Matrix transMat = Math::Matrix::CreateTranslation(inst.offset + animOffset);
		finalMat = scaleMat * rotMat * transMat * ownerMat;

		KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);
		shader.DrawModel(*m_spModel, finalMat, kWhiteColor, finalColor);
		KdShaderManager::Instance().UndoRasterizerState();
	}
}

void GlowPartComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("GlowPartComponent")) return;
	const auto& myData = data.at("GlowPartComponent");

	std::string path = JsonHelper::GetString(myData, "model");
	if (!path.empty()) SetModel(path);

	JsonHelper::GetVector3(myData, "glowColor", m_glowColor, m_glowColor);

	m_enableFloat = JsonHelper::GetBool(myData, "enableFloat", false);
	m_enableBlink = JsonHelper::GetBool(myData, "enableBlink", false);
	m_enableRotate = JsonHelper::GetBool(myData, "enableRotate", false);
	m_isDirectional = JsonHelper::GetBool(myData, "isDirectional", false);

	m_floatSpeed = JsonHelper::GetFloat(myData, "floatSpeed", 5.0f);
	m_blinkSpeed = JsonHelper::GetFloat(myData, "blinkSpeed", 5.0f);
	m_rotateSpeed = JsonHelper::GetFloat(myData, "rotateSpeed", 90.0f);
	JsonHelper::GetVector3(myData, "floatDirection", m_floatDirection, { 0.0f, 1.0f, 0.0f });
	JsonHelper::GetVector3(myData, "rotateAxis", m_rotateAxis, { 0.0f, 1.0f, 0.0f });

	if (myData.contains("instances") && myData["instances"].is_array())
	{
		m_instances.clear();
		Math::Vector3 defaultScale = { 1,1,1 };

		for (const auto& jsonInst : myData["instances"])
		{
			GlowInstanceData gData;
			gData.name = JsonHelper::GetString(jsonInst, "name", "Glow");
			JsonHelper::GetVector3(jsonInst, "offset", gData.offset);
			JsonHelper::GetVector3(jsonInst, "baseRot", gData.baseRotation);
			JsonHelper::GetVector3(jsonInst, "scale", defaultScale);
			m_instances.push_back(gData);
		}
	}
	else if (myData.contains("offset"))//後方互換性
	{
		m_instances.clear();
		Math::Vector3 oldOffset;
		JsonHelper::GetVector3(myData, "offset", oldOffset);
		AddGlowInstance("Default", oldOffset, Math::Vector3::Zero, Math::Vector3(1, 1, 1));
	}
}

nlohmann::json GlowPartComponent::ToJson() const
{
	nlohmann::json j;
	j["model"] = m_modelPath;
	j["glowColor"] = { m_glowColor.x, m_glowColor.y, m_glowColor.z };

	j["enableFloat"] = m_enableFloat;
	j["enableBlink"] = m_enableBlink;
	j["enableRotate"] = m_enableRotate;
	j["isDirectional"] = m_isDirectional;
	j["floatSpeed"] = m_floatSpeed;
	j["blinkSpeed"] = m_blinkSpeed;
	j["rotateSpeed"] = m_rotateSpeed;
	j["floatDirection"] = { m_floatDirection.x, m_floatDirection.y, m_floatDirection.z };
	j["rotateAxis"] = { m_rotateAxis.x, m_rotateAxis.y, m_rotateAxis.z };

	j["instances"] = nlohmann::json::array();

	for (const auto& inst : m_instances)
	{
		nlohmann::json i;
		i["name"] = inst.name;
		i["offset"] = { inst.offset.x, inst.offset.y, inst.offset.z };
		i["baseRot"] = { inst.baseRotation.x, inst.baseRotation.y, inst.baseRotation.z };
		i["scale"] = { inst.scale.x, inst.scale.y, inst.scale.z };
		j["instances"].push_back(i);
	}

	return j;
}

void GlowPartComponent::OnInspect()
{
	ImGui::PushID(this);

	if (ImGui::CollapsingHeader("GlowPar Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		if (!m_modelPath.empty()) ImGui::Text("Model: %s", m_modelPath.c_str());

		ImGui::ColorEdit3("Color", &m_glowColor.x);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::Separator();
		ImGui::Text("Animation");
		if (ImGui::Checkbox("Float", &m_enableFloat))itemDeactivated = true;
		if (ImGui::Checkbox("Blink", &m_enableBlink)) itemDeactivated = true;
		if (ImGui::Checkbox("Rotate", &m_enableRotate)) itemDeactivated = true;

		if (m_enableFloat)
		{
			ImGui::DragFloat("Float Speed", &m_floatSpeed, 0.1f);
			itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

			ImGui::DragFloat3("Float Direction", &m_floatDirection.x, 0.1f);
			itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::Checkbox("Is JumpBlock", &m_isDirectional))itemDeactivated = true;
		}

		if (m_enableRotate)
		{
			ImGui::DragFloat("Rot Speed", &m_rotateSpeed, 0.1f);
			itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

			ImGui::DragFloat3("Rot Axis", &m_rotateAxis.x, 0.1f);
			itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();
		}

		if (m_enableBlink)
		{
			ImGui::DragFloat("Blink Speed", &m_blinkSpeed, 0.1f);
			itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();
		}

		ImGui::Separator();
		ImGui::Text("Instances");

		if (ImGui::Button("Add Top")) {
			AddGlowInstance("Top", { 0.0, 1.0f, 0.0 }, { 0, 0, 0 }, Math::Vector3(1, 1, 1));
			itemDeactivated = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Buttom")) {
			AddGlowInstance("Buttom", { 0.0, -1.0f, 0.0 }, { 0, 0, 0 }, Math::Vector3(1, 1, 1));
			itemDeactivated = true;
		}

		if (ImGui::Button("Add Front")) {
			AddGlowInstance("Front", { 0.0, 0.0, -1.0f }, { 90, 0, 0 }, Math::Vector3(1, 1, 1));
			itemDeactivated = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Back")) {
			AddGlowInstance("Back", { 0.0, 0.0, 1.0f }, { 90, 0, 0 }, Math::Vector3(1, 1, 1));
			itemDeactivated = true;
		}

		if (ImGui::Button("Add Right")) {
			AddGlowInstance("Right", { 1.0, 0.0, 0.0f }, { 90, 0, 0 }, Math::Vector3(1, 1, 1));
			itemDeactivated = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Left")) {
			AddGlowInstance("Left", { -1.0, 0.0, 0.0f }, { 90, 0, 0 }, Math::Vector3(1, 1, 1));
			itemDeactivated = true;
		}

		int removeIndex = -1;

		for (int i = 0; i < m_instances.size(); ++i)
		{
			auto& inst = m_instances[i];
			std::string header = inst.name + "##" + std::to_string(i);

			if (ImGui::TreeNode(header.c_str()))
			{
				ImGui::DragFloat3("Offset", &inst.offset.x, 0.01f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				ImGui::DragFloat3("Base Rot", &inst.baseRotation.x, 1.0f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				ImGui::DragFloat3("Scale", &inst.scale.x, 0.01f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				if (ImGui::Button("Remove")) {
					removeIndex = i; itemDeactivated = true;
				}

				ImGui::TreePop();
			}
		}

		if (removeIndex != -1) {
			m_instances.erase(m_instances.begin() + removeIndex);
		}

		if (itemDeactivated)
		{
			RequestStateChangeCommand();
		}
	}

	ImGui::PopID();
}

void GlowPartComponent::RequestStateChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

void GlowPartComponent::SetFloatAnimation(const Math::Vector3& direction, float speed, float amplitude)
{
	m_enableFloat = true;
	m_floatDirection = direction;
	m_floatSpeed = speed;
	m_floatAmplitude = amplitude;
}

void GlowPartComponent::SetModel(const std::string& path)
{
	if (!path.empty())
	{
		m_spModel = KdAssets::Instance().m_modeldatas.GetData(path);
		m_modelPath = path;
	}
}

void GlowPartComponent::AddGlowInstance(const std::string& name, const Math::Vector3& offset, const Math::Vector3& rot, const Math::Vector3& scale)
{
	GlowInstanceData data;
	data.name = name;
	data.offset = offset;
	data.baseRotation = rot;
	data.scale = scale;
	m_instances.push_back(data);
}