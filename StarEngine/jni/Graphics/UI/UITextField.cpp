#include "UITextField.h"
#include "../../Components/Graphics/TextComponent.h"

namespace star
{
	UITextField::UITextField(
		const tstring & name,
		const tstring & text,
		const tstring & fontName,
		const Color & color
		)
		: UIElement(name)
	{
		m_pTextComponent = 
			new TextComponent(fontName);
		
		AddComponent(m_pTextComponent);

		m_pTextComponent->SetColor(color);
		m_pTextComponent->SetText(text);
		m_pTextComponent->SetHUDOptionEnabled(true);
	}

	UITextField::UITextField(
		const tstring & name,
		const tstring & text,
		const tstring & fontName,
		const tstring & fontPath,
		uint32 fontSize,
		const Color & color
		)
		: UIElement(name)
	{
		m_pTextComponent = 
			new TextComponent(fontPath, fontName, fontSize);
		
		AddComponent(m_pTextComponent);

		m_pTextComponent->SetColor(color);
		m_pTextComponent->SetText(text);
	}

	UITextField::~UITextField(void)
	{

	}

	void UITextField::Initialize()
	{
		UIElement::Initialize();
	}
	
	void UITextField::SetHorizontalAlignment(
		HorizontalAlignment alignment,
		bool redefineCenter
		)
	{
		if(redefineCenter)
		{
			switch(alignment)
			{
				case HorizontalAlignment::Left:
					GetTransform()->SetCenterX(0);
					m_pTextComponent->AlignTextLeft();
					break;
				case HorizontalAlignment::Center:
					GetTransform()->SetCenterX(
						float32(m_pTextComponent->GetWidth()) / 2.0f
						);
					m_pTextComponent->AlignTextCenter();
					break;
				case HorizontalAlignment::Right:
					GetTransform()->SetCenterX(
						float32(m_pTextComponent->GetWidth())
						);
					m_pTextComponent->AlignTextRight();
					break;
			}
		}

		UIElement::SetHorizontalAlignment(
			alignment,
			redefineCenter
			);
	}

	void UITextField::SetVerticalAlignment(
		VerticalAlignment alignment,
		bool redefineCenter
		)
	{
		if(redefineCenter)
		{
			switch(alignment)
			{
				case VerticalAlignment::Bottom:
					GetTransform()->SetCenterY(0);
					break;
				case VerticalAlignment::Center:
					GetTransform()->SetCenterY(
						float32(m_pTextComponent->GetHeight()) / 2.0f
						);
					break;
				case VerticalAlignment::Top:
					GetTransform()->SetCenterY(
						float32(m_pTextComponent->GetHeight())
						);
					break;
			}
		}

		UIElement::SetVerticalAlignment(
			alignment,
			redefineCenter
			);
	}

	void UITextField::SetText(const tstring & text)
	{
		m_pTextComponent->SetText(text);
	}

	const tstring & UITextField::GetText() const
	{
		return m_pTextComponent->GetText();
	}

	/// <summary>
	/// Sets the color.
	/// </summary>
	/// <param name="color">The color.</param>
	void UITextField::SetColor(const Color & color)
	{
		m_pTextComponent->SetColor(color);
	}

	vec2 UITextField::GetDimensions() const
	{
		return vec2(GetWidth(), GetHeight());
	}

	int32 UITextField::GetWidth() const
	{
		return m_pTextComponent->GetWidth();
	}

	int32 UITextField::GetHeight() const
	{
		return m_pTextComponent->GetHeight();
	}
}
