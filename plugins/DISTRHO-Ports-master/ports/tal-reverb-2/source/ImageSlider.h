class ImageSlider : public Slider
{
public:

	// Curently only supports vertical sliders
	ImageSlider(Image image, const int length)
		: Slider("Bitmap Slider"),
		imageKnob(image)
	{
		this->length = length;

		setTextBoxStyle(NoTextBox, 0, 0, 0);
		setSliderStyle(LinearVertical);

		frameHeight = imageKnob.getHeight();
		frameWidth = imageKnob.getWidth();

		setRange(0.0f, 1.0f, 0.001f);

		customValueText = false;
		valueTextHeight = 10;

		this->setSliderSnapsToMousePosition(false);
	}

	void paint(Graphics& g) override
	{
		double value = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
		g.drawImage(imageKnob, 0, (1.0f - value) * length, frameWidth, frameHeight,
			0, 0, frameWidth, frameHeight);

		g.setColour(Colour((const juce::uint8)10, (const juce::uint8)10, (const juce::uint8)10));
		g.fillRoundedRectangle(8, length + frameHeight - 14, getWidth() - 16, valueTextHeight + 4, 6.0f);

		g.setColour(Colour((const juce::uint8)200, (const juce::uint8)200, (const juce::uint8)200));

		g.setFont(12.0f);
		if (!customValueText)
		{
			valueText = juce::String(getValue(), 2); //  << T("dB");
			g.drawText(valueText, 0, length + frameHeight - 14 + 2, getWidth(), valueTextHeight, juce::Justification::centred, false);
		}
		else
		{
			g.drawText(valueText, 0, length + frameHeight - 14 + 2, getWidth(), valueTextHeight, juce::Justification::centred, false);
		}
	}

	void setTextValue(juce::String valueText)
	{
		this->valueText = valueText;
		customValueText = true;
	}

private:
	Image imageKnob;
	int length;
	int frameWidth, frameHeight;

	juce::String valueText;
	bool customValueText;
	int valueTextHeight;
};
