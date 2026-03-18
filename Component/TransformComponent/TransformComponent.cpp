#include "TransformComponent.h"
#include"../GameObject.h"
#include"../IdComponent/IdComponent.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"
#include"../MovingBlockComponent/MovingBlockComponent.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../../Editor/EditorGizmoContext.h"
#include"../Src/Application/Scene/GameScene/GameScene.h"
#include"../SinkingBlockComponent/SinkingBlockComponent.h"

void TransformComponent::PostUpdate()
{
	//値に変更があった場合のみ行列を再計算する(最適化)
	if (m_isDirty)
	{
		Math::Matrix scaleMat = Math::Matrix::CreateScale(m_scale);
		Math::Matrix rotMat = Math::Matrix::CreateFromYawPitchRoll
		(
			DirectX::XMConvertToRadians(m_rot.y),
			DirectX::XMConvertToRadians(m_rot.x),
			DirectX::XMConvertToRadians(m_rot.z)
		);
		Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
		m_worldMat = scaleMat * rotMat * transMat;
		m_isDirty = false;
	}
}

void TransformComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Transform Compoent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool valueChanged = false;
		bool itemDeactivated = false;

		//--Pos--
		valueChanged |= ImGui::DragFloat3("Position", &m_pos.x, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--Rot--
		valueChanged |= ImGui::DragFloat3("Rotation", &m_rot.x, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--Scale--
		valueChanged |= ImGui::DragFloat3("Scale", &m_scale.x, 0.1f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		if (valueChanged) { m_isDirty = true; }

		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
}

void TransformComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			//ViewModelを通じて変更コマンドを発行(Undo/Redo対応のため)
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

void TransformComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("TransformComponent"))return;

	const auto& transformData = data.at("TransformComponent");

	JsonHelper::GetVector3(transformData, "position", m_pos, m_pos);
	JsonHelper::GetVector3(transformData, "rotation", m_rot, m_rot);
	JsonHelper::GetVector3(transformData, "scale", m_scale, m_scale);
}

nlohmann::json TransformComponent::ToJson() const
{
	nlohmann::json j;
	j["position"] = { m_pos.x,m_pos.y,m_pos.z };
	j["rotation"] = { m_rot.x,m_rot.y,m_rot.z };
	j["scale"] = { m_scale.x,m_scale.y,m_scale.z };

	return j;
}

bool TransformComponent::OnDrawGizmos(const EditorGizmoContext& context, GameScene& scene)
{
	//特殊な動きをするコンポーネントがついている場合、ギズモ操作を無効化することがある
	if (m_owner->GetComponent<MovingBlockComponent>() || m_owner->GetComponent<SinkingBlockComponent>())
	{
		return false;
	}

	ImGuizmo::PushID("ObjectGizmo");

	bool isUsingGizmo = false;

	Math::Matrix objMat = GetMatrix();

	//ギズモ(移動・回転・拡縮ハンドル)の描画と操作
	ImGuizmo::Manipulate(
		(float*)context.viewMat, (float*)context.projMat,
		context.gizmoOperation, ImGuizmo::WORLD, (float*)&objMat
	);

	if (ImGuizmo::IsUsing())
	{
		if (!m_isGizmoDragging)
		{
			isUsingGizmo = true;
			m_isGizmoDragging = true;
		}
		//操作結果を行列から分解して反映
		Math::Vector3 newPos, newScale;
		Math::Quaternion newQuat;
		objMat.Decompose(newScale, newQuat, newPos);

		Math::Vector3 newRot = newQuat.ToEuler();
		newRot.x = DirectX::XMConvertToDegrees(newRot.x);
		newRot.y = DirectX::XMConvertToDegrees(newRot.y);
		newRot.z = DirectX::XMConvertToDegrees(newRot.z);

		SetPos(newPos);
		SetRot(newRot);
		SetScale(newScale);
	}
	else if (m_isGizmoDragging)
	{
		//ドラッグ終了時にコマンド発行(確定)
		RequestTransformChangeCommand();
		m_isGizmoDragging = false;
	}

	ImGuizmo::PopID();

	return isUsingGizmo;
}