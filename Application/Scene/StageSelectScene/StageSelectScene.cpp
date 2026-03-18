#include "StageSelectScene.h"
#include"../SceneManager.h"
#include"../GameScene/GameManager/GameManager.h"
#include"../../JsonHelper/JsonHelper.h"
#include"../GameScene/GameScene.h"
#include"../../GameLogic/StageModel/StageModel.h"
#include"../../GameViewModel.h"
#include"../Src/Framework/Component/TransformComponent/TransformComponent.h"
#include"../Src/Framework/Component/CameraComponent/CameraComponent.h"
#include"../Src/Framework/Component/GameObject.h"
#include"../../main.h"
#include"../Button/Button.h"
#include"../Src/Framework/Component/SkydomeComponent/SkydomeComponent.h"
#include"../Src/Framework/Component/CameraComponent/SceneCameraComponent/SceneCameraComponent.h"

using json = nlohmann::json;

void StageSelectScene::Init()
{
	SceneManager::Instance().SetMode(SceneManager::SceneMode::UI);
	m_currentView = ViewMode::Main;

	//テクスチャ
	m_windowTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/white.png");
	m_buttonTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/bar_square_gloss_large.png");

	//カメラ
	auto cameraObj = std::make_shared<GameObject>();
	auto cameraComp = cameraObj->AddComponent<SceneCameraComponent>();
	cameraObj->AddComponent<TransformComponent>();
	cameraObj->GetComponent<TransformComponent>()->SetPos({ 0.0f, 5.0f, -10.0f });
	cameraObj->GetComponent<TransformComponent>()->SetRot({ 20.0f, 0.0f, 0.0f });
	cameraObj->Init();
	AddObject(cameraObj);
	SetActiveCamera(cameraComp);

	//背景
	auto sky = std::make_shared<GameObject>();
	sky->AddComponent<SkydomeComponent>();
	sky->AddComponent<TransformComponent>();
	sky->Init();
	AddObject(sky);


	m_spBGM = KdAudioManager::Instance().Play("Asset/Sound/TitleBGM.wav", true, 1.0f);

	//jsonロード
	std::ifstream ifs1("Asset/Data/Stages/StageList.json");
	if (ifs1.is_open())
	{
		ifs1 >> m_stageListData;
	}

	std::ifstream ifs2("Asset/Data/Stages/TutorialList.json");
	if (ifs2.is_open())
	{
		ifs2 >> m_tutorialStageListData;
	}

	//ボタン構築
	RebuildStageButtons();

	m_buttonAlpha = 0.0f;
}

void StageSelectScene::SceneUpdate()
{
	float deltatime = Application::Instance().GetDeltaTime();
	auto& fader = SceneManager::Instance().GetFader();

	if (m_buttonAlpha < 1.0f && !fader.IsFadeing())
	{
		m_buttonAlpha += 2.0f * deltatime;
	}
	m_buttonAlpha = std::min(m_buttonAlpha, 1.0f);

	for (auto& info : m_stageButtons) info.btn->Update();
	for (auto& btn : m_systemButtons) btn->Update();

	if (m_needRebuild)
	{
		RebuildStageButtons();
		m_needRebuild = false;
	}
}

void StageSelectScene::DrawSprite()
{
	BaseScene::DrawSprite();

	//ボタン描画
	{
		KdShaderManager::Instance().m_spriteShader.Begin();

		//画面全体に薄いグリッドを敷く
		//Math::Color gridColor = { 0.0f, 1.0f, 1.0f, 0.2f }; //薄いシアン
		//float gridSize = 50.0f;

		////縦線
		//for (float x = -640; x < 640; x += gridSize) {
		//	DrawRect(x, 0, 1.0f, 720.0f, gridColor);
		//}
		////横線
		//for (float y = -360; y < 360; y += gridSize) {
		//	DrawRect(0, y, 1280.0f, 1.0f, gridColor);
		//}

		//--ウィンドウの枠描画--
		float winX = 0;
		float winY = 0;
		float winW = 1100;
		float winH = 650;

		//背景（半透明の黒）
		DrawRect(winX, winY, winW, winH, { 0.0f, 0.0f, 0.1f, 0.8f * m_buttonAlpha });

		//枠線（発光しているようなシアン）
		DrawFrame(winX, winY, winW, winH, 4.0f, { 0.0f, 1.0f, 1.0f, m_buttonAlpha });

		//四隅の飾り
		float decoSize = 20.0f;
		float decoThick = 8.0f;

		//左上だけ例示（他も同様に計算）
		DrawRect(winX - winW / 2, winY + winH / 2, decoSize, decoThick, { 0, 1, 1, m_buttonAlpha }); // 横
		DrawRect(winX - winW / 2, winY + winH / 2, decoThick, decoSize, { 0, 1, 1, m_buttonAlpha }); // 縦

		//ステージボタン描画
		for (auto& info : m_stageButtons)
		{
			info.btn->Draw(m_buttonAlpha);
		}

		//システムボタン描画
		for (auto& btns : m_systemButtons)
		{
			btns->Draw(m_buttonAlpha);
		}

		//プレビュー画像生成には3D描画(DrawLit等)が含まれるため、一度2D描画を終了してGPUの状態を整える
		KdShaderManager::Instance().m_spriteShader.End();
	}

	//--プレビュー生成、描画--
	std::string hoveringStagePath = "";
	std::string hoveringStageName = "";

	//ホバー判定
	for (auto& info : m_stageButtons)
	{
		if (info.btn->IsHovered())
		{
			hoveringStagePath = info.stagePath;
			hoveringStageName = info.stageName;
			break;
		}
	}

	//プレビュー画像生成、表示
	if (!hoveringStagePath.empty())
	{
		GenerateStagePreview(hoveringStagePath);

		//生成後は2D描画に戻る
		KdShaderManager::Instance().m_spriteShader.Begin();

		//右側に描画
		float previewX = 200.0f;
		float previewY = 50.0f;
		float previewW = 480.0f;
		float previewH = 270.0f;

		//プレビュー枠
		if (m_windowTex)
		{
			Math::Color frameColor = { 0.5f, 0.8f, 1.0f, m_buttonAlpha };
			KdShaderManager::Instance().m_spriteShader.DrawTex(
				m_windowTex.get(), previewX, previewY, previewW + 20, previewH + 20, nullptr, &frameColor
			);
		}

		//プレビュー画像
		if (m_stagePreviews[hoveringStagePath])
		{
			//アルファ値を考慮して描画
			Math::Color color = { 1.0f, 1.0f, 1.0f, m_buttonAlpha };

			KdShaderManager::Instance().m_spriteShader.DrawTex(
				m_stagePreviews[hoveringStagePath].get(),
				previewX, previewY, previewW, previewH, nullptr, &color
			);
		}

		//ステージ名
		Math::Color textColor = { 1.0f, 1.0f, 1.0f, m_buttonAlpha };
		float textScale = 1.5f;
		Math::Vector2 textPos = { (previewX - 70) / textScale,(previewY + 160) / textScale };
		KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::CreateScale(textScale));
		KdShaderManager::Instance().m_spriteShader.DrawFont(2, hoveringStageName, textPos, &textColor);
		KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);

		KdShaderManager::Instance().m_spriteShader.End();
	}
}

void StageSelectScene::Release()
{
	if (m_spBGM && m_spBGM->IsPlaying())
	{
		m_spBGM->Stop();
	}
	m_spBGM = nullptr;
}

void StageSelectScene::DrawRect(float x, float y, float w, float h, const Math::Color& color)
{
	if (!m_windowTex)return;

	KdShaderManager::Instance().m_spriteShader.DrawTex(m_windowTex.get(), x, y, w, h, nullptr, &color);
}

void StageSelectScene::DrawFrame(float x, float y, float w, float h, float thickness, const Math::Color& color)
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

void StageSelectScene::RebuildStageButtons()
{
	//リストクリア
	m_stageButtons.clear();
	m_systemButtons.clear();

	//モードに応じて対象のjsonデータの参照取得
	const nlohmann::json* currentList = nullptr;

	if (m_currentView == ViewMode::Main)
	{
		currentList = &m_stageListData;
	}
	else
	{
		currentList = &m_tutorialStageListData;
	}

	//データが不正なら終了
	if (!currentList || !currentList->contains("stages") || !(*currentList)["stages"].is_array()) return;

	const auto& stages = (*currentList)["stages"];
	int totalStages = static_cast<int>(stages.size());

	//現在のページに基づいて、配列内のデータの開始インデックスと終了インデックスを計算
	int startIndex = m_currentPage * ITEMS_PER_PAGE;
	int endIndex = std::min(startIndex + ITEMS_PER_PAGE, totalStages);

	//レイアウト調整
	float startX = -300.0f;
	float startY = 150.0f;
	float stepY = 80.0f;

	//スタイル設定
	int fontNo = 2;
	Math::Color textColor = { 0.8f, 0.9f, 1.0f, 1.0f };
	float btnScale = 2.0f;

	//計算した範囲内(ページ内)のデータだけをループしてボタン化
	for (int i = startIndex; i < endIndex; ++i)
	{
		const auto& stageInfo = stages[i];
		std::string label = JsonHelper::GetString(stageInfo, "label", "Unknown");
		std::string path = JsonHelper::GetString(stageInfo, "path");

		Math::Vector2 btnPos = { startX, startY - (i - startIndex) * stepY };

		//ボタン生成
		auto btn = std::make_shared<Button>(
			btnPos, m_buttonTex, fontNo, label,
			[this, path, label]() {
				auto& gm = GameManager::Instance();
				gm.SetNextStage(path, label);
				if (gm.GetLoadMode() == GameManager::LoadMode::Play) {
					SceneManager::Instance().SetMode(SceneManager::SceneMode::Game);
				}
				else {
					SceneManager::Instance().SetMode(SceneManager::SceneMode::Create);
				}
				SceneManager::Instance().ChangeScene(SceneManager::SceneType::Game);
			}, 1.0f, btnScale, textColor
		);

		m_stageButtons.push_back({ btn,path,label });
	}

	float sysBtnY = -250.0f;

	//前へ(1ページ目以外なら描画)
	if (m_currentPage > 0)
	{
		auto preBtn = std::make_shared<Button>(
			Math::Vector2(startX - 80.0f, sysBtnY), m_buttonTex, fontNo, "<",
			[this]() { m_currentPage--; m_needRebuild = true; KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f); },
			1.0f, btnScale - 0.5f, textColor // 小さめ
		);
		m_systemButtons.push_back(preBtn);
	}

	//次へ(次のページがあるなら描画)
	if (endIndex < totalStages)
	{
		auto nextBtn = std::make_shared<Button>(
			Math::Vector2(startX + 80.0f, sysBtnY), m_buttonTex, fontNo, ">",
			[this]() { m_currentPage++; m_needRebuild = true; KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f); },
			1.0f, btnScale - 0.5f, textColor
		);
		m_systemButtons.push_back(nextBtn);
	}

	//戻る
	auto backBtn = std::make_shared<Button>(
		Math::Vector2(400.0f, -250.0f), m_buttonTex, fontNo, "TITLE",
		[]() { SceneManager::Instance().ChangeScene(SceneManager::SceneType::Title); },
		1.0f, btnScale, textColor
	);
	m_systemButtons.push_back(backBtn);


	// モード切替 (Tutorial <-> Main)
	std::string switchLabel = (m_currentView == ViewMode::Main) ? "TUTORIALS" : "MAIN STAGES";
	auto switchBtn = std::make_shared<Button>(
		Math::Vector2(startX, startY + 100.0f), m_buttonTex, fontNo, switchLabel,
		[this]() {
			m_currentView = (m_currentView == ViewMode::Main) ? ViewMode::Tutorial : ViewMode::Main;
			m_currentPage = 0;
			m_needRebuild = true;
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);
		}, 1.0f, btnScale, textColor
	);
	m_systemButtons.push_back(switchBtn);
}

void StageSelectScene::GenerateStagePreview(const std::string& stagePath)
{
	//既にキャッシュ済みなら生成しない
	if (m_stagePreviews.count(stagePath))return;

	//レンダーターゲット(描画先テクスチャ)作成
	const int previewWidth = 320;
	const int previewHeight = 180;
	KdRenderTargetPack previewRT;
	previewRT.CreateRenderTarget(previewWidth, previewHeight, true);


	if (!previewRT.m_RTTexture)
	{
		m_stagePreviews[stagePath] = nullptr; //失敗したことを記録
		return;
	}

	//一時的にGameSceneとViewModelを生成し、対象ステージを読み込ませる
	auto tempModel = std::make_shared<StageModel>();
	tempModel->Init();
	auto tempScene = std::make_unique<GameScene>();
	auto tempViewModel = std::make_shared<GameViewModel>(tempModel, nullptr, tempScene.get());
	tempViewModel->LoadStage(stagePath);

	//オブジェクトの位置や行列を確定させるために1回更新する
	tempScene->PreUpdate();
	tempScene->Update();
	tempScene->PostUpdate();

	//プレビューカメラがステージ内に設定されてない場合のデフォルトカメラ行列
	Math::Matrix cameraMat = Math::Matrix::CreateLookAt(
		Math::Vector3(10.0f, 10.0f, -15.0f),//カメラの位置
		Math::Vector3::Zero,//見つめる先の座標（原点）
		Math::Vector3::Up//カメラの上方向
	).Invert();

	//ステージ内にPreviewCameraオブジェクトがあれば、その視点を使用する
	auto previewPoint = tempScene->FindObject("PreviewCamera");
	if (previewPoint)
	{
		if (auto pointTransform = previewPoint->GetComponent<TransformComponent>())
		{
			cameraMat = pointTransform->GetMatrix();
		}
	}

	//シーンに撮影用カメラを追加
	auto cameraObj = std::make_shared<GameObject>();
	auto cameraComp = cameraObj->AddComponent<CameraComponent>();
	cameraObj->AddComponent<TransformComponent>();
	tempScene->AddObject(cameraObj);
	cameraObj->Init();
	cameraComp->GetCamera()->SetCameraMatrix(cameraMat);
	tempScene->SetActiveCamera(cameraComp);

	//レンダーターゲットに切り替えて、1フレームだけシーンを描画(サムネイル撮影)
	{
		KdRenderTargetChanger rtChanger;
		rtChanger.ChangeRenderTarget(previewRT);
		previewRT.ClearTexture(Math::Color(1.0f, 0.0f, 1.0f, 1.0f)); //背景色を設定
		tempScene->PreDraw();
		tempScene->Draw();
		rtChanger.UndoRenderTarget();
	}

	//撮影したレンダーターゲットをテクスチャとしてキャッシュに保存
	m_stagePreviews[stagePath] = previewRT.m_RTTexture;

	//ObjectListを解放するため明示的にRelease
	tempScene->Release();
}