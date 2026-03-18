#include "SceneCameraComponent.h"
#include"../../TransformComponent/TransformComponent.h"
#include"../../GameObject.h"

void SceneCameraComponent::Awake()
{
	CameraComponent::Awake();
}

void SceneCameraComponent::PreDraw()
{
	if (!m_transform) return;

	m_spCamera->SetCameraMatrix(m_transform->GetMatrix());
}
