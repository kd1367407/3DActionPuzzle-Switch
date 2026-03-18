#pragma once
#include"../Component.h"

class TransformComponent;
class GameViewModel;

class MagicCircleComponent :public Component
{
public:
	void Awake()override;
	void Start()override;
	void Update()override;
	void DrawLit()override;
	void DrawBright()override;

	void OnSelect(bool isSelected);
	void SetModel(const std::string& path);
	std::string GetModelPath() const { return m_modelPath; }
	void RequestTransformChangeCommand();
	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel) { m_wpViewModel = viewModel; }

	const Math::Vector3& GetLocalPos()const { return m_localPos; }
	void SetLocalPos(const Math::Vector3& pos) { m_localPos = pos; }
	const Math::Vector3& GetLocalRot()const { return m_localRot; }
	void SetLocalRot(const Math::Vector3& rot) { m_localRot = rot; }
	const Math::Vector3& GetLocalScale()const { return m_localScale; }
	void SetLocalScale(const Math::Vector3& scale) { m_localScale = scale; }
	const float& GetOrbitRadius()const { return m_orbitRadius; }
	void SetOrbitRadius(const float& radius) { m_orbitRadius = radius; }
	const float& GetOrbitSpeed()const { return m_orbitSpeed; }
	void SetOrbitSpeed(const float& speed) { m_orbitSpeed = speed; }
	const Math::Vector3& GetOrbitAxisOffset()const { return m_orbitAxisOffset; }
	void SetOrbitAxisOffset(const Math::Vector3& offset) { m_orbitAxisOffset = offset; }
	void SetNormalSpeed(const float& speed) { m_normalSpeed = speed; }
	const float& GetNormalSpeed()const { return m_normalSpeed; }
	void SetSelectedSpeed(const float& speed) { m_selectedSpeed = speed; }
	const float& GetSelectedSpeed()const { return m_selectedSpeed; }
	void SetScaleMultiplier(const float& multiplier) { m_selectionScaleMultiplier = multiplier; }
	const float& GetScaleMultiplier()const { return m_selectionScaleMultiplier; }
	void SetScaleLerpSpeed(const float& speed) { m_scaleLerpSpeed = speed; }
	const float& GetScaleLerpSpeed()const { return m_scaleLerpSpeed; }

	void SetEnable(bool bEnable) { m_enable = bEnable; }
	bool IsEnable() const { return m_enable; }
	void SetEmissiveColor(const Math::Vector3& color);
	void ResetEmissiveColor();

	void SetUVScrollSpeed(const Math::Vector2 speed);
	void SetRotationSpeedMultiplier(float multiplier) { m_rotationSpeedMultiplier = multiplier; }

	void SetDissolve(float threshold, float range, const Math::Vector3& color, float resolution);

	void Configure(const nlohmann::json& data) override;
	nlohmann::json ToJson() const override;
	void OnInspect()override;
	const char* GetComponentName() const override { return "MagicCircleComponent"; }

private:
	std::shared_ptr<KdModelData> m_spModel;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	std::string m_modelPath;
	std::shared_ptr<TransformComponent> m_ownerTransform;
	Math::Vector3 m_localPos = Math::Vector3::Zero;
	Math::Vector3 m_localRot = Math::Vector3::Zero;
	Math::Vector3 m_localScale = { 2.0f,1.0f,2.0f };
	bool m_isDirty = false;
	float m_orbitAngle = 0.0f;
	float m_orbitSpeed = 45.0f;
	float m_orbitRadius = 0.0f;
	Math::Vector3 m_orbitAxisOffset = Math::Vector3::Zero;

	bool m_isSelected = false;
	float m_normalSpeed = 90.0f;
	float m_selectedSpeed = 360.0f;
	Math::Vector3 m_currentScale = Math::Vector3::One;
	float m_scaleLerpSpeed = 10.0f;
	float m_selectionScaleMultiplier = 1.5f;

	bool m_enable = true;
	Math::Vector3 m_currentEmissiveColor = { 0.5f, 0.5f, 0.5f };

	Math::Vector2 m_uvScrollSpeed = {};
	Math::Vector2 m_uvScrollOffset = {};

	bool m_isDissolving = false;
	float m_dissolveThreshold = 0.0f;
	float m_dissolveEdgeRange = 0.03f;
	Math::Vector3 m_dissolveEdgeColor = { 0.0f, 1.0f, 1.0f };
	float	m_resolution = 4.0f;

	float m_rotationSpeedMultiplier = 1.0f;
};