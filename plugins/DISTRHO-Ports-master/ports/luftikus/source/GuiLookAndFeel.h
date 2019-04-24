#ifndef __GUILOOKANDFEEL_H_A272DB78__
#define __GUILOOKANDFEEL_H_A272DB78__

#include "JuceHeader.h"

class GuiLookAndFeel : public LookAndFeel_V2
{
public:
	GuiLookAndFeel();
	void drawTickBox (Graphics& g,
                              Component& component,
                              float x, float y, float w, float h,
                              bool ticked,
                              bool isEnabled,
                              bool /*isMouseOverButton*/,
                              bool /*isButtonDown*/) override;


	void drawRotarySlider (Graphics& g,
                           int x, int y,
                           int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           Slider& slider) override;

	void drawToggleButton (Graphics& g,
	                       ToggleButton& button,
	                       bool isMouseOverButton,
	                       bool isButtonDown) override;
private:
	Image chickenKnob;
	int numChickenImages;

	Image masterKnob;
	int numTrimImages;

	Image toggleOff;
	Image toggleOn;
	Image switchOff;
	Image switchOn;

};


class GuiSlider : public Slider,
                  public Label::Listener,
                  public Slider::Listener
{
public:
	explicit GuiSlider (const String& componentName = String());

	GuiSlider (SliderStyle style, TextEntryBoxPosition textBoxPosition);

	void resized() override;

	void mouseDown(const MouseEvent& e) override;

	void labelTextChanged(Label* labelThatHasChanged) override;

	void sliderValueChanged(Slider* slider) override;

	void setLabelText();

private:

	double resetValue;
	Label label;
};


#endif  // __GUILOOKANDFEEL_H_A272DB78__
