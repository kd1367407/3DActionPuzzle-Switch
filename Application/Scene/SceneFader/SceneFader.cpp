#include "SceneFader.h"
#include"../../main.h"
#include "Framework/KdFramework.h"

void SceneFader::Init()
{
	m_currentAlpha = 1.0f;
	m_state = FadeState::Idle;
}

void SceneFader::Update()
{
	float deltatime = Application::Instance().GetDeltaTime();

	if (m_state == FadeState::FadingOut)
	{
		m_currentAlpha += m_fadeSpeed * deltatime;
		if (m_currentAlpha >= 1.0f)
		{
			m_currentAlpha = 1.0f;
			m_state = FadeState::Idle;
		}
	}
	else if (m_state == FadeState::FadingIn)
	{
		m_currentAlpha -= m_fadeSpeed * deltatime;
		if (m_currentAlpha <= 0.0f)
		{
			m_currentAlpha = 0.0f;
			m_state = FadeState::Idle;
		}
	}
}

void SceneFader::Draw()
{
	if (!IsFadeing()) return;

	//画面遷移用のワイプ演出描画
	KdShaderManager::Instance().m_postProcessShader.DrawWipe(m_currentAlpha);
}