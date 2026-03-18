#pragma once
#include"../Component.h"

class TransformComponent;
class GameViewModel;

//3Dモデルの描画を担当するコンポーネント
class RenderComponent :public Component
{
public:
	//ハイライトの状態
	enum class HighlightState
	{
		None,
		Hoverd,//カーソルが乗っている
		Selected//選択されている
	};

	void Awake()override;
	void Start()override;

	void Configure(const nlohmann::json& data);
	nlohmann::json ToJson() const override;
	const char* GetComponentName()const override { return "RenderComponent"; }

	void DrawLit()override;
	void DrawBright()override;
	void GenerateDepthMapFromLight()override;

	void SetModel(const std::shared_ptr<KdModelData>& model);
	void SetModel(const std::string& path);

	void SetTargetModel(const std::string& path);

	std::string GetModelPath() const { return m_modelPath; }

	//ハイライトの状態を外部が設定、取得
	void SetHighlightState(HighlightState state) { m_highlightState = state; }
	HighlightState GetHighlightState()const { return m_highlightState; }

	void OnInspect()override;

	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel) { m_wpViewModel = viewModel; }

	void SetEnable(bool bEnable) { m_enable = bEnable; }
	bool IsEnable() const { return m_enable; }

	void SetEmissiveColor(const Math::Vector3& color);
	void ResetEmissiveColor();

	std::shared_ptr<KdModelWork> GetModelWork();

	void SetDissolve(float threshold, float range, const Math::Vector3& color, float resolution);

private:
	std::shared_ptr<KdModelData> m_spModel;
	std::shared_ptr<KdModelWork> m_spModelWork;
	std::shared_ptr<KdModelData> m_spTargetModel;
	std::string m_modelPath;
	std::weak_ptr<GameViewModel> m_wpViewModel;

	//描画に必要な他のコンポーネントへの参照を保持する
	std::shared_ptr<TransformComponent> m_transform;

	//ハイライトの状態管理
	HighlightState m_highlightState = HighlightState::None;
	Math::Vector3 m_emissiveColor = Math::Vector3::Zero;//現在の発光色
	bool m_useCustomEmissive = false;

	//描画するかどうか
	bool m_enable = true;

	//--ディゾルブ変数--
	bool	m_isDissolving = false;
	float m_dissolveThreshold = 0.0f;
	float m_dissolveEdgeRange = 0.01f;
	Math::Vector3 m_dissolveEdgeColor = { 1.0f, 1.0f, 1.0f };
	float	m_resolution = 4.0f;
};
