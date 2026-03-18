#include "ResultScene.h"
#include"../GameScene/GameManager/GameManager.h"
#include"../Src/Framework/Component/TimerComponent/TimerComponent.h"
#include"../Src/Framework/Component/TransformComponent/TransformComponent.h"
#include"../Src/Framework/Component/GameObject.h"
#include"../Src/Framework/Component/SkydomeComponent/SkydomeComponent.h"
#include"../Src/Framework/Component/CameraComponent/SceneCameraComponent/SceneCameraComponent.h"

void ResultScene::Init()
{
	SceneManager::Instance().SetMode(SceneManager::SceneMode::UI);

	while (ShowCursor(TRUE) < 0);
	ClipCursor(nullptr);
	ImGui::GetIO().MouseDrawCursor = true;

	//--データ取得--
	m_finalTime = GameManager::Instance().GetFinalTime();
	m_playerMoves = GameManager::Instance().GetFinalMoves();
	m_parMoves = GameManager::Instance().GetParMoves();

	//カメラ
	auto cameraObj = std::make_shared<GameObject>();
	auto cameraComp = cameraObj->AddComponent<SceneCameraComponent>();
	cameraObj->AddComponent<TransformComponent>();
	cameraObj->GetComponent<TransformComponent>()->SetPos({ 0.0f, 5.0f, -10.0f });
	cameraObj->GetComponent<TransformComponent>()->SetRot({ 20.0f, 0.0f, 0.0f });
	cameraObj->Init();
	AddObject(cameraObj);
	SetActiveCamera(cameraComp);

	//--背景--
	auto sky = std::make_shared<GameObject>();
	sky->AddComponent<SkydomeComponent>();
	sky->AddComponent<TransformComponent>();
	sky->Init();
	AddObject(sky);


	//--TimerObject準備--
	m_timerObject = std::make_shared<GameObject>();
	m_timerComp = m_timerObject->AddComponent<TimerComponent>();
	m_timerComp->SetUseScreenAutoPos(false);
	auto timerTransform = m_timerObject->AddComponent<TransformComponent>();
	timerTransform->SetPos({ -150.0f,190.0f,0.0f });
	timerTransform->SetScale({ 1.2f,1.2f,1.0f });
	m_timerObject->Init();
	AddObject(m_timerObject);

	//テクスチャロード
	m_clearTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/StageClear3.png");
	m_windowTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/white.png");
	m_buttonTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/bar_square_gloss_large.png");
	m_rankSTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/Rank/RankS.png");
	m_rankATex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/Rank/RankA.png");
	m_rankBTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/Rank/RankB.png");
	m_rankCTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/Rank/RankC.png");

	//--ランク計算--
	CalculateRank();

	//--ボタン--
	int fontNo = 2;
	Math::Color textColor = { 0.8f, 0.9f, 1.0f, 1.0f };
	float btnScale = 1.2f;

	//Retry
	m_buttons.push_back(std::make_shared<Button>(
		Math::Vector2(0, -200), m_buttonTex, fontNo, "RETRY",
		[]() {
			SceneManager::Instance().SetMode(SceneManager::SceneMode::Game);
			SceneManager::Instance().ChangeScene(SceneManager::SceneType::Game);
		}, 1.0f, btnScale, textColor
	));

	//Select
	m_buttons.push_back(std::make_shared<Button>(
		Math::Vector2(0, -240), m_buttonTex, fontNo, "SELECT",
		[]() { SceneManager::Instance().ChangeScene(SceneManager::SceneType::StageSelect); },
		1.0f, btnScale, textColor
	));

	//Title
	m_buttons.push_back(std::make_shared<Button>(
		Math::Vector2(0, -280), m_buttonTex, fontNo, "TITLE",
		[]() { SceneManager::Instance().ChangeScene(SceneManager::SceneType::Title); },
		1.0f, btnScale, textColor
	));

	//--初期化--
	m_texAlpha = 0.0f;
	m_moveAlpha = 0.0f;
	m_rankAlpha = 0.0f;
	m_buttonAlpha = 0.0f;
	m_showRank = false;
	m_hasCountUpStarted = false;

	//--サウンド-
	m_spBGM = KdAudioManager::Instance().Play("Asset/Sound/TitleBGM.wav", true, 1.0f);
}

void ResultScene::SceneUpdate()
{
	float deltatime = Application::Instance().GetDeltaTime();
	float fadeSpeed = 1.5f;
	auto& fader = SceneManager::Instance().GetFader();

	//シーケンス制御:クリアロゴ->タイマー->スコア->ランク->ボタンの順に表示する

	//1.クリアロゴ
	if (!fader.IsFadeing() && m_texAlpha < 1.0f)
	{
		m_texAlpha += fadeSpeed * deltatime;
	}
	m_texAlpha = std::min(m_texAlpha, 1.0f);

	//2.タイマーカウントアップ
	if (m_texAlpha >= 1.0f)
	{
		if (!m_hasCountUpStarted)
		{
			//カウントアップ開始
			m_timerComp->StartCountUp(m_finalTime);
			m_hasCountUpStarted = true;
			//カウントアップ音
		}

		//3.カウントアップが終わったらスコア表示開始
		if (!m_timerComp->UpdateCountUp(deltatime))
		{
			m_moveAlpha += fadeSpeed * deltatime;
		}
	}
	m_moveAlpha = std::min(m_moveAlpha, 1.0f);

	//4.ランク表示(ハンコ)
	if (m_moveAlpha >= 1.0f)
	{
		m_showRank = true;
		m_rankAlpha += fadeSpeed * deltatime;
	}
	m_rankAlpha = std::min(m_rankAlpha, 1.0f);

	//5.ボタン表示
	if (m_rankAlpha >= 1.0f)
	{
		m_buttonAlpha += fadeSpeed * deltatime;
	}
	m_buttonAlpha = std::min(m_buttonAlpha, 1.0f);

	//ボタン更新
	if (m_buttonAlpha > 0.8f)
	{
		for (auto& btn : m_buttons)
		{
			btn->Update();
		}
	}
}

void ResultScene::DrawSprite()
{
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);

	BaseScene::DrawSprite();

	float winX = -5;
	float winY = -55;
	float winW = 550;
	float winH = 550;

	//背景（半透明の黒）
	DrawRect(winX, winY, winW, winH, { 0.0f, 0.0f, 0.1f, 0.8f * 1.0 });

	//枠線（発光しているようなシアン）
	DrawFrame(winX, winY, winW, winH, 4.0f, { 0.0f, 1.0f, 1.0f, 1.0 });

	//四隅の飾り
	float decoSize = 20.0f;
	float decoThick = 8.0f;

	//左上だけ例示（他も同様に計算）
	DrawRect(winX - winW / 2, winY + winH / 2, decoSize, decoThick, { 0, 1, 1, m_buttonAlpha }); // 横
	DrawRect(winX - winW / 2, winY + winH / 2, decoThick, decoSize, { 0, 1, 1, m_buttonAlpha }); // 縦


	//クリアロゴ
	DrawClearWindow();

	//タイマー
	if (m_timerObject)
	{
		auto& fader = SceneManager::Instance().GetFader();
		if (!fader.IsFadeing())
		{
			m_timerObject->DrawSprite();
		}
	}

	//スコア
	DrawMoveWindow();

	//ランク
	DrawRankWindow();

	//ボタン
	for (auto& btn : m_buttons)
	{
		btn->Draw(m_buttonAlpha);
	}

	KdShaderManager::Instance().UndoBlendState();
}

void ResultScene::Release()
{
	if (m_spBGM && m_spBGM->IsPlaying())
	{
		m_spBGM->Stop();
	}
	m_spBGM = nullptr;
}

void ResultScene::DrawClearWindow()
{
	Math::Color color = { 1,1,1,1.0f };

	Math::Matrix scaleMat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transMat = Math::Matrix::CreateTranslation(0, 280, 0);
	Math::Matrix finalMat = scaleMat * transMat;

	KdShaderManager::Instance().m_spriteShader.SetMatrix(finalMat);
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_clearTex.get(), 0, 0, nullptr, &color);
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
}

void ResultScene::DrawRankWindow()
{
	if (!m_showRank || !m_currentRankTex) return;

	//ハンコが押されるような拡大縮小演出
	float scale = 0.7f + (1.0f - m_rankAlpha);

	Math::Matrix scaleMat = Math::Matrix::CreateScale(scale);
	Math::Matrix transMat = Math::Matrix::CreateTranslation(0, -90, 0);
	Math::Matrix finalMat = scaleMat * transMat;

	KdShaderManager::Instance().m_spriteShader.SetMatrix(finalMat);

	Math::Color color = { 1, 1, 1, m_rankAlpha };
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_currentRankTex.get(), 0, 0, nullptr, &color);

	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
}

void ResultScene::DrawMoveWindow()
{
	if (m_moveAlpha <= 0.0f)return;

	Math::Color color = { 1, 1, 1, m_moveAlpha };
	float posY = 80.0f;

	//player入れ替え回数の数字
	float numScale = 1.0f;
	std::string moveStr = std::to_string(m_playerMoves);
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::CreateScale(numScale));
	KdShaderManager::Instance().m_spriteShader.DrawFont(1, moveStr, { 0.0f / numScale, posY / numScale }, &color);
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);

	//player入れ替え回数のラベル
	KdShaderManager::Instance().m_spriteShader.DrawFont(2, "MOVES", { -100.0f / numScale, posY + 10.0f / numScale }, &color);

	float refY = posY - 90.0f;
	Math::Color refColor = { 1.0f, 1.0f, 1.0f, m_moveAlpha };
	float refScale = 1.0f;

	//製作者想定入れ替え回数の数字
	std::string refStr = std::to_string(m_parMoves);
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::CreateScale(numScale));
	KdShaderManager::Instance().m_spriteShader.DrawFont(1, refStr, { 0.0f / refScale, posY - 60 / refScale }, &color);
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);

	//製作者想定入れ替え回数のラベル
	KdShaderManager::Instance().m_spriteShader.DrawFont(2, "REF", { -100.0f / refScale, posY - 50.0f / refScale }, &color);
}

void ResultScene::CalculateRank()
{
	if (m_parMoves <= 0) m_parMoves = 10;//不正値の場合

	//プレイヤーの手数と想定手数(Par)の差分で判定
	int diff = m_playerMoves - m_parMoves;

	if (diff <= 0)
	{
		m_currentRankTex = m_rankSTex;//Sランク(想定以下)
	}
	else if (diff <= 2)
	{
		m_currentRankTex = m_rankATex;
	}
	else if (diff <= 5)
	{
		m_currentRankTex = m_rankBTex;
	}
	else
	{
		m_currentRankTex = m_rankCTex;
	}
}

void ResultScene::DrawRect(float x, float y, float w, float h, const Math::Color& color)
{
	if (!m_windowTex)return;

	KdShaderManager::Instance().m_spriteShader.DrawTex(m_windowTex.get(), x, y, w, h, nullptr, &color);

}

void ResultScene::DrawFrame(float x, float y, float w, float h, float thickness, const Math::Color& color)
{
	//上
	DrawRect(x, y + h / 2.0f, w + thickness, thickness, color);
	//下
	DrawRect(x, y - h / 2.0f, w + thickness, thickness, color);
	//左
	DrawRect(x - w / 2.0f, y, thickness, h, color);
	//右
	DrawRect(x + w / 2.0f, y, thickness, h, color);

}
