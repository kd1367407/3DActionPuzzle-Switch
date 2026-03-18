#pragma once
#include"../CameraComponent.h"

class TPSCameraComponent :public CameraComponent
{
public:
	void Awake() override;
	void Update() override;
	void PostUpdate() override;

	void SetTarget(const std::shared_ptr<TransformComponent>& target);

	const float& GetYaw()const { return m_yaw; }

	void SetForceMoveTarget(const Math::Vector3& targetPos, float duration);
	bool IsForceMoving() const { return m_isForceMoving; }

	const char* GetComponentName() const override { return "TPSCameraComponent"; }

private:
	float m_sensitivity;
	std::weak_ptr<TransformComponent> m_wpTarget;
	Math::Matrix m_localMat;
	float m_yaw = 0.0f;
	float m_pitch = 0.0f;
	POINT m_fixMousePos = {};

	//--強制移動要--
	bool m_isForceMoving = false;
	Math::Vector3 m_forceMoveStartPos;
	Math::Vector3 m_forceMoveEndPos;
	float m_forceMoveTimer = 0.0f;
	float m_forceMoveDuration = 0.0f;

	//強制移動中のカメラ回転維持
	Math::Quaternion m_startRotQuat;
	Math::Quaternion m_endRotQuat;

	Math::Vector3 m_currentTarget = {};
	bool m_isFirstFrame = true;
};