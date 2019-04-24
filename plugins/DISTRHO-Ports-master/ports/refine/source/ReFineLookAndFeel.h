#ifndef REFINELOOKANDFEEL_H_INCLUDED
#define REFINELOOKANDFEEL_H_INCLUDED

#include "JuceHeader.h"

class RefinedSlider : public Slider
{
public:

    RefinedSlider (Image slider, Image vu, AudioProcessorValueTreeState& state, const String& parameterID)
        : sliderImage (slider), vuImage (vu), vuValue (0.f)
    {
        attachment = new AudioProcessorValueTreeState::SliderAttachment(state, parameterID, *this);
        setSliderStyle(Slider::RotaryVerticalDrag);
        setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    }

    virtual ~RefinedSlider()
    {
    }
    
    void setVuValue (float newValue)
    {
        if (newValue != vuValue)
        {
            vuValue = newValue;
            repaint();
        }
    }

    float getVuValue() const
    {
        return vuValue;
    }

    const Image sliderImage;
    const Image vuImage;

private:

    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> attachment;
    float vuValue;
    JUCE_DECLARE_NON_COPYABLE(RefinedSlider)
};

class X2Button : public Button
{
public:

    X2Button (AudioProcessorValueTreeState& state, const String& parameterID)
        : Button ("X2Button"),
          image (ImageCache::getFromMemory(BinaryData::x2button_png, BinaryData::x2button_pngSize))
    {
        attachment = new AudioProcessorValueTreeState::ButtonAttachment(state, parameterID, *this);
        setClickingTogglesState(true);
    }

    void paintButton (Graphics& g, bool /*isMouseOverButton*/, bool isButtonDown)
    {
        const bool state = getToggleState() || isButtonDown;
        const int iw = image.getWidth();
        const int ih = image.getHeight();
        g.drawImageAt(image.getClippedImage(juce::Rectangle<int> (0, state ? ih/2 : 0, iw, ih/2)), 0, 0);
    }

private:

    ScopedPointer<AudioProcessorValueTreeState::ButtonAttachment> attachment;
    Image image;
};

class RefineLookAndFeel : public LookAndFeel_V3
{
public:

    void drawRotarySlider (Graphics&, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        Slider&) override;

private:

};



#endif  // REFINELOOKANDFEEL_H_INCLUDED
