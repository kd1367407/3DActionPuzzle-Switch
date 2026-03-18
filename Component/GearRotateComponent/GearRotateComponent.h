#pragma once
#include"../Component.h"
#include "../Src/Application/GameData/BlockState/BlockState.h"

class TransformComponent;
class GameViewModel;

class GearRotateComponent :public Component
{
public:
	void Awake()override;
	void Start() override;
	void Update() override;
	void DrawLit() override;

	void Configure(const nlohmann::json& data) override;
	nlohmann::json ToJson() const override;
	const char* GetComponentName() const override { return "GearRotateComponent"; }

	void OnInspect() override;
	void RequestTransformChangeCommand();
	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel) { m_wpViewModel = viewModel; }

	void SetDissolve(float threshold, float range, const Math::Vector3& color, float resolution)
	{
		m_isDissolving = (threshold > 0.0f);
		m_dissolveThreshold = threshold;
		m_dissolveEdgeRange = range;
		m_dissolveEdgeColor = color;
		m_resolution = resolution;
	}

	void SetModel(const std::string& path);
	std::string GetModelPath() const { return m_modelPath; }

	void SetRotationSpeed(const float& speed) { m_rotationSpeed = speed; }
	const float& GetRotationSpeed() { return m_rotationSpeed; }

	void AddGear(const std::string& name, const Math::Vector3& offset, const Math::Vector3& axis, const Math::Vector3& baseRot, float startAngle, bool reverse, const Math::Vector3& scale);
	const std::vector<GearData>& GetGears() const { return m_gears; }
	void SetGears(const std::vector<GearData>& gears) { m_gears = gears; }

private:
	std::shared_ptr<KdModelData> m_spModel;
	std::string m_modelPath;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	std::shared_ptr<TransformComponent> m_ownerTransform;

	//共通パラメータ
	float m_rotationSpeed = 90.0f;
	float m_currentAngle = 0.0f;

	//歯車リスト
	std::vector<GearData> m_gears;

	//ディゾルブ
	bool m_isDissolving = false;
	float m_dissolveThreshold = 0.0f;
	float m_dissolveEdgeRange = 0.01f;
	Math::Vector3 m_dissolveEdgeColor = { 1.0f, 1.0f, 1.0f };
	float m_resolution = 4.0f;
};