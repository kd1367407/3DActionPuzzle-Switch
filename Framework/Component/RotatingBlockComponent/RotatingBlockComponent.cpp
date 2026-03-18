#include "RotatingBlockComponent.h"
#include"../Src/Application/main.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"
#include"../GameObject.h"
#include"../Src/Application/GameViewModel.h"
#include"../IdComponent/IdComponent.h"
#include"../GearRotateComponent/GearRotateComponent.h"
#include"../ParticleEmitterComponent/ParticleEmitterComponent.h"

void RotatingBlockComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("RotatingBlockComponent"))return;

	const auto& rotationData = data.at("RotatingBlockComponent");

	JsonHelper::GetVector3(rotationData, "rotation_axis", m_rotationAxis, m_rotationAxis);
	m_rotationAmount = JsonHelper::GetFloat(rotationData, "rotation_amount", m_rotationAmount);
	m_rotationSpeed = JsonHelper::GetFloat(rotationData, "rotation_speed", m_rotationSpeed);
}

void RotatingBlockComponent::Awake()
{
	//何もしない。宣言することが大事
}

void RotatingBlockComponent::Start()
{
	m_wpTransform = m_owner->GetComponent<TransformComponent>();
	m_wpEmitter = m_owner->GetComponent<ParticleEmitterComponent>();
	m_wpGear = m_owner->GetComponent<GearRotateComponent>();

	if (auto transform = m_wpTransform.lock())
	{
		Math::Vector3 initialEuler = transform->GetRot();

		//初期のオイラー角をクォータニオンに変換して、現在の「目標回転」として保存
		m_targetRotation.CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(initialEuler.y),
			DirectX::XMConvertToRadians(initialEuler.x),
			DirectX::XMConvertToRadians(initialEuler.z)
		);
	}

	if (auto gear = m_wpGear.lock())
	{
		m_defaultGearSpeed = gear->GetRotationSpeed();

		if (m_defaultGearSpeed < 0.1f)m_defaultGearSpeed = 90.0f;

	}
}

void RotatingBlockComponent::Update()
{
	float deltatime = Application::Instance().GetDeltaTime();

	if (m_cooldownTimer > 0.0f)
	{
		m_cooldownTimer -= deltatime;
	}

	if (auto emitter = m_wpEmitter.lock())
	{
		emitter->SetActive(m_isRotating);
	}

	//回転状態の変化検知
	if (m_isRotating != m_wasRotating)
	{
		if (auto gearComp = m_wpGear.lock())
		{
			if (m_isRotating)
			{
				//回転中は歯車を高速回転させる演出
				m_defaultGearSpeed = gearComp->GetRotationSpeed();
				gearComp->SetRotationSpeed(m_defaultGearSpeed * 4.0f);
			}
			else
			{
				gearComp->SetRotationSpeed(m_defaultGearSpeed);
			}
		}
		m_wasRotating = m_isRotating;
	}

	if (m_isRotating)
	{
		if (auto transform = m_wpTransform.lock())
		{
			//現在の回転をクォータニオンで取得
			Math::Vector3 currentEuler = transform->GetRot();
			Math::Quaternion currentQuat;
			currentQuat = currentQuat.CreateFromYawPitchRoll(
				DirectX::XMConvertToRadians(currentEuler.y),
				DirectX::XMConvertToRadians(currentEuler.x),
				DirectX::XMConvertToRadians(currentEuler.z)
			);

			//球面線形補完(Slerp)を使って、現在回転から目標回転へ滑らかに補間
			float deltaTime = Application::Instance().GetDeltaTime();
			Math::Quaternion nextQuat = Math::Quaternion::Slerp(currentQuat, m_targetRotation, deltaTime * m_rotationSpeed);

			//補間結果をオイラー角に戻してTransformにセット
			Math::Vector3 nextEuler = nextQuat.ToEuler();
			nextEuler.x = DirectX::XMConvertToDegrees(nextEuler.x);
			nextEuler.y = DirectX::XMConvertToDegrees(nextEuler.y);
			nextEuler.z = DirectX::XMConvertToDegrees(nextEuler.z);
			transform->SetRot(nextEuler);

			//目標回転とのドット積が1に近い(=ほぼ一致している)なら回転終了とみなす
			if (abs(currentQuat.Dot(m_targetRotation)) > 0.9999f)
			{
				//誤差をなくすために目標値で確定させる
				Math::Vector3 finalEuler = m_targetRotation.ToEuler();
				finalEuler.x = DirectX::XMConvertToDegrees(finalEuler.x);
				finalEuler.y = DirectX::XMConvertToDegrees(finalEuler.y);
				finalEuler.z = DirectX::XMConvertToDegrees(finalEuler.z);
				transform->SetRot(finalEuler);

				m_isRotating = false;
				m_cooldownTimer = 0.5f;
			}
		}
	}
}

void RotatingBlockComponent::OnCollision(const CollisionInfo& info)
{
	if (m_isRotating || m_cooldownTimer > 0.0f)return;

	//プレイヤーが上に乗ったら回転開始
	if (info.otherObject && info.contactNormal.y > 0.7f)
	{
		m_isRotating = true;

		//指定された軸周りに、指定角度分だけ回転させるクォータニオンを作成
		Math::Quaternion rotationStep;
		rotationStep = rotationStep.CreateFromAxisAngle(m_rotationAxis, DirectX::XMConvertToRadians(m_rotationAmount));

		//現在の目標回転に合成して、新しい目標回転を設定
		m_targetRotation = m_targetRotation * rotationStep;

		//チャージ音

	}
}

void RotatingBlockComponent::OnInspect()
{
	if (ImGui::CollapsingHeader("Rotating Block Compoent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool itemDeactivated = false;

		//--回転軸--
		ImGui::DragFloat3("Rotation Axis", &m_rotationAxis.x, 0.01f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--回転量--
		ImGui::DragFloat("Rotation Amount", &m_rotationAmount, 1.0f, 0.0f, 360.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//--回転スピード--
		ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.1f, 20.0f);
		itemDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

		//いずれかのウィジェットウィジェットのドラッグが終了した瞬間
		if (itemDeactivated)
		{
			RequestTransformChangeCommand();
		}
	}
}

void RotatingBlockComponent::RequestTransformChangeCommand()
{
	if (auto viewModel = m_wpViewModel.lock())
	{
		if (auto idComp = m_owner->GetComponent<IdComponent>())
		{
			viewModel->UpdateStateFromGameObject(m_owner->shared_from_this());
		}
	}
}

nlohmann::json RotatingBlockComponent::ToJson() const
{
	nlohmann::json j;

	j["rotation_axis"] = { m_rotationAxis.x, m_rotationAxis.y, m_rotationAxis.z };
	j["rotation_amount"] = m_rotationAmount;
	j["rotation_speed"] = m_rotationSpeed;

	return j;
}
