#pragma once
#include"../BaseScene/BaseScene.h"
#include"../Button/Button.h"

class GameObject;
class TimerComponent;

class ResultScene :public BaseScene
{
public:
	void Init()override;
	void SceneUpdate()override;
	void DrawSprite()override;
	void Release()override;

private:
	void DrawClearWindow();
	void DrawRankWindow();
	void DrawMoveWindow();

	void CalculateRank();

	//矩形描画
	void DrawRect(float x, float y, float w, float h, const Math::Color& color);

	//枠線描画
	void DrawFrame(float x, float y, float w, float h, float thickness, const Math::Color& color);

	std::shared_ptr<GameObject> m_timerObject;
	std::shared_ptr<TimerComponent> m_timerComp;

	//--データ--
	float m_finalTime = 0.0f;
	int m_playerMoves = 0;
	int m_parMoves = 0;

	//--演出用--
	float m_texAlpha = 0.0f;
	float m_moveAlpha = 0.0f;
	float m_rankAlpha = 0.0f;
	float m_buttonAlpha = 0.0f;

	Math::Vector2 m_basePos = { 0.0f,0.0f };

	bool m_hasCountUpStarted = false;
	bool m_showRank = false;

	//--テクスチャ--
	std::shared_ptr<KdTexture> m_clearTex;
	std::shared_ptr<KdTexture> m_playerMovesTex;
	std::shared_ptr<KdTexture> m_parTex;
	std::shared_ptr<KdTexture> m_numTex;
	std::shared_ptr<KdTexture> m_windowTex;
	std::shared_ptr<KdTexture> m_buttonTex;
	std::shared_ptr<KdTexture> m_rankSTex;
	std::shared_ptr<KdTexture> m_rankATex;
	std::shared_ptr<KdTexture> m_rankBTex;
	std::shared_ptr<KdTexture> m_rankCTex;
	std::shared_ptr<KdTexture> m_currentRankTex;
	
	//--ボタン--
	std::vector<std::shared_ptr<Button>> m_buttons;
};