#include "GuiLookAndFeel.h"

GuiLookAndFeel::GuiLookAndFeel()
{

	chickenKnob = ImageCache::getFromMemory(BinaryData::chicken_300_png, BinaryData::chicken_300_pngSize);
	masterKnob = ImageCache::getFromMemory(BinaryData::trim_480_png, BinaryData::trim_480_pngSize);
	toggleOff = ImageCache::getFromMemory(BinaryData::air_button_off_png, BinaryData::air_button_off_pngSize);
	toggleOn = ImageCache::getFromMemory(BinaryData::air_button_on_png, BinaryData::air_button_on_pngSize);
	switchOn = ImageCache::getFromMemory(BinaryData::kippschalter_on_png, BinaryData::kippschalter_on_pngSize);
	switchOff = ImageCache::getFromMemory(BinaryData::kippschalter_off_png, BinaryData::kippschalter_off_pngSize);

	setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
	setColour(TextButton::buttonColourId, Colours::black);
	setColour(TextButton::buttonOnColourId, Colours::black);
	setColour(ComboBox::buttonColourId, Colours::darkgrey);
	setColour(Slider::trackColourId, Colours::black);
	setColour(Slider::rotarySliderFillColourId, Colours::black);
	setColour(Slider::rotarySliderOutlineColourId, Colours::black);
	setColour(Slider::textBoxBackgroundColourId, Colours::white.withAlpha(0.5f));
	setColour(Slider::textBoxHighlightColourId, Colours::lightgrey);

	numChickenImages = chickenKnob.getHeight() / chickenKnob.getWidth();
	numTrimImages = masterKnob.getHeight() / masterKnob.getWidth();;
}

void GuiLookAndFeel::drawTickBox (Graphics& g,
                                  Component& /*component*/,
                                  float x, float y, float w, float h,
                                  bool ticked,
                                  bool isEnabled,
                                  bool /*isMouseOverButton*/,
                                  bool /*isButtonDown*/)
{
	const bool isSwitch = h > 20;
	Image img(isSwitch ? (ticked ? switchOn : switchOff) : (ticked ? toggleOn : toggleOff));

	g.setColour(Colours::black.withAlpha(isEnabled ? 1.f : 0.5f));
	g.drawImageAt(img, int(x+w*0.5f)-img.getWidth()/2, int(y+h*0.5f)-img.getHeight()/2);
}


void GuiLookAndFeel::drawToggleButton (Graphics& g,
                                       ToggleButton& button,
                                       bool /*isMouseOverButton*/,
                                       bool /*isButtonDown*/)
{
	//const int w = button.getWidth();
	const int h = button.getHeight();
	const bool ticked = button.getToggleState();

	const bool isSwitch = h > 25;
	Image img(isSwitch ? (ticked ? switchOn : switchOff) : (ticked ? toggleOn : toggleOff));

	g.setColour(Colours::black.withAlpha(button.isEnabled() ? 1.f : 0.5f));
	g.drawImageAt(img, 0, 0);
}

void GuiLookAndFeel::drawRotarySlider (Graphics& g,
                       int x, int y,
                       int width, int height,
                       float sliderPos,
                       float /*rotaryStartAngle*/,
                       float /*rotaryEndAngle*/,
                       Slider& /*slider*/)
{
	const float radius = jmin(width, height)*0.5f;
	const float centreX = height > width ? x + width * 0.5f : x + radius + 1.f;
	const float centreY = height > width ? y + radius + 1.f : y + height*0.5f;
	const float rw = radius * 2.0f;

	g.setColour(Colours::black);

	if (rw > 89 && rw < 91 && numChickenImages > 0)
	{
		const int idx = jlimit(0, numChickenImages-1, int(sliderPos * (numChickenImages-1)));
		const int kw = chickenKnob.getWidth();
		const int kh = kw;
		const int offset = 0;
		const Rectangle<int> rect(0, offset + idx*kh, kw, kw);
		
		Image subImg(chickenKnob.getClippedImage(rect));

		g.drawImageAt(subImg, int(centreX-radius), int(centreY-radius), false);
	}
	else if (rw > 41 && rw < 43)
	{
		const int idx = jlimit(0, numTrimImages-1, int(sliderPos * (numTrimImages-1)));
		const int kw = masterKnob.getWidth();
		const int kh = kw;
		const int offset = 0;
		const Rectangle<int> rect(0, offset + idx*kh, kw, kw);
		
		Image subImg(masterKnob.getClippedImage(rect));

		g.drawImageAt(subImg, int(centreX-radius), int(centreY-radius), false);
	}
	else 
	{
		jassertfalse;
	}

}

// ================================================================================

GuiSlider::GuiSlider (const String& componentName) 
: Slider(componentName), 
  resetValue(0) 
{		
	//editor.setInputRestrictions(3, "0123456789.");
	//editor.addListener(this);
	addListener(this);
	label.addListener(this);
	label.setFont(Font(14.f));
	label.setEditable(false, true, false);	
	label.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
	label.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
	label.setColour(TextEditor::shadowColourId, Colours::transparentBlack);
	setRotaryParameters(0, 270*float_Pi/180.f, true);
	addAndMakeVisible(&label);
	setLabelText();
}

GuiSlider::GuiSlider (SliderStyle style, TextEntryBoxPosition textBoxPosition) 
: Slider(style, textBoxPosition), 
  resetValue(0) 
{
}

void GuiSlider::resized()
{
	Slider::resized();

	const int w = getWidth();
	const int h = getHeight();

	if (w > h)
	{
		label.setFont(Font(14.f));
		label.setBounds(h+3, 0, w-h-5, 20);
		label.setJustificationType(Justification::centredLeft);
		label.setBorderSize(BorderSize<int>(1, 1, 1, 1));
	}
	else
	{
		label.setFont(Font(16.f));
		const int lw = jmin(w, 60);
		label.setBounds((w-lw)/2, h-24,lw, 20);
		label.setJustificationType(Justification::centred);
	}
}

void GuiSlider::mouseDown(const MouseEvent& e)
{
	if (e.mods.isAltDown() || e.getNumberOfClicks() > 1)
		setValue(resetValue, sendNotificationAsync);
	else
		Slider::mouseDown(e);
}

void GuiSlider::labelTextChanged(Label* labelThatHasChanged)
{
	if (labelThatHasChanged == &label)
	{
		const double val = label.getText().getDoubleValue();
		setValue(val, sendNotificationAsync);
	}
}

void GuiSlider::sliderValueChanged(Slider* slider)
{
	if (slider == this)
		setLabelText();
}

void GuiSlider::setLabelText()
{
	const double val = getValue();

	const double interval = getInterval();

	if (interval > 0.9)
		label.setText(String((int) val), dontSendNotification);
	else if (interval > 0.4)
		label.setText(String(val, 1), dontSendNotification);
	else
		label.setText(String(val, 2), dontSendNotification);
}