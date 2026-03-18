#pragma once
#include"../BaseScene/BaseScene.h"

class Button;

class StageSelectScene :public BaseScene
{
public:
	void Init()override;
	void SceneUpdate()override;
	void DrawSprite()override;
	void Release()override;

private:

	enum class ViewMode
	{
		Main,
		Tutorial
	};

	struct StageButtonInfo
	{
		std::shared_ptr<Button> btn;
		std::string stagePath;
		std::string stageName;
	};

	//矩形描画
	void DrawRect(float x, float y, float w, float h, const Math::Color& color);

	//枠線描画
	void DrawFrame(float x, float y, float w, float h, float thickness,const Math::Color& color);

	std::vector<StageButtonInfo> m_stageButtons;

	//ボタン再構築
	void RebuildStageButtons();

	//プレビュー画像生成
	void GenerateStagePreview(const std::string& stagePath);

	ViewMode m_currentView = ViewMode::Main;

	std::map<std::string, std::shared_ptr<KdTexture>> m_stagePreviews;

	nlohmann::json m_stageListData;
	nlohmann::json m_tutorialStageListData;

	float m_buttonAlpha = 0.0f;

	bool m_needRebuild = false;

	//--ページ管理--
	int m_currentPage = 0;
	const int ITEMS_PER_PAGE = 5; //1ページに表示するステージ数
	std::vector<std::shared_ptr<Button>> m_systemButtons;

	//テクスチャ
	std::shared_ptr<KdTexture> m_windowTex;
	std::shared_ptr<KdTexture> m_buttonTex;
};