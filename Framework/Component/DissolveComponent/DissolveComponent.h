#pragma once
#include"../Component.h"

class RenderComponent;
class GameViewModel;
class GlowPartComponent;
class GearRotateComponent;
class BouncerComponent;

class DissolveComponent :public Component
{
public:
	enum class State
	{
		Idle,
		FadingOut,
		FadingIn
	};

	void Configure(const nlohmann::json& data) override;
	void	Awake()override;
	void	Start()override;
	void	Update()override;

	nlohmann::json ToJson() const override;
	void OnInspect() override;
	void RequestTransformChangeCommand();

	const char* GetComponentName() const override { return "DissolveComponent"; }

	//--外部からアニメーションを制御--
	void	FadeOut();
	void	FadeIn();

	//--外部からアニメーションと共に処理を登録させる--
	std::function<void()> OnFadeOutComplete;
	std::function<void()> OnFadeInComplete;

	//--ゲッター/セッター--
	void SetViewModel(const std::shared_ptr<GameViewModel>& viewModel) { m_wpViewModel = viewModel; }
	const float& GetFadeDuration()const { return m_fadeDuration; }
	const Math::Vector3& GetEdgeColor()const { return m_edgeColor; }
	const float& GetEdgeRange()const { return m_edgeRange; }
	const float& GetBlockResolution()const { return m_blockResolution; }

	void SetFadeDuration(const float& duration) { m_fadeDuration = duration; }
	void SetEdgeColor(const Math::Vector3& color) { m_edgeColor = color; }
	void SetEdgeRange(const float& range) { m_edgeRange = range; }
	void SetBlockResolution(const float& resolution) { m_blockResolution = resolution; }

private:
	//--制御対象のコンポーネント--
	std::weak_ptr<RenderComponent> m_wpRenderComp;
	std::weak_ptr<GameViewModel> m_wpViewModel;
	std::weak_ptr<GlowPartComponent> m_wpGlowComp;
	std::weak_ptr<GearRotateComponent> m_wpGearComp;
	std::weak_ptr< BouncerComponent> m_wpBouncer;

	//--設定変数--
	float	m_fadeDuration = 0.3f;//アニメーションにかかる時間
	Math::Vector3 m_edgeColor = { 1.0f,1.0f,1.0f };//消えるフチの色
	float m_edgeRange = 0.01f;//フチの幅
	float m_blockResolution = 4.0f;//ブロック状の解像度

	State m_state = State::Idle;
	float	m_timer = 0.0f;
	float	 m_currentThreshold = 0.0f;//シェーダに渡すしきい値
};