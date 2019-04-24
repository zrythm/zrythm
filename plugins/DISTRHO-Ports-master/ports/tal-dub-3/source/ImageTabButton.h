class ImageTabButton : public Button, public Timer
{
public:
	ImageTabButton(const String& componentName, Image imageOff, Image imageOn)
		: Button(componentName), imageOn(imageOn), imageOff(imageOff)
	{
		frameHeight = imageOff.getHeight();
		frameWidth = imageOff.getWidth();

		setClickingTogglesState (false);
		setTriggeredOnMouseDown (true);

		maxDelay = 40000;

		//
		lastTimeInMs = 0;
		deltaTimeInMs = 0;
	}

    void timerCallback() override
    {
		lastTimeInMs = 0;
		deltaTimeInMs = 0;
		stopTimer();
		setToggleState(false, dontSendNotification);
		repaint();
	}

    juce::int64 setClick()
    {
		if (lastTimeInMs > 0)
		{
			deltaTimeInMs = (juce::int64)Time::currentTimeMillis() - lastTimeInMs;
		}
		lastTimeInMs = (juce::int64)Time::currentTimeMillis();
		startTimer(4000);
		setToggleState(true, dontSendNotification);
		repaint();
		return deltaTimeInMs;
	}

	void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
	{
		if (this->getToggleState())
		{
			g.drawImage(imageOn, 0, 0, frameWidth, frameHeight, 0, 0, frameWidth, frameHeight);
		}
		else
		{
			g.drawImage(imageOff, 0, 0, frameWidth, frameHeight, 0, 0, frameWidth, frameHeight);
		}
	}

private:
	Image imageOn;
	Image imageOff;
	int frameWidth, frameHeight;

	int maxDelay;

	juce::int64 lastTimeInMs;
	juce::int64 deltaTimeInMs;
};
