#pragma once
#include"../BaseScene/BaseScene.h"
#include"../Button/Button.h"
#include"../Slider/Slider.h"

class TitleScene:public BaseScene
{
public:
	enum class MenuState
	{
		Main,
		Create,
		NewStage,
		Volume
	};

	void Init()override;
	void SceneUpdate()override;
	void DrawSprite()override;
	void Release()override;

private:
	void DrawTitleWindow();

	//矩形描画
	void DrawRect(float x, float y, float w, float h, const Math::Color& color);

	//枠線描画
	void DrawFrame(float x, float y, float w, float h, float thickness, const Math::Color& color);


	float m_titleAlpha = 0.0f;
	float m_buttonAlpha = 0.0f;
	float m_cameraAngle = 0.0f;
	float m_animTime = 0.0f;
	Math::Vector2 m_playButtonPos = {};

	std::shared_ptr<KdTexture> m_buttonTex;
	std::shared_ptr<KdTexture> m_sliderTex;
	std::shared_ptr<KdTexture> m_knobTex;
	std::shared_ptr<KdTexture> m_windowTex;
	std::shared_ptr<KdTexture> m_titleLogoTex;

	std::map<MenuState, std::vector<Button>> m_buttons;
	std::map<MenuState, std::vector<Slider>> m_sliders;
	MenuState m_currentState;
};