#pragma once
#include"../CameraComponent.h"


class SceneCameraComponent : public CameraComponent
{
public:
	const char* GetComponentName() const override { return "SceneCameraComponent"; }

	void Awake() override;
	void PreDraw() override;
};