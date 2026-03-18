#include "BackgroundComponent.h"
#include"../Src/Application/main.h"

void BackgroundComponent::Awake()
{
	m_normalTex1 = KdAssets::Instance().m_textures.GetData("Asset/Textures/grid4.png");
	m_normalTex2 = KdAssets::Instance().m_textures.GetData("Asset/Textures/grid3.png");
}

void BackgroundComponent::Start()
{
}

void BackgroundComponent::Update()
{
	float deltaTime = Application::Instance().GetDeltaTime();

	//UVスクロール処理。2枚のテクスチャを異なる速度でスクロールさせて奥行き感を出す
	m_uvOffset1.x += 0.02f * deltaTime;
	m_uvOffset2.y += 0.005f * deltaTime;

	//数値が大きくなりすぎないようにループさせる
	if (m_uvOffset1.x > 0.5f) m_uvOffset1.x -= 0.5f;
	if (m_uvOffset2.y > 0.5f) m_uvOffset2.y -= 0.5f;
}

void BackgroundComponent::DrawSprite()
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	//グリッド描画モードを有効化(専用のシェーダー処理を使用)
	spriteShader.SetGridEnable(true);

	//スクロール用のUVオフセット値をシェーダーに転送
	spriteShader.SetGridUVOffset(m_uvOffset1, m_uvOffset2);

	//UVのタイリング(繰り返し回数)を設定
	spriteShader.SetUVTiling({ 3,3 });

	//グリッド用の法線マップ画像を転送
	KdShaderManager::Instance().m_spriteShader.SetGridNomalTexture(*m_normalTex1, *m_normalTex2);

	//テクスチャの端をループさせるためにサンプラー状態をWrapに変更
	KdShaderManager::Instance().ChangeSamplerState(KdSamplerState::Point_Wrap);

	//画面全体を覆うように描画
	spriteShader.DrawTex(
		m_normalTex1.get(), 0, 0,
		(int)KdDirect3D::Instance().GetBackBuffer()->GetWidth(),
		(int)KdDirect3D::Instance().GetBackBuffer()->GetHeight()
	);

	//サンプラー状態を元に戻す
	KdShaderManager::Instance().UndoSamplerState();

	//設定をリセット
	spriteShader.SetUVTiling({ 1,1 });
	spriteShader.SetGridEnable(false);
}