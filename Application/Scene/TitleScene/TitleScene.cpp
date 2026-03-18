#include "TitleScene.h"
#include"../SceneManager.h"
#include"../GameScene/GameManager/GameManager.h"
#include"../Src/Framework/Component/GameObject.h"
#include"../Src/Framework/Component/BackgroundComponent/BackgroundComponent.h"
#include"../../main.h"
#include"../../SettingsManager/SettingsManager.h"
#include"../Button/Button.h"
#include"../Src/Framework/Component/CameraComponent/SceneCameraComponent/SceneCameraComponent.h"
#include"../Src/Framework/Component/SkydomeComponent/SkydomeComponent.h"
#include"../Src/Framework/Component/TransformComponent/TransformComponent.h"

void TitleScene::Init()
{
	SceneManager::Instance().SetMode(SceneManager::SceneMode::UI);

	//ボタン画像
	m_knobTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/bar_round_gloss_large_square.png");
	m_buttonTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/bar_square_gloss_large.png");
	m_sliderTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/bar_round_gloss_small.png");
	m_windowTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/white.png");
	m_titleLogoTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/UI/TitleLogo.png");

	//背景
	auto sky = std::make_shared<GameObject>();
	sky->AddComponent<SkydomeComponent>();
	sky->AddComponent<TransformComponent>();
	sky->Init();
	AddObject(sky);

	//カメラ
	auto cameraObj = std::make_shared<GameObject>();
	auto cameraComp = cameraObj->AddComponent<SceneCameraComponent>();
	cameraObj->AddComponent<TransformComponent>();
	cameraObj->GetComponent<TransformComponent>()->SetPos({ 0.0f, 5.0f, -10.0f });
	cameraObj->GetComponent<TransformComponent>()->SetRot({ 20.0f, 0.0f, 0.0f });
	cameraObj->Init();
	AddObject(cameraObj);
	SetActiveCamera(cameraComp);

	m_spBGM = KdAudioManager::Instance().Play("Asset/Sound/TitleBGM.wav", true, 1.0f);
	m_animTime = 0.0f;

	//--ボタン類構築--
	int fontNo = 2;
	Math::Color textColor = { 0.8f, 0.9f, 1.0f, 1.0f }; // サイバーっぽい青白
	float textScale = 1.2f;
	float buttonScale = 2.0f;

	//--通常メニュー--
	m_buttons[MenuState::Main].emplace_back(Math::Vector2(0, -30), m_buttonTex, fontNo, "PLAY",
		[this]() {
			GameManager::Instance().SetLoadMode(GameManager::LoadMode::Play);
			SceneManager::Instance().ChangeScene(SceneManager::SceneType::StageSelect);
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::Main].emplace_back(Math::Vector2(0, -130), m_buttonTex, fontNo, "CREATE",
		[this]() {
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);
			m_currentState = MenuState::Create;
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::Main].emplace_back(Math::Vector2(0, -230), m_buttonTex, fontNo, "OPTION",
		[this]() {
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);
			m_currentState = MenuState::Volume;
		}, textScale, buttonScale, textColor
	);

	//--ステージ作成メニュー--
	m_buttons[MenuState::Create].emplace_back(Math::Vector2(0, -50), m_buttonTex, fontNo, "EDIT STAGE",
		[this]() {
			GameManager::Instance().SetLoadMode(GameManager::LoadMode::Edit);
			SceneManager::Instance().ChangeScene(SceneManager::SceneType::StageSelect);
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::Create].emplace_back(Math::Vector2(0, -130), m_buttonTex, fontNo, "NEW STAGE",
		[this]() {
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);
			m_currentState = MenuState::NewStage;
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::Create].emplace_back(Math::Vector2(0, -200), m_buttonTex, fontNo, "BACK",
		[this]() {
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);
			m_currentState = MenuState::Main;
		}, textScale, buttonScale, textColor
	);

	//--ステージ土台メニュー--
	m_buttons[MenuState::NewStage].emplace_back(Math::Vector2(0, -30), m_buttonTex, fontNo, "BASE:FLAT",
		[this]() {
			auto& gm = GameManager::Instance();
			gm.SetLoadMode(GameManager::LoadMode::CreateNew);
			gm.SetNextStage("Asset/Data/Stages/Template01.json", "New Stage");
			SceneManager::Instance().SetMode(SceneManager::SceneMode::Create);
			SceneManager::Instance().ChangeScene(SceneManager::SceneType::Game);
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::NewStage].emplace_back(Math::Vector2(0, -80), m_buttonTex, fontNo, "BASE:ROOM",
		[this]() {
			auto& gm = GameManager::Instance();
			gm.SetLoadMode(GameManager::LoadMode::CreateNew);
			gm.SetNextStage("Asset/Data/Stages/Template02.json", "New Stage");
			SceneManager::Instance().SetMode(SceneManager::SceneMode::Create);
			SceneManager::Instance().ChangeScene(SceneManager::SceneType::Game);
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::NewStage].emplace_back(Math::Vector2(0, -130), m_buttonTex, fontNo, "BASE:TOWER",
		[this]() {
			auto& gm = GameManager::Instance();
			gm.SetLoadMode(GameManager::LoadMode::CreateNew);
			gm.SetNextStage("Asset/Data/Stages/Template03.json", "New Stage");
			SceneManager::Instance().SetMode(SceneManager::SceneMode::Create);
			SceneManager::Instance().ChangeScene(SceneManager::SceneType::Game);
		}, textScale, buttonScale, textColor
	);

	m_buttons[MenuState::NewStage].emplace_back(Math::Vector2(0, -250), m_buttonTex, fontNo, "BACK",
		[this]() {
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);
			m_currentState = MenuState::Create;
		}, textScale, buttonScale, textColor
	);

	//--sound--
	m_sliders[MenuState::Volume].emplace_back(Math::Vector2(0, -50), m_sliderTex, m_knobTex,
		KdAudioManager::Instance().GetBGMVolume(),
		[this](float newVolume) { KdAudioManager::Instance().SetBGMVolume(newVolume); }
	);
	m_sliders[MenuState::Volume].emplace_back(Math::Vector2(0, -150), m_sliderTex, m_knobTex,
		KdAudioManager::Instance().GetSEVolume(),
		[this](float newVolume) { KdAudioManager::Instance().SetSEVolume(newVolume); }
	);

	m_buttons[MenuState::Volume].emplace_back(Math::Vector2(0, -250), m_buttonTex, fontNo, "BACK",
		[this]() {
			KdAudioManager::Instance().Play("Asset/Sound/UIButton.wav", false, 1.0f);

			//設定保存
			nlohmann::json& settings = SettingsManager::Instance().WorkGameSetting();
			settings["volume_settings"]["SE"] = KdAudioManager::Instance().GetSEVolume();
			settings["volume_settings"]["BGM"] = KdAudioManager::Instance().GetBGMVolume();
			SettingsManager::Instance().SaveGameSetting();

			m_currentState = MenuState::Main;
		}, textScale, buttonScale, textColor
	);

	m_currentState = MenuState::Main;
}

void TitleScene::SceneUpdate()
{
	float deltatime = Application::Instance().GetDeltaTime();
	m_animTime += Application::Instance().GetDeltaTime();
	float fadeSpeed = 1.0f;
	auto& fader = SceneManager::Instance().GetFader();

	//タイトルロゴのフェードイン
	if (m_titleAlpha < 1.0f)
	{
		if (!fader.IsFadeing())
		{
			m_titleAlpha += fadeSpeed * deltatime;
		}
	}

	//タイトルがある程度表示されたらボタンをフェードイン
	if (m_titleAlpha >= 0.5f && m_buttonAlpha < 1.0f)
	{
		m_buttonAlpha += fadeSpeed * deltatime;
	}

	m_titleAlpha = std::min(m_titleAlpha, 1.0f);
	m_buttonAlpha = std::min(m_buttonAlpha, 1.0f);

	//カメラを原点中心にゆっくり回転させる演出
	if (auto camera = GetActiveCamera())
	{
		m_cameraAngle += 10.0f * Application::Instance().GetDeltaTime();

		//Y軸回転(自転) -> Z軸手前に配置 -> X軸回転(見下ろし)
		Math::Matrix rotY = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_cameraAngle));
		Math::Matrix trans = Math::Matrix::CreateTranslation(0, 5.0f, -15.0f);
		Math::Matrix rotX = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(15.0f));

		Math::Matrix worldMat = rotX * trans * rotY;
		camera->GetCamera()->SetCameraMatrix(worldMat);
	}

	for (auto& button : m_buttons[m_currentState])
	{
		button.Update();
	}

	if (m_currentState == MenuState::Volume)
	{
		for (auto& slider : m_sliders[m_currentState])
		{
			slider.Update();
		}
	}
}

void TitleScene::DrawSprite()
{
	BaseScene::DrawSprite();

	//--ウィンドウの枠描画--
	float winX = 0;
	float winY = -120;
	float winW = 350;
	float winH = 350;

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


	DrawTitleWindow();

	for (auto& button : m_buttons[m_currentState])
	{
		button.Draw(m_buttonAlpha);
	}

	if (m_currentState == MenuState::Volume)
	{
		KdShaderManager::Instance().m_spriteShader.DrawFont(2, "BGM", { -150, -60 }, &kWhiteColor);
		KdShaderManager::Instance().m_spriteShader.DrawFont(2, "SE", { -150, -160 }, &kWhiteColor);

		for (auto& slider : m_sliders[m_currentState])
		{
			slider.Draw(1.0f);
		}
	}
}

void TitleScene::Release()
{
	if (m_spBGM && m_spBGM->IsPlaying())
	{
		m_spBGM->Stop();
	}
	m_spBGM = nullptr;
}

void TitleScene::DrawTitleWindow()
{
	//-1~1のサイン波を0~1に変換して明滅させる
	float blink = (sin(m_animTime * 2.0f) + 1.0f) * 0.5f;
	float alpha = 0.8f + (blink * 0.2f);

	Math::Color color = { 1.0f, 1.0f, 1.0f, m_titleAlpha * alpha };

	KdShaderManager::Instance().m_spriteShader.DrawTex(
		m_titleLogoTex.get(),
		0, 250,
		600, 300,
		nullptr,
		&color
	);
}

void TitleScene::DrawRect(float x, float y, float w, float h, const Math::Color& color)
{
	if (!m_windowTex)return;

	KdShaderManager::Instance().m_spriteShader.DrawTex(m_windowTex.get(), x, y, w, h, nullptr, &color);

}

void TitleScene::DrawFrame(float x, float y, float w, float h, float thickness, const Math::Color& color)
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
