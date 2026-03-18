#pragma once
#include"../Component.h"

class TransformComponent;
class GameViewModel;

class BouncerComponent :public Component
{
public:
	void Awake()override;
	void Start() override;
	void Update() override;
	void DrawLit() override;

	void OnJump(float duration);

	void Configure(const nlohmann::json& data) override;
	nlohmann::json ToJson() const override;
	const char* GetComponentName() const override { return "BouncerComponent"; }

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
	void SetOffset(const Math::Vector3& offset) { m_baseOffset = offset; }
	const Math::Vector3& GetOffset() { return m_baseOffset; }
	void SetStroke(float amount) { m_stroke = amount; }
	const float& GetStroke() { return m_stroke; }

private:
	std::shared_ptr<KdModelData> m_spModel;
	std::string m_modelPath;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	std::shared_ptr<TransformComponent> m_ownerTransform;

	//設定値
	Math::Vector3 m_baseOffset = Math::Vector3::Zero;
	float m_stroke = 0.3f;
	float m_animationDuration = 0.3f;

	//内部状態
	bool m_isAnimating = false;
	float m_animTimer = 0.0f;
	Math::Vector3 m_currentDrawOffset = Math::Vector3::Zero;

	//ディゾルブ
	bool m_isDissolving = false;
	float m_dissolveThreshold = 0.0f;
	float m_dissolveEdgeRange = 0.01f;
	Math::Vector3 m_dissolveEdgeColor = { 1.0f, 1.0f, 1.0f };
	float m_resolution = 4.0f;
};