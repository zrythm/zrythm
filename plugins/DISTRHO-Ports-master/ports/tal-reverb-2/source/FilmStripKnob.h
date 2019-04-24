class FilmStripKnob : public Slider
{
public:
	FilmStripKnob(Image image, const int numFrames, const bool stripIsHorizontal)
		:   Slider("Film Strip Slider"),
		filmStrip(image),
		numFrames_(numFrames),
		isHorizontal_(stripIsHorizontal)
	{
		setTextBoxStyle(NoTextBox, 0, 0, 0);
		setSliderStyle(RotaryVerticalDrag);
		frameHeight = filmStrip.getHeight() / numFrames_;
		frameWidth = filmStrip.getWidth();
		setRange(0.0f, 1.0f, 0.001f);
		customValueText = false;
		valueTextHeight = 10;
	}

	void paint(Graphics& g) override
	{
		int value = (getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFrames_ - 1);
		if(isHorizontal_) {
			g.drawImage(filmStrip, 8, 0, getWidth() - 16, getHeight() - valueTextHeight * 2,
				value * frameWidth, 0, frameWidth, frameHeight);
		} else {
			g.drawImage(filmStrip, 8, 0, getWidth() - 16, getHeight() - valueTextHeight * 2,
				0, value * frameHeight, frameWidth, frameHeight);
		}

		g.setColour(Colour((const juce::uint8)10, (const juce::uint8)10, (const juce::uint8)10));
		g.fillRoundedRectangle(0, getWidth() + 6 - 16, getWidth(), valueTextHeight + 4, 6.0f);

		g.setColour(Colour((const juce::uint8)200, (const juce::uint8)200, (const juce::uint8)200));

		g.setFont(12.0f);
		if (!customValueText)
		{
			valueText = juce::String(getValue(), 2); //  << T("dB");
			g.drawText(valueText, 0, getWidth() + 8 - 16, getWidth(), valueTextHeight, juce::Justification::centred, false);
		}
		else
		{
			g.drawText(valueText, 0, getWidth() + 8 - 16, getWidth(), valueTextHeight, juce::Justification::centred, false);
		}
	}

	void setTextValue(juce::String valueText)
	{
		this->valueText = valueText;
		customValueText = true;
	}

private:
	Image filmStrip;
	const int numFrames_;
	const bool isHorizontal_;
	int frameWidth, frameHeight;

	juce::String valueText;
	bool customValueText;
	int valueTextHeight;
};
