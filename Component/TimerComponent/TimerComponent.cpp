#include "TimerComponent.h"
#include"../Src/Application/main.h"
#include"../GameObject.h"
#include"../TransformComponent/TransformComponent.h"
#include"../Src/Application/Scene/SceneManager.h"
#include"../Src/Application/Scene/GameScene/GameManager/GameManager.h"

void TimerComponent::Awake()
{
	m_numTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/Scene/Number.png");
	m_colonTex = KdAssets::Instance().m_textures.GetData("Asset/Textures/Scene/neonColon.png");
}

void TimerComponent::Start()
{
	if (m_owner)
	{
		m_wpTransform = m_owner->GetComponent<TransformComponent>();
	}
}

void TimerComponent::Update()
{
	if (SceneManager::Instance().GetCurrentMode() != SceneManager::SceneMode::Game)
	{
		return;
	}

	float deltaTime = Application::Instance().GetDeltaTime();
	auto& fader = SceneManager::Instance().GetFader();

	if (!fader.IsFadeing())
	{
		if (m_isTimerActive)
		{
			m_elapsedTime += deltaTime;
		}
	}
}

void TimerComponent::DrawSprite()
{
	if (GameManager::Instance().GetLoadMode() != GameManager::LoadMode::Play)return;

	if (!m_numTex || !m_colonTex)return;

	//現在の画面サイズを取得
	float screenW = static_cast<float>(KdDirect3D::Instance().GetBackBuffer()->GetWidth());
	float screenH = static_cast<float>(KdDirect3D::Instance().GetBackBuffer()->GetHeight());

	//タイマーのレイアウト設定
	const float charWidth = 64.0f;// 1文字の幅
	const float charHeight = 100.0f;// 1文字の高さ
	const float totalTimerWidth = charWidth * 5.0f; // 全体の幅（6文字 + コロン2つ = 8個分）
	const float marginX = 20.0f;// 右端からの余白
	const float marginY = 20.0f;// 上端からの余白

	//--描画基準位置を計算--
	//右端
	float screenRightEdge = screenW * 0.5f;
	//上端
	float screenTopEdge = screenH * 0.5f;

	Math::Vector3 drawPos = {};

	if (m_useScreenAutoPos)
	{
		//X座標：右端 - 余白 - (全体の幅) + (文字幅の半分:中心合わせ用補正)
		drawPos.x = screenRightEdge - marginX - totalTimerWidth + (charWidth * 0.5f)-10.0f;
		//Y座標：上端 - 余白 - (文字高さの半分)
		drawPos.y = screenTopEdge - marginY - (charHeight * 0.5f);
	}
	else
	{
		if (auto spTransform = m_wpTransform.lock())
		{
			drawPos = spTransform->GetPos();
		}
	}

	drawPos.z = 0.0f;

	//経過時間を分/秒/ミリ秒に分解して表示
	int totalSeconds = static_cast<int>(m_elapsedTime);
	int minutes = totalSeconds / 60;//分
	int seconds = totalSeconds % 60;//秒
	int milliseconds = static_cast<int>((m_elapsedTime - totalSeconds) * 100);//ミリ秒(2桁)

	//画像の1文字幅に合わせた配置間隔 (64pxずつずらす)

	//分
	DrawNumber(minutes / 10, drawPos.x, drawPos.y);
	DrawNumber(minutes % 10, drawPos.x + charWidth, drawPos.y);

	//コロン1(文字幅の2倍の位置)
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_colonTex, drawPos.x + (charWidth * 2), drawPos.y - 8.0f);

	//秒(文字幅の3倍、4倍の位置)
	DrawNumber(seconds / 10, drawPos.x + (charWidth * 3), drawPos.y);
	DrawNumber(seconds % 10, drawPos.x + (charWidth * 4), drawPos.y);

	//コロン2(文字幅の5倍の位置)
	//KdShaderManager::Instance().m_spriteShader.DrawTex(m_colonTex, drawPos.x + (charWidth * 5), drawPos.y - 8.0f);

	////ミリ秒(文字幅の6倍、7倍の位置)
	//DrawNumber(milliseconds / 10, drawPos.x + (charWidth * 6), drawPos.y);
	//DrawNumber(milliseconds % 10, drawPos.x + (charWidth * 7), drawPos.y);
}

bool TimerComponent::UpdateCountUp(float deltatime)
{
	if (!m_isCountUp)return false;

	m_elapsedTime += m_countUpSpeed * deltatime;

	if (m_elapsedTime >= m_targetTime)
	{
		m_elapsedTime = m_targetTime;
		m_isCountUp = false;
		return false;//カウントアップ完了
	}
	return true;
}

void TimerComponent::StartCountUp(float targetTime)
{
	m_targetTime = targetTime;
	m_elapsedTime = 0.0f;
	m_isCountUp = true;

	//ターゲット時間に合わせてカウントアップ速度を調整(どんなタイムでも約1.5秒でカウントアップが終わるようにする)
	if (m_targetTime > 0.0f)
	{
		float animationDirection = 1.5f;
		m_countUpSpeed = m_targetTime / animationDirection;
	}
	else
	{
		m_countUpSpeed = 1.0f;
	}
}

void TimerComponent::DrawNumber(int number, float x, float y)
{
	if (number < 0 || number>9)return;

	const float	numTexWidth = 64.0f;  
	const float	numTexHeight = 100.0f;

	//数字に応じたテクスチャの切り出し範囲(Rect)を計算
	Math::Rectangle srcRect;
	srcRect.x = numTexWidth * number;
	srcRect.y = 0;
	srcRect.width = numTexWidth;
	srcRect.height = numTexHeight;

	//描画
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_numTex, x, y, numTexWidth, numTexHeight, &srcRect);
}