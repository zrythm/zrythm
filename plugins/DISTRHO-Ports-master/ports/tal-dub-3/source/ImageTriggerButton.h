class ImageTriggerButton : public Button
{
public:
	ImageTriggerButton(const String& componentName, Image* imageOff, Image* imageOn)
		: Button(componentName), imageOn(imageOn), imageOff(imageOff)
	{
		frameHeight = imageOff->getHeight();
		frameWidth = imageOff->getWidth();

		setClickingTogglesState (true);
		setTriggeredOnMouseDown (true);
	}

	void setTriggeredOnMouseDown (const bool isTriggeredOnMouseDown)
	{
		if (isTriggeredOnMouseDown)
		{
			this->setToggleState(true, dontSendNotification);
			repaint();
		}
		else
		{
			this->setToggleState(false, dontSendNotification);
			repaint();
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
	Image* imageOn;
	Image* imageOff;
	int frameWidth, frameHeight;
};
