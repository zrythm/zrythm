class FilmStripKnob : public Slider
{
public:
	FilmStripKnob(Image image, const int numFrames, const bool stripIsHorizontal, int index)
		:   Slider(juce::String(index)),
		filmStrip(image),
		numFrames_(numFrames),
		isHorizontal_(stripIsHorizontal)
	{
		if(filmStrip.isValid())
		{
			setTextBoxStyle(NoTextBox, 0, 0, 0);
			setSliderStyle(RotaryVerticalDrag);
			frameHeight = filmStrip.getHeight() / numFrames_;
			frameWidth = filmStrip.getWidth();
			setRange(0.0f, 1.0f, 0.001f);
		}
	}

	void paint(Graphics& g) override
	{
		if(filmStrip.isValid()) {
			int value = (int)((getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFrames_ - 1));
			if(isHorizontal_) {
				g.drawImage(filmStrip, 0, 0, getWidth(), getHeight(),
					value * frameWidth, 0, frameWidth, frameHeight);
			} else {
				g.drawImage(filmStrip, 0, 0, getWidth(), getHeight(),
					0, value * frameHeight, frameWidth, frameHeight);
			}
		}
	}

private:
	Image filmStrip;
	const int numFrames_;
	const bool isHorizontal_;
	int frameWidth, frameHeight;
};
