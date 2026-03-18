#pragma once
#include "../Component.h"
#include"../Src/Application/GameData/BlockState/BlockState.h"

class TransformComponent;
class GameViewModel;

class GlowPartComponent : public Component
{
public:
	void Awake() override;
	void Start() override;
	void Update()override;
	void DrawLit()override;
	void DrawBright() override;

	void Configure(const nlohmann::json& data) override;
	nlohmann::json ToJson() const override;
	const char* GetComponentName() const override { return "GlowPartComponent"; }

	void OnInspect()override;
	void RequestStateChangeCommand();
	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel) { m_wpViewModel = viewModel; }

	//指定した方向へ揺れる＆向く
	void SetFloatAnimation(const Math::Vector3& direction, float speed = 5.0f, float amplitude = 0.2f);

	void SetDissolve(float threshold, float range, const Math::Vector3& color, float resolution)
	{
		m_isDissolving = (threshold > 0.0f);
		m_dissolveThreshold = threshold;
		m_dissolveEdgeRange = range;
		m_dissolveEdgeColor = color;
		m_resolution = resolution;
	}

	//ゲッター/セッター
	void SetModel(const std::string& path);
	std::string GetModelPath() const { return m_modelPath; }
	void SetGlowColor(const Math::Vector3& color) { m_glowColor = color; }
	const Math::Vector3& GetGlowColor() { return m_glowColor; }
	void SetEnableFloat(bool enable) { m_enableFloat = enable; }
	bool GetEnableFloat() { return m_enableFloat; }
	void SetEnableBlink(bool enable) { m_enableBlink = enable; }
	bool GetEnableBlink() { return m_enableBlink; }
	void SetEnableRotate(bool enable) { m_enableRotate = enable; }
	bool GetEnableRotate() { return m_enableRotate; }
	void SetFloatSpeed(const float& speed) { m_floatSpeed = speed; }
	const float& GetFloatSpeed() { return m_floatSpeed; }
	void SetBlinkSpeed(const float& speed) { m_blinkSpeed = speed; }
	const float& GetBlinkSpeed() { return m_blinkSpeed; }
	void SetRotateSpeed(const float& speed) { m_rotateSpeed = speed; }
	const float& GetRotateSpeed() { return m_rotateSpeed; }
	void SetFloatDirection(const Math::Vector3& dir) { m_floatDirection = dir; }
	const Math::Vector3& GetFloatDirection() const { return m_floatDirection; }
	void SetRotateAxis(const Math::Vector3& axis) { m_rotateAxis = axis; }
	const Math::Vector3& GetRotateAxis() const { return m_rotateAxis; }
	void SetIsDirectional(bool enable) { m_isDirectional = enable; }
	bool GetIsDirectional() const { return m_isDirectional; }

	void SetGlowActive(bool active) { m_isGlowActive = active; }
	bool IsGlowActive() const { return m_isGlowActive; }

	//個別の設定
	void SetGlowInstances(const std::vector<GlowInstanceData>& instances) { m_instances = instances; }
	const std::vector<GlowInstanceData>& GetGlowInstances() const { return m_instances; }

	//パーツ追加
	void AddGlowInstance(const std::string& name, const Math::Vector3& offset, const Math::Vector3& rot, const Math::Vector3& scale);

private:
	std::shared_ptr<KdModelData> m_spModel;
	std::string m_modelPath;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	std::shared_ptr<TransformComponent> m_ownerTransform;

	//--共通設定--
	Math::Vector3 m_glowColor = { 0.0f, 50.0f, 0.0f };

	//--アニメーション--
	bool m_enableFloat = false;
	bool m_enableBlink = false;
	bool m_enableRotate = false;
	bool m_isDirectional = false;
	float m_floatSpeed = 5.0f;
	Math::Vector3 m_floatDirection = { 0.0f, 1.0f, 0.0f };
	float m_floatAmplitude = 0.2f;
	float m_blinkSpeed = 5.0f;
	float m_rotateSpeed = 90.0f;
	Math::Vector3 m_rotateAxis = { 0.0f, 1.0f, 0.0f };
	float m_animTimer = 0.0f;

	//ディゾルブ
	bool m_isDissolving = false;
	float m_dissolveThreshold = 0.0f;
	float m_dissolveEdgeRange = 0.01f;
	Math::Vector3 m_dissolveEdgeColor = { 1.0f, 1.0f, 1.0f };
	float m_resolution = 4.0f;

	bool m_isGlowActive = true;
	std::vector<GlowInstanceData> m_instances;
};