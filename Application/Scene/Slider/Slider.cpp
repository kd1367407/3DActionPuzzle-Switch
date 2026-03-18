#include "Slider.h"
#include"../../main.h"
#include"../../UIHelpers/UIHelpers.h"

Slider::Slider(const Math::Vector2& pos, std::shared_ptr<KdTexture> backTex, std::shared_ptr<KdTexture> knobTex, float initialValue, std::function<void(float)> onValueChanged) :
	m_pos(pos), m_backTex(backTex), m_knobTex(knobTex), m_value(initialValue), m_onValueChanged(onValueChanged)
{
	if (m_backTex)
	{
		m_size = {
			(float)m_backTex->GetInfo().Width,
			(float)m_backTex->GetInfo().Height
		};
	}
}

void Slider::Update()
{
	float screenWidth = KdDirect3D::Instance().GetBackBuffer()->GetInfo().Width;
	float screenHeight = KdDirect3D::Instance().GetBackBuffer()->GetInfo().Height;

	m_rect = CalculateRectFromWorld(m_pos, m_size, screenWidth, screenHeight);

	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(Application::Instance().GetWindowHandle(), &mousePos);

	bool isOver = m_rect.Contains(static_cast<long>(mousePos.x), static_cast<long>(mousePos.y));

	if (isOver && KdInputManager::Instance().IsHold("Select"))
	{
		//マウスのX座標が矩形の左端から何ピクセル目かを計算
		float mouseRelX = static_cast<float>(mousePos.x - m_rect.x);

		//矩形の幅で割ることで、左端を0.0、右端を1.0とする割合を算出
		float newValue = mouseRelX / m_rect.width;

		newValue = std::max(0.0f, std::min(1.0f, newValue));

		if (m_value != newValue)
		{
			m_value = newValue;

			if (m_onValueChanged)
			{
				m_onValueChanged(m_value);
			}
		}
	}
}

void Slider::Draw(float baseAlpha)
{
	if (m_backTex)
	{
		Math::Color color = { 1,1,1, 1 };
		KdShaderManager::Instance().m_spriteShader.DrawTex(m_backTex.get(), m_pos.x, m_pos.y, nullptr, &color);

		//現在の値(0.0～1.0)に基づいてつまみの描画位置を計算
		float knobWidth = (float)m_knobTex->GetInfo().Width;
		float barLeftX = m_pos.x - (m_size.x / 2);
		float knobCenterX = barLeftX + (m_size.x * m_value);

		KdShaderManager::Instance().m_spriteShader.DrawTex(m_knobTex.get(), knobCenterX, m_pos.y, nullptr, &color);
	}
}