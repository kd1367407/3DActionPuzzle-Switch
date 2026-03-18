#include "main.h"
#include"../Application/Scene/SceneManager.h"
#include"../Application/Scene/BaseScene/BaseScene.h"
#include"../Framework/GameObject/ArchetypeManager.h"
#include"../Application/SettingsManager/SettingsManager.h"
#include"../Application/Scene/GameScene/GameScene.h"

//===================================================================
// エントリーポイント
//===================================================================
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// メモリリーク検出有効化
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// COM初期化
	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
	{
		CoUninitialize();
		return 0;
	}

	// 日本語対応
	setlocale(LC_ALL, "japanese");

	// アプリケーション実行
	Application::Instance().Execute();

	// COM解放
	CoUninitialize();

	return 0;
}

void Application::AddLog(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	m_debugGui.AddLogV(fmt, args);
	va_end(args);
}

void Application::KdBeginUpdate()
{
	// 入力状況の更新
	KdInputManager::Instance().Update();

	// ImGuiフレーム開始
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Application::KdPostUpdate()
{
	// 3DSoundListenerの行列を更新（カメラの逆行列を設定して音の定位を計算）
	KdAudioManager::Instance().SetListnerMatrix(KdShaderManager::Instance().GetCameraCB().mView.Invert());
}

void Application::KdBeginDraw(bool usePostProcess)
{
	// バックバッファクリア
	KdDirect3D::Instance().ClearBackBuffer();

	// ポストプロセスを使う場合は、そのためのテクスチャもクリア
	if (!usePostProcess) return;
	KdShaderManager::Instance().m_postProcessShader.Draw();
}

void Application::KdPostDraw()
{
	// クリエイトモードかつGameSceneのときだけデバッグGUIのロジックを呼び出す
	if (SceneManager::Instance().GetCurrentMode() == SceneManager::SceneMode::Create &&
		SceneManager::Instance().GetCurrentSceneType() == SceneManager::SceneType::Game)
	{
		if (auto* scene = SceneManager::Instance().GetCurrentScene())
		{
			m_debugGui.GuiProcess(*scene);
		}
	}

	// ImGuiの描画コマンドを生成し、レンダリング
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// フェード処理はUIより手前に描画
	SceneManager::Instance().GetFader().Draw();

	// BackBuffer -> 画面表示
	KdDirect3D::Instance().WorkSwapChain()->Present(0, 0);
}

bool Application::Init(int w, int h)
{
	// ウィンドウ作成
	if (m_window.Create(w, h, "3D GameProgramming", "Window") == false) {
		MessageBoxA(nullptr, "ウィンドウ作成に失敗", "エラー", MB_OK);
		return false;
	}

	// Direct3D初期化
	bool deviceDebugMode = false;
#ifdef _DEBUG
	deviceDebugMode = true;
#endif

	std::string errorMsg;
	if (KdDirect3D::Instance().Init(m_window.GetWndHandle(), w, h, deviceDebugMode, errorMsg) == false) {
		MessageBoxA(m_window.GetWndHandle(), errorMsg.c_str(), "Direct3D初期化失敗", MB_OK | MB_ICONSTOP);
		return false;
	}

	// 各種マネージャー初期化
	SettingsManager::Instance().Init();
	ArchetypeManager::Instance().Init();
	InitInput();

	// システム初期化
	KdShaderManager::Instance().Init();
	KdAudioManager::Instance().Init();

	//フルスクリーン
	m_window.SetBorderlessFullscreen(true);

	// フォント初期化
	KdFontManager::Instance().Init(GetWindowHandle());
	KdFontManager::Instance().AddFontResource("Asset/Font/NotoSansJPRegular.ttf");
	KdFontManager::Instance().AddFont(0, "ノートサンズ", 50);
	KdFontManager::Instance().AddFontResource("Asset/Font/Orbitron-Bold.ttf");
	KdFontManager::Instance().AddFont(1, "Orbitron", 64);
	KdFontManager::Instance().AddFont(2, "Orbitron", 24);

	// GUI & シーン初期化
	m_debugGui.GuiInit();
	SceneManager::Instance().Init();

	return true;
}

void Application::Execute()
{
	// 設定ファイルからウィンドウサイズ読み込み
	KdCSVData windowData("Asset/Data/WindowSettings.csv");
	const std::vector<std::string>& sizeData = windowData.GetLine(0);

	if (Application::Instance().Init(atoi(sizeData[0].c_str()), atoi(sizeData[1].c_str())) == false) {
		return;
	}

	// 時間管理初期化
	m_fpsController.Init();

	// メインループ
	while (true)
	{
		// FPS制御（デバッグ用ショートカット）
		if (GetAsyncKeyState(VK_F2) & 0x8000) m_fpsController.m_maxFps = 60;
		if (GetAsyncKeyState(VK_F3) & 0x8000) m_fpsController.m_maxFps = 30;
		if (GetAsyncKeyState(VK_F4) & 0x8000) m_fpsController.m_maxFps = 10;

		// DeltaTimeの更新
		m_fpsController.Update();
		m_deltaTime = m_fpsController.GetDeltaTime();

		if (m_endFlag) break;

		// ウィンドウメッセージ処理
		m_window.ProcessMessage();
		if (m_window.IsCreated() == false) break;

		// ESCキーでの終了確認
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			bool shouldQuit = true;
			if (auto scene = SceneManager::Instance().GetCurrentScene())
			{
				if (scene->HasUnsavedChanges())
				{
					if (MessageBoxA(m_window.GetWndHandle(), "保存されていない変更があります。\n本当に終了しますか？",
						"終了確認", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO)
					{
						shouldQuit = false;
					}
				}
			}
			if (shouldQuit) End();
		}

		// アプリケーション更新処理
		// PreUpdate(入力更新等) -> Update(ゲームロジック) -> PostUpdate(物理演算等) の順で実行
		KdBeginUpdate();
		{
			SceneManager::Instance().PreUpdate();
			SceneManager::Instance().Update();
			SceneManager::Instance().PostUpdate();
		}
		KdPostUpdate();

		// アプリケーション描画処理
		// PreDraw(定数バッファ更新) -> Draw(3D) -> PostDraw(ポストエフェクト) -> DrawSprite(2D UI) の順で実行
		KdBeginDraw();
		{
			SceneManager::Instance().PreDraw();
			SceneManager::Instance().Draw();
			SceneManager::Instance().PostDraw();
			SceneManager::Instance().DrawSprite();
		}
		KdPostDraw();

		// 指定FPSになるように待機
		m_fpsController.Wait();

		// タイトルバーにFPS表示
		std::string TitleBar = "Switch FPS:" + std::to_string(m_fpsController.m_nowfps);
		SetWindowTextA(m_window.GetWndHandle(), TitleBar.c_str());
	}

	Release();
}

void Application::Release()
{
	SceneManager::Instance().Release();
	m_debugGui.GuiRelease();
	KdInputManager::Instance().Release();
	KdShaderManager::Instance().Release();
	KdAudioManager::Instance().Release();
	KdDirect3D::Instance().Release();
	m_window.Release();
}

void Application::InitInput()
{
	const auto& inputSetting = SettingsManager::Instance().GetInputSetting();
	auto keyBordDeviceCollector = std::make_unique<KdInputCollector>();

	const std::map<std::string, int> keyMap = {
		{"VK_F1", VK_F1}, {"W", 'W'}, {"S", 'S'}, {"A", 'A'}, {"D", 'D'}, {"Z", 'Z'},
		{"VK_SPACE", VK_SPACE}, {"VK_LBUTTON", VK_LBUTTON}, {"VK_RBUTTON", VK_RBUTTON},
		{"VK_TAB",VK_TAB}, {"VK_DELETE",VK_DELETE}, {"VK_CONTROL",VK_CONTROL}
	};

	// キーボード設定読み込み
	if (inputSetting.contains("keyboard"))
	{
		for (const auto& setting : inputSetting["keyboard"])
		{
			std::string name = setting.value("name", "");
			std::string key = setting.value("key", "");

			if (!name.empty() && keyMap.count(key))
			{
				keyBordDeviceCollector->AddButton(name, std::make_shared<KdInputButtonForWindows>(keyMap.at(key)));
			}
		}
	}

	// マウス設定読み込み
	if (inputSetting.contains("mouse"))
	{
		for (const auto& setting : inputSetting["mouse"])
		{
			std::string name = setting.value("name", "");
			std::string button = setting.value("button", "");

			if (!name.empty() && keyMap.count(button))
			{
				keyBordDeviceCollector->AddButton(name, std::make_shared<KdInputButtonForWindows>(keyMap.at(button)));
			}
		}
	}

	KdInputManager::Instance().AddDevice("WindowsKeyBordDevice", keyBordDeviceCollector);
}