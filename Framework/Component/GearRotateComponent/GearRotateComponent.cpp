#include "GearRotateComponent.h"
#include "../TransformComponent/TransformComponent.h"
#include "../Src/Application/JsonHelper/JsonHelper.h"
#include "../GameObject.h"
#include "../IdComponent/IdComponent.h"
#include "../Src/Application/main.h"

void GearRotateComponent::Awake()
{
}

void GearRotateComponent::Start()
{
	m_ownerTransform = m_owner->GetComponent<TransformComponent>();
}

void GearRotateComponent::Update()
{
	//回転角度の更新
	float deltatime = Application::Instance().GetDeltaTime();
	m_currentAngle += m_rotationSpeed * deltatime;

	//数値が大きくなりすぎないようにループさせる
	if (m_currentAngle > 360.0f) m_currentAngle -= 360.0f;
	if (m_currentAngle < -360.0f) m_currentAngle += 360.0f;
}

void GearRotateComponent::DrawLit()
{
	if (!m_spModel || !m_ownerTransform) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	const Math::Matrix& ownerMat = m_ownerTransform->GetMatrix();

	for (const auto& gear : m_gears)
	{
		//自転角度計算(逆回転フラグ考慮)
		float angle = gear.reverse ? -m_currentAngle : m_currentAngle;

		//初期角度を加算してオフセット
		angle += gear.startAngle;

		//回転軸(Axis)に基づいた回転行列作成
		Math::Matrix rotMat = Math::Matrix::CreateFromAxisAngle(gear.rotationAxis, DirectX::XMConvertToRadians(angle));

		//歯車の向き(基本姿勢)を設定
		Math::Matrix baseRotMat = Math::Matrix::CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(gear.baseRotation.y),
			DirectX::XMConvertToRadians(gear.baseRotation.x),
			DirectX::XMConvertToRadians(gear.baseRotation.z)
		);

		//scale
		Math::Matrix scaleMat = Math::Matrix::CreateScale(gear.scale);

		//offset(配置位置)
		Math::Matrix transMat = Math::Matrix::CreateTranslation(gear.offset);

		//行列合成: 拡大縮小->自転->基本姿勢->配置位置->親のワールド変換
		Math::Matrix finalMat = scaleMat * rotMat * baseRotMat * transMat * ownerMat;

		if (m_isDissolving)
		{
			shader.SetDissolve(m_dissolveThreshold, &m_dissolveEdgeRange, &m_dissolveEdgeColor, &m_resolution);
		}

		shader.DrawModel(*m_spModel, finalMat);
	}
}

void GearRotateComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("GearRotateComponent")) return;
	const auto& myData = data.at("GearRotateComponent");

	std::string path = JsonHelper::GetString(myData, "model");
	if (!path.empty()) SetModel(path);

	m_rotationSpeed = JsonHelper::GetFloat(myData, "speed", 90.0f);

	//歯車リストの読み込み
	if (myData.contains("gears") && myData["gears"].is_array())
	{
		m_gears.clear();
		for (const auto& gearJson : myData["gears"])
		{
			GearData gear;
			gear.name = JsonHelper::GetString(gearJson, "name", "Gear");
			JsonHelper::GetVector3(gearJson, "offset", gear.offset);
			JsonHelper::GetVector3(gearJson, "axis", gear.rotationAxis, { 0, 1, 0 });
			JsonHelper::GetVector3(gearJson, "baseRot", gear.baseRotation);
			gear.startAngle = JsonHelper::GetFloat(gearJson, "startAngle", 0.0f);
			gear.reverse = JsonHelper::GetBool(gearJson, "reverse", false);
			JsonHelper::GetVector3(gearJson, "scale", gear.scale, { 1.0f, 1.0f, 1.0f });
			m_gears.push_back(gear);
		}
	}
}

nlohmann::json GearRotateComponent::ToJson() const
{
	nlohmann::json j;
	j["model"] = m_modelPath;
	j["speed"] = m_rotationSpeed;

	//歯車リストの保存
	j["gears"] = nlohmann::json::array();
	for (const auto& gear : m_gears)
	{
		nlohmann::json g;
		g["name"] = gear.name;
		g["offset"] = { gear.offset.x, gear.offset.y, gear.offset.z };
		g["axis"] = { gear.rotationAxis.x, gear.rotationAxis.y, gear.rotationAxis.z };
		g["baseRot"] = { gear.baseRotation.x, gear.baseRotation.y, gear.baseRotation.z };
		g["startAngle"] = gear.startAngle;
		g["reverse"] = gear.reverse;
		g["scale"] = { gear.scale.x, gear.scale.y, gear.scale.z };
		j["gears"].push_back(g);
	}
	return j;
}

void GearRotateComponent::OnInspect()
{
	ImGui::PushID(this);//ID競合回避

	if (ImGui::CollapsingHeader("GearRotate Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		if (!m_modelPath.empty()) ImGui::Text("Model: %s", m_modelPath.c_str());

		ImGui::DragFloat("Speed", &m_rotationSpeed, 1.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::Separator();
		ImGui::Text("GearList");

		//プリセット追加ボタン群
		if (ImGui::Button("Add Top"))
		{
			AddGear("Top", { 0, 1.0f, 0 }, { 0, 1, 0 }, { 0, 0, 0 }, 0, false, { 1,1,1 });
			itemDeactivated = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Bottom"))
		{
			AddGear("Bottom", { 0, -1.0f, 0 }, { 0, 1, 0 }, { 0, 0, 0 }, 0, false, { 1,1,1 });
			itemDeactivated = true;
		}

		if (ImGui::Button("Add Back")) {
			AddGear("Back", { 0, 0, -1.0f }, { 0, 0, 1 }, { -90, 0, 0 }, 0, false, { 1,1,1 });
			itemDeactivated = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Front")) {
			AddGear("Front", { 0, 1.0f, 0 }, { 0, 1, 0 }, { 90, 0, 0 }, 0, false, { 1,1,1 });
			itemDeactivated = true;
		}

		if (ImGui::Button("Add Right")) {
			AddGear("Right", { 1.0f, 0, 0 }, { 1, 0, 0 }, { 0, 0, -90 }, 0, false, { 1,1,1 });
			itemDeactivated = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Left")) {
			AddGear("Left", { -1.0f, 0, 0 }, { 1, 0, 0 }, { 0, 0, 90 }, 0, false, { 1,1,1 });
			itemDeactivated = true;
		}

		//リスト表示と編集
		int removeIndex = -1;
		for (int i = 0; i < m_gears.size(); ++i)
		{
			auto& gear = m_gears[i];
			std::string headerName = gear.name + "##" + std::to_string(i);

			//パラメータ編集
			if (ImGui::TreeNode(headerName.c_str()))
			{
				ImGui::DragFloat3("Offset", &gear.offset.x, 0.01f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				ImGui::DragFloat3("Axis", &gear.rotationAxis.x, 0.1f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				ImGui::DragFloat3("BaseRot", &gear.baseRotation.x, 1.0f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				ImGui::DragFloat("Start Angle", &gear.startAngle, 1.0f, 0.0f, 360.0f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				ImGui::DragFloat3("Scale", &gear.scale.x, 0.01f);
				itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

				if (ImGui::Checkbox("Reverse", &gear.reverse))
				{
					itemDeactivated = true;
				}

				if (ImGui::Button("Remove"))
				{
					removeIndex = i;
					itemDeactivated = true;
				}
				ImGui::TreePop();
			}
		}

		//削除処理
		if (removeIndex != -1)
		{
			m_gears.erase(m_gears.begin() + removeIndex);
		}

		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
	ImGui::PopID();
}

void GearRotateComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

void GearRotateComponent::SetModel(const std::string& path)
{
	m_spModel = KdAssets::Instance().m_modeldatas.GetData(path);
	m_modelPath = path;
}

void GearRotateComponent::AddGear(const std::string& name, const Math::Vector3& offset, const Math::Vector3& axis, const Math::Vector3& baseRot, float startAngle, bool reverse, const Math::Vector3& scale)
{
	GearData g;
	g.name = name;
	g.offset = offset;
	g.rotationAxis = axis;
	g.baseRotation = baseRot;
	g.startAngle = startAngle;
	g.reverse = reverse;
	g.scale = scale;
	m_gears.push_back(g);
}