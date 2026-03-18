#include "KdDebugGUI.h"
#include "../../../Application/main.h"
#include"../../../Application//Scene/BaseScene/BaseScene.h"
#include"../../../Framework/Component/GameObject.h"
#include"../../../Framework/Component/ImGuiComponent/ImGuiComponent.h"
#include"../../../Framework/Component/CameraComponent/CameraComponent.h"

//Jsonのエイリアス
using json = nlohmann::json;

void KdDebugGUI::GuiInit()
{
	// 初期化済みなら動作させない
	if (m_uqLog) return;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// Setup Dear ImGui style
	// ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();//スタイルをクラシックに変更
	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(Application::Instance().GetWindowHandle());
	ImGui_ImplDX11_Init(
		KdDirect3D::Instance().WorkDev(), KdDirect3D::Instance().WorkDevContext());

#include "imgui/ja_glyph_ranges.h"
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig config;
	config.MergeMode = true;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.Fonts->AddFontDefault();
	// 日本語対応
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msgothic.ttc", 13.0f, &config, glyphRangesJapanese);

	m_uqLog = std::make_unique<ImGuiAppLog>();
}

void KdDebugGUI::GuiProcess(BaseScene& scene)
{
	// 初期化されてないなら動作させない
	if (!m_uqLog) return;

	//ログウィンドウ
	m_uqLog->Draw("Log");
}

void KdDebugGUI::AddLog(const char* fmt, ...)
{
	// 初期化されてないなら動作させない
	if (!m_uqLog) return;

	va_list args;
	va_start(args, fmt);
	m_uqLog->AddLog(fmt, args); // 内部のImGuiAppLogのAddLogに処理を転送
	va_end(args);
}

void KdDebugGUI::AddLogV(const char* fmt, va_list args)
{
	if (!m_uqLog)return;
	m_uqLog->AddLogV(fmt, args);
}

void KdDebugGUI::ClearLog()
{
	if (m_uqLog)
	{
		m_uqLog->Clear();
	}
}

void KdDebugGUI::GuiRelease()
{
	if (!m_uqLog)return;

	m_uqLog.reset();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}