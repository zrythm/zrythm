class TalMeter : public Component, public Timer
{
public:

	// Curently only supports vertical sliders
	TalMeter(TalCore* const ownerFilter, int totalWith, int totalHeight, int verticalDistance)
		: Component("TalMeter")
	{
		ledWidth = 2;
		valueL = valueR = 0.0f;
		pad = totalHeight / 4;

		this->totalWith = totalWith;
		this->totalHeight = totalHeight;
		this->verticalDistance = verticalDistance;

		ledColour = ledColour.fromRGBA(
			(const juce::uint8)60,
			(const juce::uint8)60,
			(const juce::uint8)255,
			(const juce::uint8)255);

        startTimer(50);
		this->ownerFilter = ownerFilter;
	}

    void timerCallback() override
    {
		bool mustRepaint = false;
		float* newLevels = ownerFilter->getPeakReductionValue();
		newLevels[0] = fabsf(newLevels[0]);
		newLevels[1] = fabsf(newLevels[1]);

		// TODO: better code
		if (newLevels[0] > valueL)
        {
			if (newLevels[0] > 1.0f) newLevels[0] = 1.0f;

            valueL = newLevels[0];
            mustRepaint = true;
        }
		else
		{
			float oldValue = valueL;
			valueL -= 0.02f;
			if (valueL >= 0.0f)
			{
            mustRepaint = true;
			}
			else if (oldValue != 0.0f)
			{
				valueL = 0.0f;
				mustRepaint = true;
			}
		}
		if (newLevels[1] > valueR)
        {
			if (newLevels[1] > 1.0f) newLevels[1] = 1.0f;

            valueR = newLevels[1];
            mustRepaint = true;
        }
		else
		{
			float oldValue = valueL;
			valueR -= 0.02f;
			if (valueR >= 0.0f)
			{
				mustRepaint = true;
			}
			else if (oldValue != 0.0f)
			{
				valueR = 0.0f;
				mustRepaint = true;
			}
		}
		if (mustRepaint)
		{
			repaint();
		}
    }

	void paint(Graphics& g) override
	{
		g.setColour(Colour((const juce::uint8)0, (const juce::uint8)0, (const juce::uint8)0));
		g.fillRoundedRectangle(0.0f, 1.0f, (float)totalWith, (float)totalHeight, 5.0f);
		g.fillRoundedRectangle(0.0f, (float)verticalDistance, (float)totalWith, (float)totalHeight, 5.0f);

		g.setColour(Colour((const juce::uint8)166, (const juce::uint8)166, (const juce::uint8)166));
		g.drawRoundedRectangle(0.0f, 1.0f, (float)totalWith, (float)totalHeight, 5.0f, 2.0f);
		g.drawRoundedRectangle(0.0f, (float)verticalDistance, (float)totalWith, (float)totalHeight, 5.0f, 2.0f);

		int intCurrentMeterValue = (int)(valueL * (totalWith - pad * 2));
		for (int i = 0; i < intCurrentMeterValue; i += ledWidth * 2)
		{
			drawGlowLed(g, pad + i, pad + 1, totalHeight - 2 * pad);
		}
		intCurrentMeterValue = (int)(valueR * (totalWith - pad * 2));
		for (int i = 0; i < intCurrentMeterValue; i += ledWidth * 2)
		{
			drawGlowLed(g, pad + i, pad + verticalDistance, totalHeight - 2 * pad);
		}
	}

	void drawGlowLed(Graphics& g, int x, int y, int ledHeight)
	{
		g.setColour(ledColour);
		g.fillRect(x, y, ledWidth, ledHeight);
		g.setOpacity(0.03f);

		const float radius = 5;
		for (int i = 0; i < radius; i++)
		{
			g.fillRoundedRectangle(x - i * 2.0f - 6.0f, y - i * 2.0f, i * 4.0f + 14.0f, ledHeight + i * 4.0f, radius);
		}
	}

private:
	int totalWith;
	int totalHeight;
	int verticalDistance;

	int ledWidth;
	int pad;

	Colour ledColour;

	float valueL, valueR;
	TalCore* ownerFilter;
};
