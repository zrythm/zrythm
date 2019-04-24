/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  18 Jan 2010 2:05:05 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "XEQMain.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
XEQMain::XEQMain (XEQPlugin* plugin_)
    : plugin (plugin_),
      eqgraph (0),
      eq1Gain (0),
      eq1Bw (0),
      eq1Freq (0),
      eq2Gain (0),
      eq2Bw (0),
      eq2Freq (0),
      eq3Gain (0),
      eq3Bw (0),
      eq3Freq (0),
      eq4Gain (0),
      eq4Bw (0),
      eq4Freq (0),
      eq5Gain (0),
      eq5Bw (0),
      eq5Freq (0),
      eq6Gain (0),
      eq6Bw (0),
      eq6Freq (0),
      label (0),
      gainSlider (0),
      drywetSlider (0),
      label2 (0)
{
    addAndMakeVisible (eqgraph = new EQGraph());

    addAndMakeVisible (eq1Gain = new ImageSlider (String()));
    eq1Gain->setTooltip ("Band 1 Gain");
    eq1Gain->setRange (0, 1, 0.0001);
    eq1Gain->setSliderStyle (Slider::LinearVertical);
    eq1Gain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq1Gain->addListener (this);

    addAndMakeVisible (eq1Bw = new ParameterSlider (String()));
    eq1Bw->setTooltip ("Band 1 Q");
    eq1Bw->setRange (0, 1, 0.0001);
    eq1Bw->setSliderStyle (Slider::RotaryVerticalDrag);
    eq1Bw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq1Bw->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq1Bw->addListener (this);

    addAndMakeVisible (eq1Freq = new ParameterSlider (String()));
    eq1Freq->setTooltip ("Band 1 Frequency");
    eq1Freq->setRange (0, 1, 0.0001);
    eq1Freq->setSliderStyle (Slider::RotaryVerticalDrag);
    eq1Freq->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq1Freq->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq1Freq->addListener (this);

    addAndMakeVisible (eq2Gain = new ImageSlider (String()));
    eq2Gain->setTooltip ("Band 2 Gain");
    eq2Gain->setRange (0, 1, 0.0001);
    eq2Gain->setSliderStyle (Slider::LinearVertical);
    eq2Gain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq2Gain->addListener (this);

    addAndMakeVisible (eq2Bw = new ParameterSlider (String()));
    eq2Bw->setTooltip ("Band 2 Q");
    eq2Bw->setRange (0, 1, 0.0001);
    eq2Bw->setSliderStyle (Slider::RotaryVerticalDrag);
    eq2Bw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq2Bw->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq2Bw->addListener (this);

    addAndMakeVisible (eq2Freq = new ParameterSlider (String()));
    eq2Freq->setTooltip ("Band 2 Frequency");
    eq2Freq->setRange (0, 1, 0.0001);
    eq2Freq->setSliderStyle (Slider::RotaryVerticalDrag);
    eq2Freq->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq2Freq->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq2Freq->addListener (this);

    addAndMakeVisible (eq3Gain = new ImageSlider (String()));
    eq3Gain->setTooltip ("Band 3 Gain");
    eq3Gain->setRange (0, 1, 0.0001);
    eq3Gain->setSliderStyle (Slider::LinearVertical);
    eq3Gain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq3Gain->addListener (this);

    addAndMakeVisible (eq3Bw = new ParameterSlider (String()));
    eq3Bw->setTooltip ("Band 3 Q");
    eq3Bw->setRange (0, 1, 0.0001);
    eq3Bw->setSliderStyle (Slider::RotaryVerticalDrag);
    eq3Bw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq3Bw->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq3Bw->addListener (this);

    addAndMakeVisible (eq3Freq = new ParameterSlider (String()));
    eq3Freq->setTooltip ("Band 3 Frequency");
    eq3Freq->setRange (0, 1, 0.0001);
    eq3Freq->setSliderStyle (Slider::RotaryVerticalDrag);
    eq3Freq->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq3Freq->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq3Freq->addListener (this);

    addAndMakeVisible (eq4Gain = new ImageSlider (String()));
    eq4Gain->setTooltip ("Band 4 Gain");
    eq4Gain->setRange (0, 1, 0.0001);
    eq4Gain->setSliderStyle (Slider::LinearVertical);
    eq4Gain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq4Gain->addListener (this);

    addAndMakeVisible (eq4Bw = new ParameterSlider (String()));
    eq4Bw->setTooltip ("Band 4 Q");
    eq4Bw->setRange (0, 1, 0.0001);
    eq4Bw->setSliderStyle (Slider::RotaryVerticalDrag);
    eq4Bw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq4Bw->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq4Bw->addListener (this);

    addAndMakeVisible (eq4Freq = new ParameterSlider (String()));
    eq4Freq->setTooltip ("Band 4 Frequency");
    eq4Freq->setRange (0, 1, 0.0001);
    eq4Freq->setSliderStyle (Slider::RotaryVerticalDrag);
    eq4Freq->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq4Freq->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq4Freq->addListener (this);

    addAndMakeVisible (eq5Gain = new ImageSlider (String()));
    eq5Gain->setTooltip ("Band 5 Gain");
    eq5Gain->setRange (0, 1, 0.0001);
    eq5Gain->setSliderStyle (Slider::LinearVertical);
    eq5Gain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq5Gain->addListener (this);

    addAndMakeVisible (eq5Bw = new ParameterSlider (String()));
    eq5Bw->setTooltip ("Band 5 Q");
    eq5Bw->setRange (0, 1, 0.0001);
    eq5Bw->setSliderStyle (Slider::RotaryVerticalDrag);
    eq5Bw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq5Bw->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq5Bw->addListener (this);

    addAndMakeVisible (eq5Freq = new ParameterSlider (String()));
    eq5Freq->setTooltip ("Band 5 Frequency");
    eq5Freq->setRange (0, 1, 0.0001);
    eq5Freq->setSliderStyle (Slider::RotaryVerticalDrag);
    eq5Freq->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq5Freq->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq5Freq->addListener (this);

    addAndMakeVisible (eq6Gain = new ImageSlider (String()));
    eq6Gain->setTooltip ("Band 6 Gain");
    eq6Gain->setRange (0, 1, 0.0001);
    eq6Gain->setSliderStyle (Slider::LinearVertical);
    eq6Gain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq6Gain->addListener (this);

    addAndMakeVisible (eq6Bw = new ParameterSlider (String()));
    eq6Bw->setTooltip ("Band 6 Q");
    eq6Bw->setRange (0, 1, 0.0001);
    eq6Bw->setSliderStyle (Slider::RotaryVerticalDrag);
    eq6Bw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq6Bw->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq6Bw->addListener (this);

    addAndMakeVisible (eq6Freq = new ParameterSlider (String()));
    eq6Freq->setTooltip ("Band 6 Frequency");
    eq6Freq->setRange (0, 1, 0.0001);
    eq6Freq->setSliderStyle (Slider::RotaryVerticalDrag);
    eq6Freq->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    eq6Freq->setColour (Slider::rotarySliderFillColourId, Colours::azure);
    eq6Freq->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          "1     2     3     4     5     6"));
    label->setFont (Font (15.0000f, Font::bold));
    label->setJustificationType (Justification::centred);
    label->setEditable (false, false, false);
    label->setColour (Label::backgroundColourId, Colour (0x0));
    label->setColour (Label::textColourId, Colour (0xafffffff));
    label->setColour (Label::outlineColourId, Colour (0x0));
    label->setColour (TextEditor::textColourId, Colours::white);
    label->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (gainSlider = new ImageSlider (String()));
    gainSlider->setRange (0, 1, 0.0001);
    gainSlider->setSliderStyle (Slider::LinearVertical);
    gainSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    gainSlider->addListener (this);

    addAndMakeVisible (drywetSlider = new ImageSlider (String()));
    drywetSlider->setRange (0, 1, 0.0001);
    drywetSlider->setSliderStyle (Slider::LinearVertical);
    drywetSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    drywetSlider->addListener (this);

    addAndMakeVisible (label2 = new Label ("new label",
                                           "D/W   GAIN\n"));
    label2->setFont (Font (9.3000f, Font::bold));
    label2->setJustificationType (Justification::centred);
    label2->setEditable (false, false, false);
    label2->setColour (Label::textColourId, Colour (0xafffffff));
    label2->setColour (TextEditor::textColourId, Colours::azure);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x0));


    //[UserPreSize]
    //[/UserPreSize]

    setSize (520, 227);

    //[Constructor] You can add your own custom stuff here..
    manager = plugin->getEqualizer ();

    eqgraph->setEqualizer (manager);
    eqgraph->setLineColour (Colours::aliceblue);
    eqgraph->setFillColour (Colours::lightblue.withAlpha (0.6f));

    plugin->getParameterLock().enter ();
        plugin->addListenerToParameter (PAR_GAIN, gainSlider);
        plugin->addListenerToParameter (PAR_GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_DRYWET, drywetSlider);
        plugin->addListenerToParameter (PAR_BAND1GAIN, eq1Gain);
        plugin->addListenerToParameter (PAR_BAND1GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_BAND1FREQ, eq1Freq);
        plugin->addListenerToParameter (PAR_BAND1FREQ, eqgraph);
        plugin->addListenerToParameter (PAR_BAND1BW, eq1Bw);
        plugin->addListenerToParameter (PAR_BAND1BW, eqgraph);
        plugin->addListenerToParameter (PAR_BAND2GAIN, eq2Gain);
        plugin->addListenerToParameter (PAR_BAND2GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_BAND2FREQ, eq2Freq);
        plugin->addListenerToParameter (PAR_BAND2FREQ, eqgraph);
        plugin->addListenerToParameter (PAR_BAND2BW, eq2Bw);
        plugin->addListenerToParameter (PAR_BAND2BW, eqgraph);
        plugin->addListenerToParameter (PAR_BAND3GAIN, eq3Gain);
        plugin->addListenerToParameter (PAR_BAND3GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_BAND3FREQ, eq3Freq);
        plugin->addListenerToParameter (PAR_BAND3FREQ, eqgraph);
        plugin->addListenerToParameter (PAR_BAND3BW, eq3Bw);
        plugin->addListenerToParameter (PAR_BAND3BW, eqgraph);
        plugin->addListenerToParameter (PAR_BAND4GAIN, eq4Gain);
        plugin->addListenerToParameter (PAR_BAND4GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_BAND4FREQ, eq4Freq);
        plugin->addListenerToParameter (PAR_BAND4FREQ, eqgraph);
        plugin->addListenerToParameter (PAR_BAND4BW, eq4Bw);
        plugin->addListenerToParameter (PAR_BAND4BW, eqgraph);
        plugin->addListenerToParameter (PAR_BAND5GAIN, eq5Gain);
        plugin->addListenerToParameter (PAR_BAND5GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_BAND5FREQ, eq5Freq);
        plugin->addListenerToParameter (PAR_BAND5FREQ, eqgraph);
        plugin->addListenerToParameter (PAR_BAND5BW, eq5Bw);
        plugin->addListenerToParameter (PAR_BAND5BW, eqgraph);
        plugin->addListenerToParameter (PAR_BAND6GAIN, eq6Gain);
        plugin->addListenerToParameter (PAR_BAND6GAIN, eqgraph);
        plugin->addListenerToParameter (PAR_BAND6FREQ, eq6Freq);
        plugin->addListenerToParameter (PAR_BAND6FREQ, eqgraph);
        plugin->addListenerToParameter (PAR_BAND6BW, eq6Bw);
        plugin->addListenerToParameter (PAR_BAND6BW, eqgraph);
    plugin->getParameterLock().exit ();

    updateControls ();
    //[/Constructor]
}

XEQMain::~XEQMain()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    plugin->getParameterLock().enter ();
        plugin->removeListenerToParameter (PAR_GAIN, gainSlider);
        plugin->removeListenerToParameter (PAR_GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_DRYWET, drywetSlider);
        plugin->removeListenerToParameter (PAR_BAND1GAIN, eq1Gain);
        plugin->removeListenerToParameter (PAR_BAND1GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND1FREQ, eq1Freq);
        plugin->removeListenerToParameter (PAR_BAND1FREQ, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND1BW, eq1Bw);
        plugin->removeListenerToParameter (PAR_BAND1BW, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND2GAIN, eq2Gain);
        plugin->removeListenerToParameter (PAR_BAND2GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND2FREQ, eq2Freq);
        plugin->removeListenerToParameter (PAR_BAND2FREQ, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND2BW, eq2Bw);
        plugin->removeListenerToParameter (PAR_BAND2BW, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND3GAIN, eq3Gain);
        plugin->removeListenerToParameter (PAR_BAND3GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND3FREQ, eq3Freq);
        plugin->removeListenerToParameter (PAR_BAND3FREQ, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND3BW, eq3Bw);
        plugin->removeListenerToParameter (PAR_BAND3BW, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND4GAIN, eq4Gain);
        plugin->removeListenerToParameter (PAR_BAND4GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND4FREQ, eq4Freq);
        plugin->removeListenerToParameter (PAR_BAND4FREQ, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND4BW, eq4Bw);
        plugin->removeListenerToParameter (PAR_BAND4BW, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND5GAIN, eq5Gain);
        plugin->removeListenerToParameter (PAR_BAND5GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND5FREQ, eq5Freq);
        plugin->removeListenerToParameter (PAR_BAND5FREQ, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND5BW, eq5Bw);
        plugin->removeListenerToParameter (PAR_BAND5BW, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND6GAIN, eq6Gain);
        plugin->removeListenerToParameter (PAR_BAND6GAIN, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND6FREQ, eq6Freq);
        plugin->removeListenerToParameter (PAR_BAND6FREQ, eqgraph);
        plugin->removeListenerToParameter (PAR_BAND6BW, eq6Bw);
        plugin->removeListenerToParameter (PAR_BAND6BW, eqgraph);
    plugin->getParameterLock().exit ();
    //[/Destructor_pre]

    deleteAndZero (eqgraph);
    deleteAndZero (eq1Gain);
    deleteAndZero (eq1Bw);
    deleteAndZero (eq1Freq);
    deleteAndZero (eq2Gain);
    deleteAndZero (eq2Bw);
    deleteAndZero (eq2Freq);
    deleteAndZero (eq3Gain);
    deleteAndZero (eq3Bw);
    deleteAndZero (eq3Freq);
    deleteAndZero (eq4Gain);
    deleteAndZero (eq4Bw);
    deleteAndZero (eq4Freq);
    deleteAndZero (eq5Gain);
    deleteAndZero (eq5Bw);
    deleteAndZero (eq5Freq);
    deleteAndZero (eq6Gain);
    deleteAndZero (eq6Bw);
    deleteAndZero (eq6Freq);
    deleteAndZero (label);
    deleteAndZero (gainSlider);
    deleteAndZero (drywetSlider);
    deleteAndZero (label2);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void XEQMain::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff575f7d));

    g.setGradientFill (ColourGradient (Colour (0xff575f7d),
                                       334.0f, 220.0f,
                                       Colours::black,
                                       334.0f, 231.0f,
                                       false));
    g.fillRect (0, 212, 520, 15);

    g.setColour (Colour (0x4cffffff));
    g.fillRoundedRectangle (286.0f, 11.0f, 156.0f, 146.0f, 6.0000f);

    g.setColour (Colour (0x4ccaccce));
    g.fillRoundedRectangle (286.0f, 162.0f, 156.0f, 25.0f, 6.0000f);

    g.setColour (Colour (0x4ccaccce));
    g.fillRoundedRectangle (287.0f, 192.0f, 156.0f, 25.0f, 6.0000f);

    g.setColour (Colour (0x4c3c3c3c));
    g.fillRoundedRectangle (447.0f, 11.0f, 67.0f, 206.0f, 6.0000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void XEQMain::resized()
{
    eqgraph->setBounds (5, 11, 276, 146);
    eq1Gain->setBounds (293, 25, 10, 128);
    eq1Bw->setBounds (287, 192, 25, 25);
    eq1Freq->setBounds (287, 162, 25, 25);
    eq2Gain->setBounds (319, 25, 10, 128);
    eq2Bw->setBounds (313, 192, 25, 25);
    eq2Freq->setBounds (313, 162, 25, 25);
    eq3Gain->setBounds (345, 25, 10, 128);
    eq3Bw->setBounds (339, 192, 25, 25);
    eq3Freq->setBounds (339, 162, 25, 25);
    eq4Gain->setBounds (371, 25, 10, 128);
    eq4Bw->setBounds (365, 192, 25, 25);
    eq4Freq->setBounds (365, 162, 25, 25);
    eq5Gain->setBounds (397, 25, 10, 128);
    eq5Bw->setBounds (391, 192, 25, 25);
    eq5Freq->setBounds (391, 162, 25, 25);
    eq6Gain->setBounds (423, 25, 10, 128);
    eq6Bw->setBounds (417, 192, 25, 25);
    eq6Freq->setBounds (417, 162, 25, 25);
    label->setBounds (292, 12, 143, 16);
    gainSlider->setBounds (489, 24, 10, 170);
    drywetSlider->setBounds (460, 24, 10, 170);
    label2->setBounds (451, 197, 59, 17);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void XEQMain::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == eq1Gain)
    {
        //[UserSliderCode_eq1Gain] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND1GAIN, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq1Gain]
    }
    else if (sliderThatWasMoved == eq1Bw)
    {
        //[UserSliderCode_eq1Bw] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND1BW, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq1Bw]
    }
    else if (sliderThatWasMoved == eq1Freq)
    {
        //[UserSliderCode_eq1Freq] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND1FREQ, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq1Freq]
    }
    else if (sliderThatWasMoved == eq2Gain)
    {
        //[UserSliderCode_eq2Gain] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND2GAIN, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq2Gain]
    }
    else if (sliderThatWasMoved == eq2Bw)
    {
        //[UserSliderCode_eq2Bw] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND2BW, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq2Bw]
    }
    else if (sliderThatWasMoved == eq2Freq)
    {
        //[UserSliderCode_eq2Freq] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND2FREQ, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq2Freq]
    }
    else if (sliderThatWasMoved == eq3Gain)
    {
        //[UserSliderCode_eq3Gain] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND3GAIN, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq3Gain]
    }
    else if (sliderThatWasMoved == eq3Bw)
    {
        //[UserSliderCode_eq3Bw] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND3BW, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq3Bw]
    }
    else if (sliderThatWasMoved == eq3Freq)
    {
        //[UserSliderCode_eq3Freq] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND3FREQ, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq3Freq]
    }
    else if (sliderThatWasMoved == eq4Gain)
    {
        //[UserSliderCode_eq4Gain] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND4GAIN, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq4Gain]
    }
    else if (sliderThatWasMoved == eq4Bw)
    {
        //[UserSliderCode_eq4Bw] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND4BW, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq4Bw]
    }
    else if (sliderThatWasMoved == eq4Freq)
    {
        //[UserSliderCode_eq4Freq] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND4FREQ, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq4Freq]
    }
    else if (sliderThatWasMoved == eq5Gain)
    {
        //[UserSliderCode_eq5Gain] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND5GAIN, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq5Gain]
    }
    else if (sliderThatWasMoved == eq5Bw)
    {
        //[UserSliderCode_eq5Bw] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND5BW, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq5Bw]
    }
    else if (sliderThatWasMoved == eq5Freq)
    {
        //[UserSliderCode_eq5Freq] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND5FREQ, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq5Freq]
    }
    else if (sliderThatWasMoved == eq6Gain)
    {
        //[UserSliderCode_eq6Gain] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND6GAIN, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq6Gain]
    }
    else if (sliderThatWasMoved == eq6Bw)
    {
        //[UserSliderCode_eq6Bw] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND6BW, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq6Bw]
    }
    else if (sliderThatWasMoved == eq6Freq)
    {
        //[UserSliderCode_eq6Freq] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_BAND6FREQ, sliderThatWasMoved->getValue());
        //updateScope ();
        //[/UserSliderCode_eq6Freq]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_GAIN, sliderThatWasMoved->getValue());
        // manager->setEffectParameter (0, (int) (sliderThatWasMoved->getValue() * 127.0f));
        //updateScope ();
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == drywetSlider)
    {
        //[UserSliderCode_drywetSlider] -- add your slider handling code here..
        plugin->setParameterNotifyingHost (PAR_DRYWET, sliderThatWasMoved->getValue());
        // manager->setEffectParameter (1, (int) (sliderThatWasMoved->getValue() * 127.0f));
        //[/UserSliderCode_drywetSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void XEQMain::updateControls ()
{
    eq1Gain->setValue (plugin->getParameter (PAR_BAND1GAIN), dontSendNotification);
    eq1Freq->setValue (plugin->getParameter (PAR_BAND1FREQ), dontSendNotification);
    eq1Bw->setValue (plugin->getParameter (PAR_BAND1BW), dontSendNotification);

    eq2Gain->setValue (plugin->getParameter (PAR_BAND2GAIN), dontSendNotification);
    eq2Freq->setValue (plugin->getParameter (PAR_BAND2FREQ), dontSendNotification);
    eq2Bw->setValue (plugin->getParameter (PAR_BAND2BW), dontSendNotification);

    eq3Gain->setValue (plugin->getParameter (PAR_BAND3GAIN), dontSendNotification);
    eq3Freq->setValue (plugin->getParameter (PAR_BAND3FREQ), dontSendNotification);
    eq3Bw->setValue (plugin->getParameter (PAR_BAND3BW), dontSendNotification);

    eq4Gain->setValue (plugin->getParameter (PAR_BAND4GAIN), dontSendNotification);
    eq4Freq->setValue (plugin->getParameter (PAR_BAND4FREQ), dontSendNotification);
    eq4Bw->setValue (plugin->getParameter (PAR_BAND4BW), dontSendNotification);

    eq5Gain->setValue (plugin->getParameter (PAR_BAND5GAIN), dontSendNotification);
    eq5Freq->setValue (plugin->getParameter (PAR_BAND5FREQ), dontSendNotification);
    eq5Bw->setValue (plugin->getParameter (PAR_BAND5BW), dontSendNotification);

    eq6Gain->setValue (plugin->getParameter (PAR_BAND6GAIN), dontSendNotification);
    eq6Freq->setValue (plugin->getParameter (PAR_BAND6FREQ), dontSendNotification);
    eq6Bw->setValue (plugin->getParameter (PAR_BAND6BW), dontSendNotification);

    gainSlider->setValue (plugin->getParameter (PAR_GAIN), dontSendNotification);
    drywetSlider->setValue (plugin->getParameter (PAR_DRYWET), dontSendNotification);

    updateScope ();
}

void XEQMain::updateScope ()
{
    eqgraph->repaint ();
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="XEQMain" componentName=""
                 parentClasses="public Component" constructorParams="XEQPlugin* plugin_"
                 variableInitialisers="plugin (plugin_)" snapPixels="5" snapActive="0"
                 snapShown="0" overlayOpacity="0.330000013" fixedSize="1" initialWidth="520"
                 initialHeight="227">
  <BACKGROUND backgroundColour="ff575f7d">
    <RECT pos="0 212 520 15" fill="linear: 334 220, 334 231, 0=ff575f7d, 1=ff000000"
          hasStroke="0"/>
    <ROUNDRECT pos="286 11 156 146" cornerSize="6" fill="solid: 4cffffff" hasStroke="0"/>
    <ROUNDRECT pos="286 162 156 25" cornerSize="6" fill="solid: 4ccaccce" hasStroke="0"/>
    <ROUNDRECT pos="287 192 156 25" cornerSize="6" fill="solid: 4ccaccce" hasStroke="0"/>
    <ROUNDRECT pos="447 11 67 206" cornerSize="6" fill="solid: 4c3c3c3c" hasStroke="0"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="" id="9d00c3a88c263573" memberName="eqgraph" virtualName=""
                    explicitFocusOrder="0" pos="5 11 276 146" class="EQGraph" params=""/>
  <SLIDER name="" id="1f95da32eb640e10" memberName="eq1Gain" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="293 25 10 128" tooltip="Band 1 Gain"
          min="0" max="1" int="0.0001" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="1de567fd9cf2a5d2" memberName="eq1Bw" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="287 192 25 25" tooltip="Band 1 Q"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="b8a671c866e707b6" memberName="eq1Freq" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="287 162 25 25" tooltip="Band 1 Frequency"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="d5b15f3566889e1" memberName="eq2Gain" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="319 25 10 128" tooltip="Band 2 Gain"
          min="0" max="1" int="0.0001" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="6362c354af286ddb" memberName="eq2Bw" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="313 192 25 25" tooltip="Band 2 Q"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="abe2eaa20110c39b" memberName="eq2Freq" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="313 162 25 25" tooltip="Band 2 Frequency"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="bb521c49b89a319a" memberName="eq3Gain" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="345 25 10 128" tooltip="Band 3 Gain"
          min="0" max="1" int="0.0001" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="a8bd8daf16e7b450" memberName="eq3Bw" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="339 192 25 25" tooltip="Band 3 Q"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="5b6d4c28111ef1bd" memberName="eq3Freq" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="339 162 25 25" tooltip="Band 3 Frequency"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="3806c129dfc1f1ef" memberName="eq4Gain" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="371 25 10 128" tooltip="Band 4 Gain"
          min="0" max="1" int="0.0001" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="28aea7ebdecfba7" memberName="eq4Bw" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="365 192 25 25" tooltip="Band 4 Q"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="9599a95f6f08c87" memberName="eq4Freq" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="365 162 25 25" tooltip="Band 4 Frequency"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="9e7588115825f674" memberName="eq5Gain" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="397 25 10 128" tooltip="Band 5 Gain"
          min="0" max="1" int="0.0001" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="b556b9476fa5a80b" memberName="eq5Bw" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="391 192 25 25" tooltip="Band 5 Q"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="e5973f56619b9c5e" memberName="eq5Freq" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="391 162 25 25" tooltip="Band 5 Frequency"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="34270df31e626e55" memberName="eq6Gain" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="423 25 10 128" tooltip="Band 6 Gain"
          min="0" max="1" int="0.0001" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="f5e958253fdc2662" memberName="eq6Bw" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="417 192 25 25" tooltip="Band 6 Q"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="5dab0782bac39d6d" memberName="eq6Freq" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="417 162 25 25" tooltip="Band 6 Frequency"
          rotarysliderfill="fff0ffff" min="0" max="1" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="9f8df2af40274e22" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="292 12 143 16" bkgCol="0" textCol="afffffff"
         outlineCol="0" edTextCol="ffffffff" edBkgCol="0" labelText="1     2     3     4     5     6"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="1" italic="0" justification="36"/>
  <SLIDER name="" id="2eaf39291692360a" memberName="gainSlider" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="489 24 10 170" min="0" max="1" int="0.0001"
          style="LinearVertical" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="2d64edaf15970a0d" memberName="drywetSlider" virtualName="ImageSlider"
          explicitFocusOrder="0" pos="460 24 10 170" min="0" max="1" int="0.0001"
          style="LinearVertical" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="83c207b9cbc1851c" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="451 197 59 17" textCol="afffffff"
         edTextCol="fff0ffff" edBkgCol="0" labelText="D/W   GAIN&#10;" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="9.3" bold="1" italic="0" justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
