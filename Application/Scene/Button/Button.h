#pragma once

class Button
{
public:
	Button(
		const Math::Vector2& pos,
		std::shared_ptr<KdTexture> tex,
		int fontNo,
		const std::string& label,
		std::function<void()> onClickAction,
		float	textScale=1.0f,
		float buttonScale = 1.0f,
		const Math::Color& textColor=kWhiteColor,
		int	 antiAliasing = 0
	);

	void Update();

	void Draw(float baseAlpha);

	bool IsHovered() const { return m_isOver; }

private:
	Math::Vector2 m_pos;
	Math::Vector2 m_size;
	std::shared_ptr<KdTexture> m_tex;
	Math::Rectangle m_rect;
	std::function<void()> m_onClick;//クリック時に実行する関数
	bool m_isOver;
	float m_alpha;
	std::string m_label;
	int m_fontNo;
	float m_textScale;
	Math::Color m_textColor;
	int m_antiAliasing;
	float m_buttonScale;
};