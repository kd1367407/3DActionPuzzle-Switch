#include "MousePointerComponent.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/JsonHelper/JsonHelper.h"

void MousePointerComponent::Awake()
{

}

void MousePointerComponent::Start()
{
}

void MousePointerComponent::DrawSprite()
{
	//UIモードや編集モードではシステムカーソルが表示されるため、ゲームモード中のみカスタムカーソルを描画する
	if (SceneManager::Instance().GetCurrentMode() != SceneManager::SceneMode::Game)return;
	if (!m_spTex)return;

	KdShaderManager::Instance().m_spriteShader.DrawTex(m_spTex, 0, 0);
}

void MousePointerComponent::Configure(const nlohmann::json& data)
{
	if (data.is_null() || !data.contains("MousePointerComponent"))return;

	const auto& colData = data.at("MousePointerComponent");

	m_texturePath = JsonHelper::GetString(colData, "texture");
	m_spTex = KdAssets::Instance().m_textures.GetData(m_texturePath);
}

nlohmann::json MousePointerComponent::ToJson() const
{
	nlohmann::json j;
	j["texture"] = m_texturePath;

	return j;
}