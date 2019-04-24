#ifndef TOMATLLOOKANDFEEL_H_INCLUDED
#define TOMATLLOOKANDFEEL_H_INCLUDED

#include "JuceHeader.h"

class TomatlLookAndFeel : public LookAndFeel_V3
{
private:
	const size_t mSliderWidth;
public:
	enum TomatlColorIds
	{
		defaultBackground = 0x2001111,
		defaultText,
		defaultBorder,
		controlBackground,
		activeBackground,
		toggledBackground,
		disabledText,
		backgroundText,
		alternativeText1,
		alternativeText2,
		sliderBackground,
	};

	TomatlLookAndFeel() : mSliderWidth(12)
	{
		setColour(defaultBackground, Colour::fromString("FF1E1E1E"));
		setColour(alternativeText1, Colour::fromString("FF4DC796"));
		setColour(alternativeText2, Colour::fromString("FF0E95DA"));
		setColour(defaultText, Colours::lightgrey);
		setColour(defaultBorder, Colour::fromString("FF535353"));
		setColour(controlBackground, Colour::fromString("FF111111"));
		setColour(activeBackground, findColour(controlBackground).brighter(0.05));
		setColour(sliderBackground, findColour(activeBackground));
		setColour(toggledBackground, Colours::green);
		setColour(disabledText, Colours::lightgrey);
		setColour(backgroundText, Colours::grey);

		setColour(Label::textColourId, findColour(defaultText));
		setColour(ToggleButton::textColourId, findColour(defaultText));
		
		setColour(TextButton::textColourOffId, findColour(defaultText));
		setColour(TextButton::textColourOnId, findColour(defaultText));
		setColour(HyperlinkButton::textColourId, findColour(alternativeText2));

		setColour(ComboBox::backgroundColourId, findColour(controlBackground));
		setColour(ComboBox::textColourId, findColour(defaultText));
		setColour(ComboBox::arrowColourId, findColour(defaultText));
		setColour(ComboBox::outlineColourId, findColour(defaultBorder));
		setColour(ComboBox::buttonColourId, findColour(defaultBorder));

		setColour(PopupMenu::backgroundColourId, findColour(defaultBackground));
		setColour(PopupMenu::textColourId, findColour(defaultText));
		setColour(PopupMenu::highlightedBackgroundColourId, findColour(defaultBorder));

		setColour(GroupComponent::textColourId, findColour(defaultText));
		setColour(GroupComponent::outlineColourId, findColour(defaultBorder));
	}

	virtual void drawTickBox(Graphics& g, Component& c, float x, float y, float w, float h,
		bool ticked, bool isEnabled, bool isMouseOverButton, bool isButtonDown)
	{
		Image img = ImageCache::getFromMemory(BinaryData::check_check_png, BinaryData::check_check_pngSize);

		int l = 0;
		int t = (h - img.getHeight());

		juce::Rectangle<int> rect = juce::Rectangle<int>(x + l, y + t, img.getWidth(), img.getHeight());

		g.setColour(findColour(defaultBackground));
		g.fillRect(rect);
		g.setColour(findColour(defaultBorder));
		g.drawRect(rect);

		if (ticked) g.drawImageAt(img, x + l, y + t);
	}

	virtual void drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour,
		bool isMouseOverButton, bool isButtonDown)
	{
		g.setColour(findColour(defaultBorder));
		g.drawRect(b.getLocalBounds());

		if (isButtonDown)
		{
			g.setColour(findColour(toggledBackground));
		}
		else if (isMouseOverButton)
		{
			g.setColour(findColour(activeBackground));
		}
		else
		{
			g.setColour(findColour(controlBackground));
		}

		g.fillRect(b.getLocalBounds().expanded(-1));
	}

	void drawVerticalSliderHandle(Graphics& g, Slider& s, int x, int y, int w, int h)
	{
		w += 4;
		h = 8;

		// Let's pretend this never happened...
		if (s.getSliderStyle() == Slider::TwoValueHorizontal || s.getSliderStyle() == Slider::LinearHorizontal)
		{
			std::swap(w, h);
		}

		/*const float sliderRadius = (float)(getSliderThumbRadius(slider) - 2);
		const float outlineThickness = slider.isEnabled() ? 0.8f : 0.3f;

		drawGlassSphere(g,
			kx - sliderRadius,
			ky - sliderRadius,
			sliderRadius * 2.0f,
			knobColour, outlineThickness);

		return;*/
		juce::Rectangle<float> area;
		area.setLeft(x - w / 2);
		area.setWidth(w);
		area.setTop(y - h / 2);
		area.setHeight(h);

		//juce::Rectangle<float> area2;
		//area2.setLeft(area.getTopLeft().getX(), )

		g.setColour(findColour(s.isEnabled() ? alternativeText1 : defaultBorder));
		g.drawRect(area);

		if (w > h)
		{
			area.reduce(2, (area.getHeight() - 2) / 2);
			
		}
		else
		{
			area.reduce((area.getWidth() - 2) / 2, 2);
		}

		g.drawRect(area);

		//g.drawRoundedRectangle(area, 2, 1.5f);
		//g.setColour(findColour(s.isEnabled() ? alternativeText2 : defaultBorder).withAlpha(0.7f));
		//g.fillRoundedRectangle(area.reduced(1.5f, 1.5f), 2);
	}

	virtual void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
		float sliderPos, float minSliderPos, float maxSliderPos,
		const Slider::SliderStyle style, Slider& s)
	{
		if (style == Slider::LinearVertical)
		{
			drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
			drawVerticalSliderHandle(g, s, x + (width - x) / 2, sliderPos, mSliderWidth, s.isEnabled() ? mSliderWidth : 1);
		}
		else if (style == Slider::TwoValueVertical)
		{
			drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
			drawVerticalSliderHandle(g, s, x + (width - x) / 2, minSliderPos, mSliderWidth, s.isEnabled() ? mSliderWidth : 1);
			drawVerticalSliderHandle(g, s, x + (width - x) / 2, maxSliderPos, mSliderWidth, s.isEnabled() ? mSliderWidth : 1);

		}
		else if (style == Slider::TwoValueHorizontal)
		{
			drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
			drawVerticalSliderHandle(g, s, minSliderPos, y + (height - y) / 2, mSliderWidth, s.isEnabled() ? mSliderWidth : 1);
			drawVerticalSliderHandle(g, s, maxSliderPos, y + (height - y) / 2, mSliderWidth, s.isEnabled() ? mSliderWidth : 1);
		}
		else if (style == Slider::LinearHorizontal)
		{
			drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
			drawVerticalSliderHandle(g, s, sliderPos, y + (height - y) / 2, mSliderWidth, s.isEnabled() ? mSliderWidth : 1);
		}
		else
		{
			LookAndFeel_V3::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
		}
	}

	virtual void drawLinearSliderBackground(Graphics& g, int x, int y, int width, int height,
		float sliderPos, float minSliderPos, float maxSliderPos,
		const Slider::SliderStyle style, Slider& s)
	{
		if (style == Slider::LinearVertical || style == Slider::TwoValueVertical)
		{
			juce::Rectangle<int> area;
			area.setLeft((width - mSliderWidth) / 2);
			area.setTop(s.getLocalBounds().getTopLeft().getY());
			area.setWidth(mSliderWidth);
			area.setBottom(s.getLocalBounds().getBottomLeft().getY());

			juce::Rectangle<int> selectedArea;

			selectedArea.setLeft(area.getTopLeft().getX());
			selectedArea.setTop(style == Slider::TwoValueVertical ? maxSliderPos : sliderPos);
			selectedArea.setWidth(mSliderWidth);
			selectedArea.setBottom(style == Slider::TwoValueVertical ? minSliderPos : area.getBottom());

			selectedArea.expand(-1, -1);
			
			g.setColour(findColour(sliderBackground));
			g.fillRect(area);
			g.setColour(findColour(defaultBorder));
			g.drawRect(area);

			if (selectedArea.getHeight() > 0)
			{
				g.setColour(findColour(controlBackground));
				g.fillRect(selectedArea);
			}
		}
		else if (style == Slider::LinearHorizontal || style == Slider::TwoValueHorizontal)
		{
			juce::Rectangle<int> area;
			area.setLeft(s.getLocalBounds().getTopLeft().getX());
			area.setTop(y + height / 2 - mSliderWidth / 2);
			//area.setWidth(width);
			area.setRight(s.getLocalBounds().getTopRight().getX());
			area.setHeight(mSliderWidth);

			juce::Rectangle<int> selectedArea;

			selectedArea.setLeft(style == Slider::TwoValueHorizontal ? minSliderPos : area.getTopLeft().getX());
			selectedArea.setTop(area.getTopLeft().getY());
			selectedArea.setRight(style == Slider::TwoValueHorizontal ? maxSliderPos : sliderPos);
			selectedArea.setBottom(area.getBottom());

			selectedArea.expand(-1, -1);

			g.setColour(findColour(sliderBackground));
			g.fillRect(area);
			g.setColour(findColour(defaultBorder));
			g.drawRect(area);

			if (selectedArea.getHeight() > 0)
			{
				g.setColour(findColour(controlBackground));
				g.fillRect(selectedArea);
			}
		}
		else
		{
			LookAndFeel_V3::drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, s);
		}
	}

	/*virtual void drawDrawableButton(Graphics& g, DrawableButton& b, bool isMouseOverButton, bool isButtonDown)
	{
		g.setColour(findColour(defaultBorder));
		g.drawRect(b.getLocalBounds());
	}*/
};



#endif  // TOMATLLOOKANDFEEL_H_INCLUDED
