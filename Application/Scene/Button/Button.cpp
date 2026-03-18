#include "Button.h"
#include"../../main.h"
#include"../../UIHelpers/UIHelpers.h"

Button::Button(const Math::Vector2& pos, std::shared_ptr<KdTexture> tex, int fontNo, const std::string& label, std::function<void()> onClickAction, float textScale, float buttonScale, const Math::Color& textColor, int antiAliasing) :
	m_pos(pos), m_tex(tex), m_fontNo(fontNo), m_textScale(textScale), m_buttonScale(buttonScale), m_label(label), m_textColor(textColor), m_onClick(onClickAction), m_antiAliasing(antiAliasing), m_isOver(false), m_alpha(0.8f)
{
	if (m_tex)
	{
		m_size = {
			(float)m_tex->GetInfo().Width * m_buttonScale,
			(float)m_tex->GetInfo().Height * m_buttonScale
		};
	}
}

void Button::Update()
{
	float screenWidth = KdDirect3D::Instance().GetBackBuffer()->GetInfo().Width;
	float screenHeight = KdDirect3D::Instance().GetBackBuffer()->GetInfo().Height;

	//中心座標とサイズから、当たり判定用の矩形(Rect)を計算
	m_rect = CalculateRectFromWorld(m_pos, m_size, screenWidth, screenHeight);

	//マウス座標を取得し、スクリーン座標からウィンドウ内のクライアント座標へ変換
	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(Application::Instance().GetWindowHandle(), &mousePos);

	//当たり判定(矩形内にマウスがあるか)
	m_isOver = m_rect.Contains(static_cast<long>(mousePos.x), static_cast<long>(mousePos.y));

	if (m_isOver)
	{
		m_alpha = 1.0f;
	}
	else
	{
		m_alpha = 0.8f;
	}

	//マウスが重なっていて、かつ決定キーが押されたらコールバック実行
	if (m_isOver && KdInputManager::Instance().IsPress("Select"))
	{
		if (m_onClick)
		{
			m_onClick();
		}
	}
}

void Button::Draw(float baseAlpha)
{
	if (m_tex)
	{
		//マウスオーバー時は明るく表示
		float brightness = m_isOver ? 1.0f : 0.8f;
		Math::Color color = { brightness, brightness, brightness, m_alpha * baseAlpha };
		KdShaderManager::Instance().m_spriteShader.DrawTex(m_tex.get(), m_pos.x, m_pos.y, m_size.x, m_size.y, nullptr, &color);
	}

	//文字列描画
	if (m_fontNo >= 0 && !m_label.empty())
	{
		Math::Color finalTextColor = m_textColor;
		finalTextColor.w = m_alpha * baseAlpha;

		//文字列の矩形サイズを計測
		Math::Vector2 labelSize = KdFontManager::Instance().MeasureString(m_fontNo, m_label, m_antiAliasing);

		//スケールを考慮した幅と高さを計算
		float scaleWidth = labelSize.x * m_textScale;
		float scaleHeight = labelSize.y * m_textScale;

		//ボタンの中心にテキストが来るように座標計算(ボタン座標 - テキストサイズ半分)
		float textPosX = m_pos.x - (scaleWidth / 2.0f);
		float textPosY = m_pos.y - (scaleHeight / 2.0f);

		//シェーダーに行列をセットして文字サイズを変更
		KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::CreateScale(m_textScale));

		//行列で拡大縮小すると座標位置もズレるため、描画座標を逆スケールで割って位置を補正する
		KdShaderManager::Instance().m_spriteShader.DrawFont(m_fontNo, m_label, Math::Vector2(textPosX / m_textScale, textPosY / m_textScale), &finalTextColor, m_antiAliasing);

		//行列リセット
		KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::CreateScale(1));
	}
}