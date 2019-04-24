class ImageToggleButton : public Button
{
public:
	ImageToggleButton(const String& componentName, Image imageOff, Image imageOn) 
		: Button(componentName), imageOn(imageOn), imageOff(imageOff)
	{
		frameHeight = imageOff.getHeight();
		frameWidth = imageOff.getWidth();

		setClickingTogglesState (true);
	}

	void setValue(float value, bool notifyHost)
	{
		if (value > 0.0f)
		{
			setToggleState(true, notifyHost ? sendNotification : dontSendNotification);
		}
		else
		{
			setToggleState(false, notifyHost ? sendNotification : dontSendNotification);
		}
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
};
