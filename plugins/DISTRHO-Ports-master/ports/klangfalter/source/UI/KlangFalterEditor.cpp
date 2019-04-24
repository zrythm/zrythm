/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  19 Apr 2013 4:40:05pm

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

#include "../DecibelScaling.h"
#include "../Parameters.h"

//[/Headers]

#include "KlangFalterEditor.h"
#include "IRComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...


template<typename T>
T SnapValue(T val, T snapValue, T sensitivity)
{
  return (::fabs(val - snapValue) < sensitivity) ? snapValue : val;
}


static juce::String FormatFrequency(float freq)
{
  if (freq < 1000.0f)
  {
    return juce::String(static_cast<int>(freq+0.5f)) + juce::String("Hz");
  }
  return juce::String(freq/1000.0f, (freq < 1500.0f) ? 2 : 1) + juce::String("kHz");
}


static juce::String FormatSeconds(double seconds)
{
  if (seconds < 1.0)
  {
    return juce::String(static_cast<int>(seconds * 1000.0)) + juce::String("ms");
  }
  return juce::String(seconds, 2) + juce::String("s");
}


//[/MiscUserDefs]

//==============================================================================
KlangFalterEditor::KlangFalterEditor (Processor& processor)
    : AudioProcessorEditor(&processor),
      _processor(processor),
      _decibelScaleDry (0),
      _irTabComponent (0),
      _levelMeterDry (0),
      _dryLevelLabel (0),
      _wetLevelLabel (0),
      _drySlider (0),
      _decibelScaleOut (0),
      _wetSlider (0),
      _browseButton (0),
      _irBrowserComponent (0),
      _settingsButton (0),
      _wetButton (0),
      _dryButton (0),
      _autogainButton (0),
      _reverseButton (0),
      _hiFreqLabel (0),
      _hiGainLabel (0),
      _hiGainHeaderLabel (0),
      _hiFreqHeaderLabel (0),
      _hiGainSlider (0),
      _hiFreqSlider (0),
      _loFreqLabel (0),
      _loGainLabel (0),
      _loGainHeaderLabel (0),
      _loFreqHeaderLabel (0),
      _loGainSlider (0),
      _loFreqSlider (0),
      _levelMeterOut (0),
      _levelMeterOutLabelButton (0),
      _levelMeterDryLabel (0),
      _lowEqButton (0),
      _lowCutFreqLabel (0),
      _lowCutFreqHeaderLabel (0),
      _lowCutFreqSlider (0),
      _highCutFreqLabel (0),
      _highCutFreqHeaderLabel (0),
      _highCutFreqSlider (0),
      _highEqButton (0),
      _attackShapeLabel (0),
      _endLabel (0),
      _endSlider (0),
      _attackShapeSlider (0),
      _decayShapeLabel (0),
      _decayShapeHeaderLabel (0),
      _decayShapeSlider (0),
      _attackShapeHeaderLabel (0),
      _endHeaderLabel (0),
      _beginLabel (0),
      _beginSlider (0),
      _beginHeaderLabel (0),
      _widthLabel (0),
      _widthHeaderLabel (0),
      _widthSlider (0),
      _predelayLabel (0),
      _predelayHeaderLabel (0),
      _predelaySlider (0),
      _stretchLabel (0),
      _stretchHeaderLabel (0),
      _stretchSlider (0),
      _attackHeaderLabel (0),
      _attackLengthLabel (0),
      _attackLengthSlider (0),
      _attackLengthHeaderLabel (0),
      _decayHeaderLabel (0),
      _impulseResponseHeaderLabel (0),
      _stereoHeaderLabel (0)
{
    addAndMakeVisible (_decibelScaleDry = new DecibelScale());

    addAndMakeVisible (_irTabComponent = new TabbedComponent (TabbedButtonBar::TabsAtTop));
    _irTabComponent->setTabBarDepth (30);
    _irTabComponent->addTab (L"Placeholder", Colour (0xffb0b0b6), new IRComponent(), true);
    _irTabComponent->setCurrentTabIndex (0);

    addAndMakeVisible (_levelMeterDry = new LevelMeter());

    addAndMakeVisible (_dryLevelLabel = new Label (L"DryLevelLabel",
                                                   L"-inf"));
    _dryLevelLabel->setFont (Font (11.0000f, Font::plain));
    _dryLevelLabel->setJustificationType (Justification::centredRight);
    _dryLevelLabel->setEditable (false, false, false);
    _dryLevelLabel->setColour (Label::textColourId, Colour (0xff202020));
    _dryLevelLabel->setColour (TextEditor::textColourId, Colours::black);
    _dryLevelLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_wetLevelLabel = new Label (L"WetLevelLabel",
                                                   L"-inf"));
    _wetLevelLabel->setFont (Font (11.0000f, Font::plain));
    _wetLevelLabel->setJustificationType (Justification::centredRight);
    _wetLevelLabel->setEditable (false, false, false);
    _wetLevelLabel->setColour (Label::textColourId, Colour (0xff202020));
    _wetLevelLabel->setColour (TextEditor::textColourId, Colours::black);
    _wetLevelLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_drySlider = new Slider (String()));
    _drySlider->setRange (0, 10, 0);
    _drySlider->setSliderStyle (Slider::LinearVertical);
    _drySlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _drySlider->addListener (this);

    addAndMakeVisible (_decibelScaleOut = new DecibelScale());

    addAndMakeVisible (_wetSlider = new Slider (String()));
    _wetSlider->setRange (0, 10, 0);
    _wetSlider->setSliderStyle (Slider::LinearVertical);
    _wetSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _wetSlider->addListener (this);

    addAndMakeVisible (_browseButton = new TextButton (String()));
    _browseButton->setTooltip (L"Show Browser For Impulse Response Selection");
    _browseButton->setButtonText (L"Show Browser");
    _browseButton->setConnectedEdges (Button::ConnectedOnBottom);
    _browseButton->addListener (this);
    _browseButton->setColour (TextButton::buttonOnColourId, Colour (0xffbcbcff));

    addAndMakeVisible (_irBrowserComponent = new IRBrowserComponent());

    addAndMakeVisible (_settingsButton = new TextButton (String()));
    _settingsButton->setButtonText (L"Settings");
    _settingsButton->setConnectedEdges (Button::ConnectedOnRight | Button::ConnectedOnTop);
    _settingsButton->addListener (this);
    _settingsButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _settingsButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_wetButton = new TextButton (String()));
    _wetButton->setTooltip (L"Wet Signal On/Off");
    _wetButton->setButtonText (L"Wet");
    _wetButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _wetButton->addListener (this);
    _wetButton->setColour (TextButton::buttonColourId, Colour (0x80bcbcbc));
    _wetButton->setColour (TextButton::buttonOnColourId, Colour (0xffbcbcff));
    _wetButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _wetButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_dryButton = new TextButton (String()));
    _dryButton->setTooltip (L"Dry Signal On/Off");
    _dryButton->setButtonText (L"Dry");
    _dryButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _dryButton->addListener (this);
    _dryButton->setColour (TextButton::buttonColourId, Colour (0x80bcbcbc));
    _dryButton->setColour (TextButton::buttonOnColourId, Colour (0xffbcbcff));
    _dryButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _dryButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_autogainButton = new TextButton (String()));
    _autogainButton->setTooltip (L"Autogain On/Off");
    _autogainButton->setButtonText (L"Autogain 0.0dB");
    _autogainButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _autogainButton->addListener (this);
    _autogainButton->setColour (TextButton::buttonColourId, Colour (0x80bcbcbc));
    _autogainButton->setColour (TextButton::buttonOnColourId, Colour (0xffbcbcff));
    _autogainButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _autogainButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_reverseButton = new TextButton (String()));
    _reverseButton->setTooltip (L"Reverse Impulse Response");
    _reverseButton->setButtonText (L"Reverse");
    _reverseButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _reverseButton->addListener (this);
    _reverseButton->setColour (TextButton::buttonColourId, Colour (0x80bcbcbc));
    _reverseButton->setColour (TextButton::buttonOnColourId, Colour (0xffbcbcff));
    _reverseButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _reverseButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_hiFreqLabel = new Label (String(),
                                                 L"15.2kHz"));
    _hiFreqLabel->setFont (Font (11.0000f, Font::plain));
    _hiFreqLabel->setJustificationType (Justification::centred);
    _hiFreqLabel->setEditable (false, false, false);
    _hiFreqLabel->setColour (Label::textColourId, Colour (0xff202020));
    _hiFreqLabel->setColour (TextEditor::textColourId, Colours::black);
    _hiFreqLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_hiGainLabel = new Label (String(),
                                                 L"0.0dB"));
    _hiGainLabel->setFont (Font (11.0000f, Font::plain));
    _hiGainLabel->setJustificationType (Justification::centred);
    _hiGainLabel->setEditable (false, false, false);
    _hiGainLabel->setColour (Label::textColourId, Colour (0xff202020));
    _hiGainLabel->setColour (TextEditor::textColourId, Colours::black);
    _hiGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_hiGainHeaderLabel = new Label (String(),
                                                       L"Gain"));
    _hiGainHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _hiGainHeaderLabel->setJustificationType (Justification::centred);
    _hiGainHeaderLabel->setEditable (false, false, false);
    _hiGainHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _hiGainHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _hiGainHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_hiFreqHeaderLabel = new Label (String(),
                                                       L"Freq"));
    _hiFreqHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _hiFreqHeaderLabel->setJustificationType (Justification::centred);
    _hiFreqHeaderLabel->setEditable (false, false, false);
    _hiFreqHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _hiFreqHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _hiFreqHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_hiGainSlider = new Slider (String()));
    _hiGainSlider->setRange (-30, 30, 0);
    _hiGainSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _hiGainSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _hiGainSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _hiGainSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _hiGainSlider->addListener (this);

    addAndMakeVisible (_hiFreqSlider = new Slider (String()));
    _hiFreqSlider->setRange (2000, 20000, 0);
    _hiFreqSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _hiFreqSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _hiFreqSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _hiFreqSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _hiFreqSlider->addListener (this);

    addAndMakeVisible (_loFreqLabel = new Label (String(),
                                                 L"1234Hz"));
    _loFreqLabel->setFont (Font (11.0000f, Font::plain));
    _loFreqLabel->setJustificationType (Justification::centred);
    _loFreqLabel->setEditable (false, false, false);
    _loFreqLabel->setColour (Label::textColourId, Colour (0xff202020));
    _loFreqLabel->setColour (TextEditor::textColourId, Colours::black);
    _loFreqLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_loGainLabel = new Label (String(),
                                                 L"0.0dB"));
    _loGainLabel->setFont (Font (11.0000f, Font::plain));
    _loGainLabel->setJustificationType (Justification::centred);
    _loGainLabel->setEditable (false, false, false);
    _loGainLabel->setColour (Label::textColourId, Colour (0xff202020));
    _loGainLabel->setColour (TextEditor::textColourId, Colours::black);
    _loGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_loGainHeaderLabel = new Label (String(),
                                                       L"Gain"));
    _loGainHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _loGainHeaderLabel->setJustificationType (Justification::centred);
    _loGainHeaderLabel->setEditable (false, false, false);
    _loGainHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _loGainHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _loGainHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_loFreqHeaderLabel = new Label (String(),
                                                       L"Freq"));
    _loFreqHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _loFreqHeaderLabel->setJustificationType (Justification::centred);
    _loFreqHeaderLabel->setEditable (false, false, false);
    _loFreqHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _loFreqHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _loFreqHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_loGainSlider = new Slider (String()));
    _loGainSlider->setRange (-30, 30, 0);
    _loGainSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _loGainSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _loGainSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _loGainSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _loGainSlider->addListener (this);

    addAndMakeVisible (_loFreqSlider = new Slider (String()));
    _loFreqSlider->setRange (20, 2000, 0);
    _loFreqSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _loFreqSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _loFreqSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _loFreqSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _loFreqSlider->addListener (this);

    addAndMakeVisible (_levelMeterOut = new LevelMeter());

    addAndMakeVisible (_levelMeterOutLabelButton = new TextButton (String()));
    _levelMeterOutLabelButton->setTooltip (L"Switches Between Out/Wet Level Measurement");
    _levelMeterOutLabelButton->setButtonText (L"Out");
    _levelMeterOutLabelButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _levelMeterOutLabelButton->addListener (this);
    _levelMeterOutLabelButton->setColour (TextButton::buttonColourId, Colour (0xbbbbff));
    _levelMeterOutLabelButton->setColour (TextButton::buttonOnColourId, Colour (0xbcbcff));
    _levelMeterOutLabelButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _levelMeterOutLabelButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_levelMeterDryLabel = new Label (String(),
                                                        L"Dry"));
    _levelMeterDryLabel->setFont (Font (11.0000f, Font::plain));
    _levelMeterDryLabel->setJustificationType (Justification::centred);
    _levelMeterDryLabel->setEditable (false, false, false);
    _levelMeterDryLabel->setColour (Label::textColourId, Colour (0xff202020));
    _levelMeterDryLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _levelMeterDryLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_lowEqButton = new TextButton (String()));
    _lowEqButton->setButtonText (L"Low Cut");
    _lowEqButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _lowEqButton->addListener (this);
    _lowEqButton->setColour (TextButton::buttonColourId, Colour (0xbbbbff));
    _lowEqButton->setColour (TextButton::buttonOnColourId, Colour (0x2c2cff));
    _lowEqButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _lowEqButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_lowCutFreqLabel = new Label (String(),
                                                     L"1234Hz"));
    _lowCutFreqLabel->setFont (Font (11.0000f, Font::plain));
    _lowCutFreqLabel->setJustificationType (Justification::centred);
    _lowCutFreqLabel->setEditable (false, false, false);
    _lowCutFreqLabel->setColour (Label::textColourId, Colour (0xff202020));
    _lowCutFreqLabel->setColour (TextEditor::textColourId, Colours::black);
    _lowCutFreqLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_lowCutFreqHeaderLabel = new Label (String(),
                                                           L"Freq"));
    _lowCutFreqHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _lowCutFreqHeaderLabel->setJustificationType (Justification::centred);
    _lowCutFreqHeaderLabel->setEditable (false, false, false);
    _lowCutFreqHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _lowCutFreqHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _lowCutFreqHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_lowCutFreqSlider = new Slider (String()));
    _lowCutFreqSlider->setRange (20, 2000, 0);
    _lowCutFreqSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _lowCutFreqSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _lowCutFreqSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _lowCutFreqSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _lowCutFreqSlider->addListener (this);

    addAndMakeVisible (_highCutFreqLabel = new Label (String(),
                                                      L"15.2kHz"));
    _highCutFreqLabel->setFont (Font (11.0000f, Font::plain));
    _highCutFreqLabel->setJustificationType (Justification::centred);
    _highCutFreqLabel->setEditable (false, false, false);
    _highCutFreqLabel->setColour (Label::textColourId, Colour (0xff202020));
    _highCutFreqLabel->setColour (TextEditor::textColourId, Colours::black);
    _highCutFreqLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_highCutFreqHeaderLabel = new Label (String(),
                                                            L"Freq"));
    _highCutFreqHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _highCutFreqHeaderLabel->setJustificationType (Justification::centred);
    _highCutFreqHeaderLabel->setEditable (false, false, false);
    _highCutFreqHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _highCutFreqHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _highCutFreqHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_highCutFreqSlider = new Slider (String()));
    _highCutFreqSlider->setRange (2000, 20000, 0);
    _highCutFreqSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _highCutFreqSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _highCutFreqSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _highCutFreqSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _highCutFreqSlider->addListener (this);

    addAndMakeVisible (_highEqButton = new TextButton (String()));
    _highEqButton->setButtonText (L"High Cut");
    _highEqButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _highEqButton->addListener (this);
    _highEqButton->setColour (TextButton::buttonColourId, Colour (0xbbbbff));
    _highEqButton->setColour (TextButton::buttonOnColourId, Colour (0x2c2cff));
    _highEqButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _highEqButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_attackShapeLabel = new Label (String(),
                                                      L"1.0"));
    _attackShapeLabel->setFont (Font (11.0000f, Font::plain));
    _attackShapeLabel->setJustificationType (Justification::centred);
    _attackShapeLabel->setEditable (false, false, false);
    _attackShapeLabel->setColour (Label::textColourId, Colour (0xff202020));
    _attackShapeLabel->setColour (TextEditor::textColourId, Colours::black);
    _attackShapeLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_endLabel = new Label (String(),
                                              L"100%"));
    _endLabel->setFont (Font (11.0000f, Font::plain));
    _endLabel->setJustificationType (Justification::centred);
    _endLabel->setEditable (false, false, false);
    _endLabel->setColour (Label::textColourId, Colour (0xff202020));
    _endLabel->setColour (TextEditor::textColourId, Colours::black);
    _endLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_endSlider = new Slider (String()));
    _endSlider->setRange (0, 1, 0);
    _endSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _endSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _endSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _endSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _endSlider->addListener (this);

    addAndMakeVisible (_attackShapeSlider = new Slider (String()));
    _attackShapeSlider->setRange (0, 10, 0);
    _attackShapeSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _attackShapeSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _attackShapeSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _attackShapeSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _attackShapeSlider->addListener (this);
    _attackShapeSlider->setSkewFactor (0.5);

    addAndMakeVisible (_decayShapeLabel = new Label (String(),
                                                     L"1.0"));
    _decayShapeLabel->setFont (Font (11.0000f, Font::plain));
    _decayShapeLabel->setJustificationType (Justification::centred);
    _decayShapeLabel->setEditable (false, false, false);
    _decayShapeLabel->setColour (Label::textColourId, Colour (0xff202020));
    _decayShapeLabel->setColour (TextEditor::textColourId, Colours::black);
    _decayShapeLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_decayShapeHeaderLabel = new Label (String(),
                                                           L"Shape"));
    _decayShapeHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _decayShapeHeaderLabel->setJustificationType (Justification::centred);
    _decayShapeHeaderLabel->setEditable (false, false, false);
    _decayShapeHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _decayShapeHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _decayShapeHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_decayShapeSlider = new Slider (String()));
    _decayShapeSlider->setRange (0, 10, 0);
    _decayShapeSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _decayShapeSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _decayShapeSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _decayShapeSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _decayShapeSlider->addListener (this);
    _decayShapeSlider->setSkewFactor (0.5);

    addAndMakeVisible (_attackShapeHeaderLabel = new Label (String(),
                                                            L"Shape"));
    _attackShapeHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _attackShapeHeaderLabel->setJustificationType (Justification::centred);
    _attackShapeHeaderLabel->setEditable (false, false, false);
    _attackShapeHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _attackShapeHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _attackShapeHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_endHeaderLabel = new Label (String(),
                                                    L"End"));
    _endHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _endHeaderLabel->setJustificationType (Justification::centred);
    _endHeaderLabel->setEditable (false, false, false);
    _endHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _endHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _endHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_beginLabel = new Label (String(),
                                                L"100%"));
    _beginLabel->setFont (Font (11.0000f, Font::plain));
    _beginLabel->setJustificationType (Justification::centred);
    _beginLabel->setEditable (false, false, false);
    _beginLabel->setColour (Label::textColourId, Colour (0xff202020));
    _beginLabel->setColour (TextEditor::textColourId, Colours::black);
    _beginLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_beginSlider = new Slider (String()));
    _beginSlider->setRange (0, 1, 0);
    _beginSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _beginSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _beginSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _beginSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _beginSlider->addListener (this);

    addAndMakeVisible (_beginHeaderLabel = new Label (String(),
                                                      L"Begin"));
    _beginHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _beginHeaderLabel->setJustificationType (Justification::centred);
    _beginHeaderLabel->setEditable (false, false, false);
    _beginHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _beginHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _beginHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_widthLabel = new Label (String(),
                                                L"1.0"));
    _widthLabel->setFont (Font (11.0000f, Font::plain));
    _widthLabel->setJustificationType (Justification::centred);
    _widthLabel->setEditable (false, false, false);
    _widthLabel->setColour (Label::textColourId, Colour (0xff202020));
    _widthLabel->setColour (TextEditor::textColourId, Colours::black);
    _widthLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_widthHeaderLabel = new Label (String(),
                                                      L"Width"));
    _widthHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _widthHeaderLabel->setJustificationType (Justification::centred);
    _widthHeaderLabel->setEditable (false, false, false);
    _widthHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _widthHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _widthHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_widthSlider = new Slider (String()));
    _widthSlider->setRange (0, 10, 0);
    _widthSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _widthSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _widthSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _widthSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _widthSlider->addListener (this);
    _widthSlider->setSkewFactor (0.30102);

    addAndMakeVisible (_predelayLabel = new Label (String(),
                                                   L"0ms"));
    _predelayLabel->setFont (Font (11.0000f, Font::plain));
    _predelayLabel->setJustificationType (Justification::centred);
    _predelayLabel->setEditable (false, false, false);
    _predelayLabel->setColour (Label::textColourId, Colour (0xff202020));
    _predelayLabel->setColour (TextEditor::textColourId, Colours::black);
    _predelayLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_predelayHeaderLabel = new Label (String(),
                                                         L"Gap"));
    _predelayHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _predelayHeaderLabel->setJustificationType (Justification::centred);
    _predelayHeaderLabel->setEditable (false, false, false);
    _predelayHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _predelayHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _predelayHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_predelaySlider = new Slider (String()));
    _predelaySlider->setRange (0, 1000, 0);
    _predelaySlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _predelaySlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _predelaySlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _predelaySlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _predelaySlider->addListener (this);

    addAndMakeVisible (_stretchLabel = new Label (String(),
                                                  L"100%"));
    _stretchLabel->setFont (Font (11.0000f, Font::plain));
    _stretchLabel->setJustificationType (Justification::centred);
    _stretchLabel->setEditable (false, false, false);
    _stretchLabel->setColour (Label::textColourId, Colour (0xff202020));
    _stretchLabel->setColour (TextEditor::textColourId, Colours::black);
    _stretchLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_stretchHeaderLabel = new Label (String(),
                                                        L"Stretch"));
    _stretchHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _stretchHeaderLabel->setJustificationType (Justification::centred);
    _stretchHeaderLabel->setEditable (false, false, false);
    _stretchHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _stretchHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _stretchHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_stretchSlider = new Slider (String()));
    _stretchSlider->setRange (0, 2, 0);
    _stretchSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _stretchSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _stretchSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _stretchSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _stretchSlider->addListener (this);

    addAndMakeVisible (_attackHeaderLabel = new Label (String(),
                                                       L"Attack"));
    _attackHeaderLabel->setFont (Font (15.0000f, Font::plain));
    _attackHeaderLabel->setJustificationType (Justification::centred);
    _attackHeaderLabel->setEditable (false, false, false);
    _attackHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _attackHeaderLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _attackHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_attackLengthLabel = new Label (String(),
                                                       L"0ms"));
    _attackLengthLabel->setFont (Font (11.0000f, Font::plain));
    _attackLengthLabel->setJustificationType (Justification::centred);
    _attackLengthLabel->setEditable (false, false, false);
    _attackLengthLabel->setColour (Label::textColourId, Colour (0xff202020));
    _attackLengthLabel->setColour (TextEditor::textColourId, Colours::black);
    _attackLengthLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_attackLengthSlider = new Slider (String()));
    _attackLengthSlider->setRange (0, 1, 0);
    _attackLengthSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    _attackLengthSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    _attackLengthSlider->setColour (Slider::thumbColourId, Colour (0xffafafff));
    _attackLengthSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xb1606060));
    _attackLengthSlider->addListener (this);
    _attackLengthSlider->setSkewFactor (0.5);

    addAndMakeVisible (_attackLengthHeaderLabel = new Label (String(),
                                                             L"Length"));
    _attackLengthHeaderLabel->setFont (Font (11.0000f, Font::plain));
    _attackLengthHeaderLabel->setJustificationType (Justification::centred);
    _attackLengthHeaderLabel->setEditable (false, false, false);
    _attackLengthHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _attackLengthHeaderLabel->setColour (TextEditor::textColourId, Colours::black);
    _attackLengthHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_decayHeaderLabel = new Label (String(),
                                                      L"Decay"));
    _decayHeaderLabel->setFont (Font (15.0000f, Font::plain));
    _decayHeaderLabel->setJustificationType (Justification::centred);
    _decayHeaderLabel->setEditable (false, false, false);
    _decayHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _decayHeaderLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _decayHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_impulseResponseHeaderLabel = new Label (String(),
                                                                L"Impulse Response"));
    _impulseResponseHeaderLabel->setFont (Font (15.0000f, Font::plain));
    _impulseResponseHeaderLabel->setJustificationType (Justification::centred);
    _impulseResponseHeaderLabel->setEditable (false, false, false);
    _impulseResponseHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _impulseResponseHeaderLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _impulseResponseHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_stereoHeaderLabel = new Label (String(),
                                                       L"Stereo"));
    _stereoHeaderLabel->setFont (Font (15.0000f, Font::plain));
    _stereoHeaderLabel->setJustificationType (Justification::centred);
    _stereoHeaderLabel->setEditable (false, false, false);
    _stereoHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _stereoHeaderLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _stereoHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));


    //[UserPreSize]

    setLookAndFeel(&_customLookAndFeel);

    //[/UserPreSize]

    setSize (760, 330);


    //[Constructor] You can add your own custom stuff here..
    _irTabComponent->clearTabs(); // Remove placeholder only used as dummy in the Jucer
    _browseButton->setClickingTogglesState(true);
    _dryButton->setClickingTogglesState(true);
    _wetButton->setClickingTogglesState(true);
    _autogainButton->setClickingTogglesState(true);
    _reverseButton->setClickingTogglesState(true);
    _levelMeterOutLabelButton->setClickingTogglesState(true);

    _lowCutFreqSlider->setRange(Parameters::EqLowCutFreq.getMinValue(), Parameters::EqLowCutFreq.getMaxValue());
    _loFreqSlider->setRange(Parameters::EqLowShelfFreq.getMinValue(), Parameters::EqLowShelfFreq.getMaxValue());
    _loGainSlider->setRange(Parameters::EqLowShelfDecibels.getMinValue(), Parameters::EqLowShelfDecibels.getMaxValue());

    _highCutFreqSlider->setRange(Parameters::EqHighCutFreq.getMinValue(), Parameters::EqHighCutFreq.getMaxValue());
    _hiFreqSlider->setRange(Parameters::EqHighShelfFreq.getMinValue(), Parameters::EqHighShelfFreq.getMaxValue());
    _hiGainSlider->setRange(Parameters::EqHighShelfDecibels.getMinValue(), Parameters::EqHighShelfDecibels.getMaxValue());

    _processor.addNotificationListener(this);
    _processor.getSettings().addChangeListener(this);
    _irBrowserComponent->init(&_processor);
    updateUI();
    startTimer(100);
    //[/Constructor]
}

KlangFalterEditor::~KlangFalterEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    _settingsDialogWindow.deleteAndZero();
    _processor.removeNotificationListener(this);
    _processor.getSettings().removeChangeListener(this);
    //[/Destructor_pre]

    deleteAndZero (_decibelScaleDry);
    deleteAndZero (_irTabComponent);
    deleteAndZero (_levelMeterDry);
    deleteAndZero (_dryLevelLabel);
    deleteAndZero (_wetLevelLabel);
    deleteAndZero (_drySlider);
    deleteAndZero (_decibelScaleOut);
    deleteAndZero (_wetSlider);
    deleteAndZero (_browseButton);
    deleteAndZero (_irBrowserComponent);
    deleteAndZero (_settingsButton);
    deleteAndZero (_wetButton);
    deleteAndZero (_dryButton);
    deleteAndZero (_autogainButton);
    deleteAndZero (_reverseButton);
    deleteAndZero (_hiFreqLabel);
    deleteAndZero (_hiGainLabel);
    deleteAndZero (_hiGainHeaderLabel);
    deleteAndZero (_hiFreqHeaderLabel);
    deleteAndZero (_hiGainSlider);
    deleteAndZero (_hiFreqSlider);
    deleteAndZero (_loFreqLabel);
    deleteAndZero (_loGainLabel);
    deleteAndZero (_loGainHeaderLabel);
    deleteAndZero (_loFreqHeaderLabel);
    deleteAndZero (_loGainSlider);
    deleteAndZero (_loFreqSlider);
    deleteAndZero (_levelMeterOut);
    deleteAndZero (_levelMeterOutLabelButton);
    deleteAndZero (_levelMeterDryLabel);
    deleteAndZero (_lowEqButton);
    deleteAndZero (_lowCutFreqLabel);
    deleteAndZero (_lowCutFreqHeaderLabel);
    deleteAndZero (_lowCutFreqSlider);
    deleteAndZero (_highCutFreqLabel);
    deleteAndZero (_highCutFreqHeaderLabel);
    deleteAndZero (_highCutFreqSlider);
    deleteAndZero (_highEqButton);
    deleteAndZero (_attackShapeLabel);
    deleteAndZero (_endLabel);
    deleteAndZero (_endSlider);
    deleteAndZero (_attackShapeSlider);
    deleteAndZero (_decayShapeLabel);
    deleteAndZero (_decayShapeHeaderLabel);
    deleteAndZero (_decayShapeSlider);
    deleteAndZero (_attackShapeHeaderLabel);
    deleteAndZero (_endHeaderLabel);
    deleteAndZero (_beginLabel);
    deleteAndZero (_beginSlider);
    deleteAndZero (_beginHeaderLabel);
    deleteAndZero (_widthLabel);
    deleteAndZero (_widthHeaderLabel);
    deleteAndZero (_widthSlider);
    deleteAndZero (_predelayLabel);
    deleteAndZero (_predelayHeaderLabel);
    deleteAndZero (_predelaySlider);
    deleteAndZero (_stretchLabel);
    deleteAndZero (_stretchHeaderLabel);
    deleteAndZero (_stretchSlider);
    deleteAndZero (_attackHeaderLabel);
    deleteAndZero (_attackLengthLabel);
    deleteAndZero (_attackLengthSlider);
    deleteAndZero (_attackLengthHeaderLabel);
    deleteAndZero (_decayHeaderLabel);
    deleteAndZero (_impulseResponseHeaderLabel);
    deleteAndZero (_stereoHeaderLabel);


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void KlangFalterEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xffa6a6b1));

    //[UserPaint] Add your own custom painting code here..
    juce::Image backgroundImage(juce::ImageCache::getFromMemory(BinaryData::brushed_aluminium_png,
                                                                BinaryData::brushed_aluminium_pngSize));
    g.setFillType(juce::FillType(backgroundImage, juce::AffineTransform()));
    g.fillAll();
    //[/UserPaint]
}

void KlangFalterEditor::resized()
{
    _decibelScaleDry->setBounds (584, 40, 32, 176);
    _irTabComponent->setBounds (16, 12, 542, 204);
    _levelMeterDry->setBounds (632, 40, 12, 176);
    _dryLevelLabel->setBounds (572, 220, 60, 24);
    _wetLevelLabel->setBounds (656, 220, 64, 24);
    _drySlider->setBounds (612, 32, 24, 192);
    _decibelScaleOut->setBounds (672, 40, 32, 176);
    _wetSlider->setBounds (700, 32, 24, 192);
    _browseButton->setBounds (12, 308, 736, 24);
    _irBrowserComponent->setBounds (12, 332, 736, 288);
    _settingsButton->setBounds (708, 0, 52, 16);
    _wetButton->setBounds (684, 244, 44, 24);
    _dryButton->setBounds (596, 244, 44, 24);
    _autogainButton->setBounds (596, 276, 132, 24);
    _reverseButton->setBounds (486, 8, 72, 24);
    _hiFreqLabel->setBounds (480, 280, 52, 24);
    _hiGainLabel->setBounds (516, 280, 52, 24);
    _hiGainHeaderLabel->setBounds (516, 236, 52, 24);
    _hiFreqHeaderLabel->setBounds (480, 236, 52, 24);
    _hiGainSlider->setBounds (524, 256, 36, 28);
    _hiFreqSlider->setBounds (488, 256, 36, 28);
    _loFreqLabel->setBounds (396, 280, 52, 24);
    _loGainLabel->setBounds (432, 280, 52, 24);
    _loGainHeaderLabel->setBounds (432, 236, 52, 24);
    _loFreqHeaderLabel->setBounds (396, 236, 52, 24);
    _loGainSlider->setBounds (440, 256, 36, 28);
    _loFreqSlider->setBounds (404, 256, 36, 28);
    _levelMeterOut->setBounds (720, 40, 12, 176);
    _levelMeterOutLabelButton->setBounds (712, 20, 28, 18);
    _levelMeterDryLabel->setBounds (620, 16, 36, 24);
    _lowEqButton->setBounds (404, 220, 72, 24);
    _lowCutFreqLabel->setBounds (414, 280, 52, 24);
    _lowCutFreqHeaderLabel->setBounds (414, 236, 52, 24);
    _lowCutFreqSlider->setBounds (422, 256, 36, 28);
    _highCutFreqLabel->setBounds (498, 280, 52, 24);
    _highCutFreqHeaderLabel->setBounds (498, 236, 52, 24);
    _highCutFreqSlider->setBounds (506, 256, 36, 28);
    _highEqButton->setBounds (488, 220, 72, 24);
    _attackShapeLabel->setBounds (212, 280, 52, 24);
    _endLabel->setBounds (76, 280, 52, 24);
    _endSlider->setBounds (84, 256, 36, 28);
    _attackShapeSlider->setBounds (220, 256, 36, 28);
    _decayShapeLabel->setBounds (276, 280, 52, 24);
    _decayShapeHeaderLabel->setBounds (276, 236, 52, 24);
    _decayShapeSlider->setBounds (284, 256, 36, 28);
    _attackShapeHeaderLabel->setBounds (212, 236, 52, 24);
    _endHeaderLabel->setBounds (76, 236, 52, 24);
    _beginLabel->setBounds (40, 280, 52, 24);
    _beginSlider->setBounds (48, 256, 36, 28);
    _beginHeaderLabel->setBounds (40, 236, 52, 24);
    _widthLabel->setBounds (340, 280, 52, 24);
    _widthHeaderLabel->setBounds (340, 236, 52, 24);
    _widthSlider->setBounds (348, 256, 36, 28);
    _predelayLabel->setBounds (4, 280, 52, 24);
    _predelayHeaderLabel->setBounds (4, 236, 52, 24);
    _predelaySlider->setBounds (12, 256, 36, 28);
    _stretchLabel->setBounds (112, 280, 52, 24);
    _stretchHeaderLabel->setBounds (112, 236, 52, 24);
    _stretchSlider->setBounds (120, 256, 36, 28);
    _attackHeaderLabel->setBounds (176, 220, 88, 24);
    _attackLengthLabel->setBounds (176, 280, 52, 24);
    _attackLengthSlider->setBounds (184, 256, 36, 28);
    _attackLengthHeaderLabel->setBounds (176, 236, 52, 24);
    _decayHeaderLabel->setBounds (256, 220, 88, 24);
    _impulseResponseHeaderLabel->setBounds (12, 220, 144, 24);
    _stereoHeaderLabel->setBounds (340, 220, 52, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void KlangFalterEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == _drySlider)
    {
        //[UserSliderCode__drySlider] -- add your slider handling code here..
        const float scale = static_cast<float>(_drySlider->getValue());
        const float decibels = SnapValue(DecibelScaling::Scale2Db(scale), 0.0f, 0.5f);
        _processor.setParameterNotifyingHost(Parameters::DryDecibels, decibels);
        //[/UserSliderCode__drySlider]
    }
    else if (sliderThatWasMoved == _wetSlider)
    {
        //[UserSliderCode__wetSlider] -- add your slider handling code here..
        const float scale = static_cast<float>(_wetSlider->getValue());
        const float decibels = SnapValue(DecibelScaling::Scale2Db(scale), 0.0f, 0.5f);
        _processor.setParameterNotifyingHost(Parameters::WetDecibels, decibels);
        //[/UserSliderCode__wetSlider]
    }
    else if (sliderThatWasMoved == _hiGainSlider)
    {
        //[UserSliderCode__hiGainSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::EqHighShelfDecibels, SnapValue(static_cast<float>(_hiGainSlider->getValue()), 0.0f, 0.5f));
        //[/UserSliderCode__hiGainSlider]
    }
    else if (sliderThatWasMoved == _hiFreqSlider)
    {
        //[UserSliderCode__hiFreqSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::EqHighShelfFreq, static_cast<float>(_hiFreqSlider->getValue()));
        //[/UserSliderCode__hiFreqSlider]
    }
    else if (sliderThatWasMoved == _loGainSlider)
    {
        //[UserSliderCode__loGainSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::EqLowShelfDecibels, SnapValue(static_cast<float>(_loGainSlider->getValue()), 0.0f, 0.5f));
        //[/UserSliderCode__loGainSlider]
    }
    else if (sliderThatWasMoved == _loFreqSlider)
    {
        //[UserSliderCode__loFreqSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::EqLowShelfFreq, static_cast<float>(_loFreqSlider->getValue()));
        //[/UserSliderCode__loFreqSlider]
    }
    else if (sliderThatWasMoved == _lowCutFreqSlider)
    {
        //[UserSliderCode__lowCutFreqSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::EqLowCutFreq, static_cast<float>(_lowCutFreqSlider->getValue()));
        //[/UserSliderCode__lowCutFreqSlider]
    }
    else if (sliderThatWasMoved == _highCutFreqSlider)
    {
        //[UserSliderCode__highCutFreqSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::EqHighCutFreq, static_cast<float>(_highCutFreqSlider->getValue()));
        //[/UserSliderCode__highCutFreqSlider]
    }
    else if (sliderThatWasMoved == _endSlider)
    {
        //[UserSliderCode__endSlider] -- add your slider handling code here..
        _processor.setIREnd(_endSlider->getValue());
        //[/UserSliderCode__endSlider]
    }
    else if (sliderThatWasMoved == _attackShapeSlider)
    {
        //[UserSliderCode__attackShapeSlider] -- add your slider handling code here..
        _processor.setAttackShape(_attackShapeSlider->getValue());
        //[/UserSliderCode__attackShapeSlider]
    }
    else if (sliderThatWasMoved == _decayShapeSlider)
    {
        //[UserSliderCode__decayShapeSlider] -- add your slider handling code here..
        _processor.setDecayShape(_decayShapeSlider->getValue());
        //[/UserSliderCode__decayShapeSlider]
    }
    else if (sliderThatWasMoved == _beginSlider)
    {
        //[UserSliderCode__beginSlider] -- add your slider handling code here..
        _processor.setIRBegin(_beginSlider->getValue());
        //[/UserSliderCode__beginSlider]
    }
    else if (sliderThatWasMoved == _widthSlider)
    {
        //[UserSliderCode__widthSlider] -- add your slider handling code here..
        _processor.setParameterNotifyingHost(Parameters::StereoWidth, SnapValue(static_cast<float>(_widthSlider->getValue()), 1.0f, 0.05f));
        //[/UserSliderCode__widthSlider]
    }
    else if (sliderThatWasMoved == _predelaySlider)
    {
        //[UserSliderCode__predelaySlider] -- add your slider handling code here..
        _processor.setPredelayMs(_predelaySlider->getValue());
        //[/UserSliderCode__predelaySlider]
    }
    else if (sliderThatWasMoved == _stretchSlider)
    {
        //[UserSliderCode__stretchSlider] -- add your slider handling code here..
        double sliderVal = _stretchSlider->getValue();
        if (::fabs(sliderVal-1.0) < 0.025)
        {
          sliderVal = 1.0;
          _stretchSlider->setValue(1.0, juce::dontSendNotification);
        }
        _processor.setStretch(sliderVal);
        //[/UserSliderCode__stretchSlider]
    }
    else if (sliderThatWasMoved == _attackLengthSlider)
    {
        //[UserSliderCode__attackLengthSlider] -- add your slider handling code here..
        _processor.setAttackLength(_attackLengthSlider->getValue());
        //[/UserSliderCode__attackLengthSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void KlangFalterEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]

    const File presetDirectory = File::getCurrentWorkingDirectory();

    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == _browseButton)
    {
        //[UserButtonCode__browseButton] -- add your button handler code here..
        int browserHeight = 0;
        juce::String browseButtonText;
        if (_browseButton->getToggleState())
        {
          browserHeight = 300;
          browseButtonText = juce::String("Hide Browser");
        }
        else
        {
          browserHeight = 0;
          browseButtonText = juce::String("Show Browser");
        }
        setBounds(getBounds().withHeight(_browseButton->getY() + _browseButton->getHeight() + browserHeight));
        _irBrowserComponent->setBounds(_irBrowserComponent->getBounds().withHeight(browserHeight - 10));
        _browseButton->setButtonText(browseButtonText);
        //[/UserButtonCode__browseButton]
    }
    else if (buttonThatWasClicked == _settingsButton)
    {
        //[UserButtonCode__settingsButton] -- add your button handler code here..
        if (!_settingsDialogWindow)            
        {
        juce::DialogWindow::LaunchOptions launchOptions;
        launchOptions.dialogTitle = juce::String("Settings");
        launchOptions.content.setOwned(new SettingsDialogComponent(_processor));
        launchOptions.componentToCentreAround = this;
        launchOptions.escapeKeyTriggersCloseButton = true;
        launchOptions.useNativeTitleBar = false;
        launchOptions.resizable = false;
        launchOptions.useBottomRightCornerResizer = false;
        _settingsDialogWindow = launchOptions.launchAsync();
        }
        //[/UserButtonCode__settingsButton]
    }
    else if (buttonThatWasClicked == _wetButton)
    {
        //[UserButtonCode__wetButton] -- add your button handler code here..
        _processor.setParameterNotifyingHost(Parameters::WetOn, _wetButton->getToggleState());
        //[/UserButtonCode__wetButton]
    }
    else if (buttonThatWasClicked == _dryButton)
    {
        //[UserButtonCode__dryButton] -- add your button handler code here..
        _processor.setParameterNotifyingHost(Parameters::DryOn, _dryButton->getToggleState());
        //[/UserButtonCode__dryButton]
    }
    else if (buttonThatWasClicked == _autogainButton)
    {
        //[UserButtonCode__autogainButton] -- add your button handler code here..
        _processor.setParameterNotifyingHost(Parameters::AutoGainOn, _autogainButton->getToggleState());
        //[/UserButtonCode__autogainButton]
    }
    else if (buttonThatWasClicked == _reverseButton)
    {
        //[UserButtonCode__reverseButton] -- add your button handler code here..
        _processor.setReverse(_reverseButton->getToggleState());
        //[/UserButtonCode__reverseButton]
    }
    else if (buttonThatWasClicked == _levelMeterOutLabelButton)
    {
        //[UserButtonCode__levelMeterOutLabelButton] -- add your button handler code here..
        _processor.getSettings().setResultLevelMeterDisplay(_levelMeterOutLabelButton->getToggleState() ? Settings::Out : Settings::Wet);
        //[/UserButtonCode__levelMeterOutLabelButton]
    }
    else if (buttonThatWasClicked == _lowEqButton)
    {
        //[UserButtonCode__lowEqButton] -- add your button handler code here..
        const Parameters::EqType lowEqType = static_cast<Parameters::EqType>(_processor.getParameter(Parameters::EqLowType));
        _processor.setParameterNotifyingHost(Parameters::EqLowType, static_cast<int>((lowEqType == Parameters::Cut) ? Parameters::Shelf : Parameters::Cut));
        //[/UserButtonCode__lowEqButton]
    }
    else if (buttonThatWasClicked == _highEqButton)
    {
        //[UserButtonCode__highEqButton] -- add your button handler code here..
        const Parameters::EqType highEqType = static_cast<Parameters::EqType>(_processor.getParameter(Parameters::EqHighType));
        _processor.setParameterNotifyingHost(Parameters::EqHighType, static_cast<int>((highEqType == Parameters::Cut) ? Parameters::Shelf : Parameters::Cut));
        //[/UserButtonCode__highEqButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void KlangFalterEditor::updateUI()
{
  const bool irAvailable = _processor.irAvailable();
  const size_t numInputChannels = static_cast<size_t>(std::min(_processor.getTotalNumInputChannels(), 2));
  const size_t numOutputChannels = static_cast<size_t>(std::min(_processor.getTotalNumOutputChannels(), 2));
  {
    const double stretch = _processor.getStretch();
    _stretchSlider->setEnabled(irAvailable);
    _stretchSlider->setRange(0.5, 1.5);
    _stretchSlider->setValue(stretch, juce::dontSendNotification);
    _stretchLabel->setText(String(static_cast<int>(100.0*stretch)) + String("%"), juce::dontSendNotification);
  }
  {
    const float db = _processor.getParameter(Parameters::DryDecibels);
    const float scale = DecibelScaling::Db2Scale(db);
    _drySlider->setEnabled(true);
    _drySlider->setRange(0.0, 1.0);
    _drySlider->setValue(scale, juce::dontSendNotification);
    _dryLevelLabel->setText(DecibelScaling::DecibelString(db), juce::dontSendNotification);
    _dryButton->setToggleState(_processor.getParameter(Parameters::DryOn), juce::dontSendNotification);
  }
  {
    const float db = _processor.getParameter(Parameters::WetDecibels);
    const float scale = DecibelScaling::Db2Scale(db);
    _wetSlider->setEnabled(true);
    _wetSlider->setRange(0.0, 1.0);
    _wetSlider->setValue(scale, juce::dontSendNotification);
    _wetLevelLabel->setText(DecibelScaling::DecibelString(db), juce::dontSendNotification);
    _wetButton->setToggleState(_processor.getParameter(Parameters::WetOn), juce::dontSendNotification);
  }
  {
    _reverseButton->setEnabled(true);
    _reverseButton->setToggleState(_processor.getReverse(), juce::dontSendNotification);
  }
  {
    const double irBegin = _processor.getIRBegin();
    _beginSlider->setEnabled(irAvailable);
    _beginSlider->setValue(irBegin, juce::dontSendNotification);
    _beginLabel->setText(juce::String(static_cast<int>(100.0 * irBegin)) + juce::String("%"), juce::dontSendNotification);
  }
  {
    const double irEnd = _processor.getIREnd();
    _endSlider->setEnabled(irAvailable);
    _endSlider->setValue(irEnd, juce::dontSendNotification);
    _endLabel->setText(juce::String(static_cast<int>(100.0 * irEnd)) + juce::String("%"), juce::dontSendNotification);
  }
  {
    const double predelayMs = _processor.getPredelayMs();
    _predelaySlider->setValue(predelayMs);
    _predelaySlider->setEnabled(irAvailable);
    _predelayLabel->setText(FormatSeconds(predelayMs / 1000.0), juce::dontSendNotification);
  }
  {
    const double attackLength = _processor.getAttackLength();
    _attackLengthSlider->setValue(attackLength);
    _attackLengthSlider->setEnabled(irAvailable);
    _attackLengthLabel->setText(juce::String(100.0 * attackLength, 1)+juce::String("%"), juce::dontSendNotification);
  }
  {
    const double attackShape = _processor.getAttackShape();
    _attackShapeSlider->setValue(attackShape);
    _attackShapeSlider->setEnabled(irAvailable);
    _attackShapeLabel->setText((attackShape < 0.0001) ? juce::String("Neutral") : juce::String(attackShape, 2), juce::dontSendNotification);
  }
  {
    const double decayShape = _processor.getDecayShape();
    _decayShapeSlider->setValue(decayShape);
    _decayShapeSlider->setEnabled(irAvailable);
    _decayShapeLabel->setText((decayShape < 0.0001) ? juce::String("Neutral") : juce::String(decayShape, 2), juce::dontSendNotification);
  }
  {
    const float autoGainDecibels = _processor.getParameter(Parameters::AutoGainDecibels);
    const juce::String autoGainText = DecibelScaling::DecibelString(autoGainDecibels);
    _autogainButton->setButtonText(juce::String("Autogain ") + autoGainText);
    _autogainButton->setToggleState(_processor.getParameter(Parameters::AutoGainOn), juce::dontSendNotification);
  }
  {
    Parameters::EqType lowEqType = static_cast<Parameters::EqType>(_processor.getParameter(Parameters::EqLowType));
    const bool loEqEnabled = irAvailable;
    const float cutFreq = _processor.getParameter(Parameters::EqLowCutFreq);
    const float shelfFreq = _processor.getParameter(Parameters::EqLowShelfFreq);
    const float shelfGainDb = _processor.getParameter(Parameters::EqLowShelfDecibels);
    _lowEqButton->setButtonText(lowEqType == Parameters::Shelf ? juce::String("Low Shelf") : juce::String("Low Cut"));
    _lowCutFreqHeaderLabel->setVisible(lowEqType == Parameters::Cut);
    _lowCutFreqLabel->setVisible(lowEqType == Parameters::Cut);
    _lowCutFreqLabel->setText((::fabs(cutFreq-Parameters::EqLowCutFreq.getMinValue()) > 0.0001f) ? FormatFrequency(cutFreq) : juce::String("Off"), juce::dontSendNotification);
    _lowCutFreqSlider->setVisible(lowEqType == Parameters::Cut);
    _lowCutFreqSlider->setEnabled(loEqEnabled);
    _lowCutFreqSlider->setValue(cutFreq, juce::dontSendNotification);
    _loFreqHeaderLabel->setVisible(lowEqType == Parameters::Shelf);
    _loFreqLabel->setVisible(lowEqType == Parameters::Shelf);
    _loFreqLabel->setText(FormatFrequency(shelfFreq), juce::dontSendNotification);
    _loFreqSlider->setVisible(lowEqType == Parameters::Shelf);
    _loFreqSlider->setEnabled(loEqEnabled);
    _loFreqSlider->setValue(shelfFreq, juce::dontSendNotification);
    _loGainHeaderLabel->setVisible(lowEqType == Parameters::Shelf);
    _loGainLabel->setVisible(lowEqType == Parameters::Shelf);
    _loGainLabel->setText(DecibelScaling::DecibelString(shelfGainDb), juce::dontSendNotification);
    _loGainSlider->setVisible(lowEqType == Parameters::Shelf);
    _loGainSlider->setEnabled(loEqEnabled);
    _loGainSlider->setValue(shelfGainDb, juce::dontSendNotification);
  }
  {
    Parameters::EqType highEqType = static_cast<Parameters::EqType>(_processor.getParameter(Parameters::EqHighType));
    const bool hiEqEnabled = irAvailable;
    const float cutFreq = _processor.getParameter(Parameters::EqHighCutFreq);
    const float shelfFreq = _processor.getParameter(Parameters::EqHighShelfFreq);
    const float shelfGainDb = _processor.getParameter(Parameters::EqHighShelfDecibels);
    _highEqButton->setButtonText(highEqType == Parameters::Shelf ? juce::String("High Shelf") : juce::String("High Cut"));
    _highCutFreqHeaderLabel->setVisible(highEqType == Parameters::Cut);
    _highCutFreqLabel->setVisible(highEqType == Parameters::Cut);
    _highCutFreqLabel->setText((::fabs(cutFreq-Parameters::EqHighCutFreq.getMaxValue()) > 0.0001f) ? FormatFrequency(cutFreq) : juce::String("Off"), juce::dontSendNotification);
    _highCutFreqSlider->setVisible(highEqType == Parameters::Cut);
    _highCutFreqSlider->setEnabled(hiEqEnabled);
    _highCutFreqSlider->setValue(cutFreq, juce::dontSendNotification);
    _hiFreqHeaderLabel->setVisible(highEqType == Parameters::Shelf);
    _hiFreqLabel->setVisible(highEqType == Parameters::Shelf);
    _hiFreqLabel->setText(FormatFrequency(shelfFreq), juce::dontSendNotification);
    _hiFreqSlider->setVisible(highEqType == Parameters::Shelf);
    _hiFreqSlider->setEnabled(hiEqEnabled);
    _hiFreqSlider->setValue(shelfFreq, juce::dontSendNotification);
    _hiGainHeaderLabel->setVisible(highEqType == Parameters::Shelf);
    _hiGainLabel->setVisible(highEqType == Parameters::Shelf);
    _hiGainLabel->setText(DecibelScaling::DecibelString(shelfGainDb), juce::dontSendNotification);
    _hiGainSlider->setVisible(highEqType == Parameters::Shelf);
    _hiGainSlider->setEnabled(hiEqEnabled);
    _hiGainSlider->setValue(shelfGainDb, juce::dontSendNotification);
  }
  {
    _widthHeaderLabel->setVisible(numOutputChannels >= 2);
    _widthSlider->setEnabled(irAvailable);
    _widthSlider->setVisible(numOutputChannels >= 2);
    _widthLabel->setVisible(numOutputChannels >= 2);
    const float stereoWidth = _processor.getParameter(Parameters::StereoWidth);
    _widthSlider->setValue(stereoWidth, juce::dontSendNotification);
    _widthLabel->setText((::fabs(1.0f-stereoWidth) < 0.001) ? juce::String("Neutral") : juce::String(stereoWidth, 2), juce::dontSendNotification);
  }
  {
    _levelMeterDry->setChannelCount(numInputChannels);
    _levelMeterOut->setChannelCount(numOutputChannels);
    Settings::ResultLevelMeterDisplay resultDisplay = _processor.getSettings().getResultLevelMeterDisplay();
    _levelMeterOutLabelButton->setToggleState(resultDisplay == Settings::Out, juce::dontSendNotification);
    _levelMeterOutLabelButton->setButtonText((resultDisplay == Settings::Out) ? juce::String("Out") : juce::String("Wet"));
  }
  {
    const size_t numTabs = static_cast<size_t>(_irTabComponent->getNumTabs());
    if (numTabs > numInputChannels * numOutputChannels || numTabs != _irComponents.size())
    {
      _irTabComponent->clearTabs();
      _irComponents.clear();
    }
    for (size_t input=0; input<numInputChannels; ++input)
    {
      for (size_t output=0; output<numOutputChannels; ++output)
      {
        if (_irComponents.find(std::make_pair(input, output)) == _irComponents.end())
        {
          IRAgent* agent = _processor.getAgent(input, output);
          jassert(agent);
          if (agent)
          {
            IRComponent* irComponent = new IRComponent();
            irComponent->init(_processor.getAgent(input, output));
            _irTabComponent->addTab(juce::String(static_cast<int>(input+1)) + juce::String("-") + juce::String(static_cast<int>(output+1)),
                                    juce::Colour(0xffb0b0b6),
                                    irComponent,
                                    true);
            _irComponents.insert(std::make_pair(std::make_pair(input, output), irComponent));
          }
        }
      }
    }
  }
}


void KlangFalterEditor::changeListenerCallback(ChangeBroadcaster* source)
{
  if (source)
  {
    updateUI();
  }
}


void KlangFalterEditor::changeNotification()
{
  updateUI();
}


void KlangFalterEditor::timerCallback()
{
  Settings::ResultLevelMeterDisplay resultDisplay = _processor.getSettings().getResultLevelMeterDisplay();
  _levelMeterDry->setLevel(0, _processor.getLevelDry(0));
  _levelMeterDry->setLevel(1, _processor.getLevelDry(1));
  _levelMeterOut->setLevel(0, (resultDisplay == Settings::Out) ? _processor.getLevelOut(0) : _processor.getLevelWet(0));
  _levelMeterOut->setLevel(1, (resultDisplay == Settings::Out) ? _processor.getLevelOut(1) : _processor.getLevelWet(1));
}


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="KlangFalterEditor" componentName=""
                 parentClasses="public AudioProcessorEditor, public ChangeNotifier::Listener, public ChangeListener, public Timer"
                 constructorParams="Processor&amp; processor" variableInitialisers="AudioProcessorEditor(&amp;processor),&#10;_processor(processor)"
                 snapPixels="4" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="1" initialWidth="760" initialHeight="330">
  <BACKGROUND backgroundColour="ffa6a6b1"/>
  <GENERICCOMPONENT name="" id="6dd7ac2ee661b784" memberName="_decibelScaleDry" virtualName=""
                    explicitFocusOrder="0" pos="584 40 32 176" class="DecibelScale"
                    params=""/>
  <TABBEDCOMPONENT name="IRTabComponent" id="697fc3546f1ab7f1" memberName="_irTabComponent"
                   virtualName="" explicitFocusOrder="0" pos="16 12 542 204" orientation="top"
                   tabBarDepth="30" initialTab="0">
    <TAB name="Placeholder" colour="ffb0b0b6" useJucerComp="1" contentClassName=""
         constructorParams="" jucerComponentFile="IRComponent.cpp"/>
  </TABBEDCOMPONENT>
  <GENERICCOMPONENT name="" id="93270230a2db62e0" memberName="_levelMeterDry" virtualName=""
                    explicitFocusOrder="0" pos="632 40 12 176" class="LevelMeter"
                    params=""/>
  <LABEL name="DryLevelLabel" id="892bd8ba7f961215" memberName="_dryLevelLabel"
         virtualName="" explicitFocusOrder="0" pos="572 220 60 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="-inf" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="34"/>
  <LABEL name="WetLevelLabel" id="3469fbc38286d2b6" memberName="_wetLevelLabel"
         virtualName="" explicitFocusOrder="0" pos="656 220 64 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="-inf" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="34"/>
  <SLIDER name="" id="3694f3553dea94b" memberName="_drySlider" virtualName=""
          explicitFocusOrder="0" pos="612 32 24 192" min="0" max="10" int="0"
          style="LinearVertical" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="" id="f3824f7df1d2ea95" memberName="_decibelScaleOut" virtualName=""
                    explicitFocusOrder="0" pos="672 40 32 176" class="DecibelScale"
                    params=""/>
  <SLIDER name="" id="e50054d828347fbd" memberName="_wetSlider" virtualName=""
          explicitFocusOrder="0" pos="700 32 24 192" min="0" max="10" int="0"
          style="LinearVertical" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="e5cc4d9d88fb6d29" memberName="_browseButton" virtualName=""
              explicitFocusOrder="0" pos="12 308 736 24" tooltip="Show Browser For Impulse Response Selection"
              bgColOn="ffbcbcff" buttonText="Show Browser" connectedEdges="8"
              needsCallback="1" radioGroupId="0"/>
  <GENERICCOMPONENT name="" id="5388ff2994f22af6" memberName="_irBrowserComponent"
                    virtualName="" explicitFocusOrder="0" pos="12 332 736 288" class="IRBrowserComponent"
                    params=""/>
  <TEXTBUTTON name="" id="53a50811080a676c" memberName="_settingsButton" virtualName=""
              explicitFocusOrder="0" pos="708 0 52 16" textCol="ff202020" textColOn="ff202020"
              buttonText="Settings" connectedEdges="6" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="c0b279e2bae7030e" memberName="_wetButton" virtualName=""
              explicitFocusOrder="0" pos="684 244 44 24" tooltip="Wet Signal On/Off"
              bgColOff="80bcbcbc" bgColOn="ffbcbcff" textCol="ff202020" textColOn="ff202020"
              buttonText="Wet" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="499237d463b07642" memberName="_dryButton" virtualName=""
              explicitFocusOrder="0" pos="596 244 44 24" tooltip="Dry Signal On/Off"
              bgColOff="80bcbcbc" bgColOn="ffbcbcff" textCol="ff202020" textColOn="ff202020"
              buttonText="Dry" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="f25a3c5b0535fcca" memberName="_autogainButton" virtualName=""
              explicitFocusOrder="0" pos="596 276 132 24" tooltip="Autogain On/Off"
              bgColOff="80bcbcbc" bgColOn="ffbcbcff" textCol="ff202020" textColOn="ff202020"
              buttonText="Autogain 0.0dB" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="" id="fcb6829f0d6fc21f" memberName="_reverseButton" virtualName=""
              explicitFocusOrder="0" pos="486 8 72 24" tooltip="Reverse Impulse Response"
              bgColOff="80bcbcbc" bgColOn="ffbcbcff" textCol="ff202020" textColOn="ff202020"
              buttonText="Reverse" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <LABEL name="" id="841738a894cf241a" memberName="_hiFreqLabel" virtualName=""
         explicitFocusOrder="0" pos="480 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="15.2kHz" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="fc2c2b94ce457ca1" memberName="_hiGainLabel" virtualName=""
         explicitFocusOrder="0" pos="516 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="0.0dB" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="56744cc537b91ad6" memberName="_hiGainHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="516 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Gain" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="71caf1a3b5a498dd" memberName="_hiFreqHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="480 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Freq" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="1c3776d0e6cbce1d" memberName="_hiGainSlider" virtualName=""
          explicitFocusOrder="0" pos="524 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="-30" max="30" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="702db07b37c24f93" memberName="_hiFreqSlider" virtualName=""
          explicitFocusOrder="0" pos="488 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="2000" max="20000" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="" id="8b28430d938b39ca" memberName="_loFreqLabel" virtualName=""
         explicitFocusOrder="0" pos="396 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="1234Hz" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="390bab67dc140d90" memberName="_loGainLabel" virtualName=""
         explicitFocusOrder="0" pos="432 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="0.0dB" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="8d9e4adc7538b7dc" memberName="_loGainHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="432 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Gain" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="f3b3523aee42340f" memberName="_loFreqHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="396 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Freq" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="a3dc7342caaa661f" memberName="_loGainSlider" virtualName=""
          explicitFocusOrder="0" pos="440 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="-30" max="30" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="313e885756edaea8" memberName="_loFreqSlider" virtualName=""
          explicitFocusOrder="0" pos="404 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="20" max="2000" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="" id="e4867bf99a47726a" memberName="_levelMeterOut" virtualName=""
                    explicitFocusOrder="0" pos="720 40 12 176" class="LevelMeter"
                    params=""/>
  <TEXTBUTTON name="" id="f5ed3d758ad2c3f4" memberName="_levelMeterOutLabelButton"
              virtualName="" explicitFocusOrder="0" pos="712 20 28 18" tooltip="Switches Between Out/Wet Level Measurement"
              bgColOff="bbbbff" bgColOn="bcbcff" textCol="ff202020" textColOn="ff202020"
              buttonText="Out" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <LABEL name="" id="55e5b719a217b777" memberName="_levelMeterDryLabel"
         virtualName="" explicitFocusOrder="0" pos="620 16 36 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Dry" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <TEXTBUTTON name="" id="91aebb1e5cd3b857" memberName="_lowEqButton" virtualName=""
              explicitFocusOrder="0" pos="404 220 72 24" bgColOff="bbbbff"
              bgColOn="2c2cff" textCol="ff202020" textColOn="ff202020" buttonText="Low Cut"
              connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <LABEL name="" id="3c3de25483a2083a" memberName="_lowCutFreqLabel" virtualName=""
         explicitFocusOrder="0" pos="414 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="1234Hz" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="e7ff4fcd0be82eec" memberName="_lowCutFreqHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="414 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Freq" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="939a6b5201207a68" memberName="_lowCutFreqSlider"
          virtualName="" explicitFocusOrder="0" pos="422 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="20" max="2000" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="" id="54b3891e3f360022" memberName="_highCutFreqLabel"
         virtualName="" explicitFocusOrder="0" pos="498 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="15.2kHz" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="e765fe8c17a454cf" memberName="_highCutFreqHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="498 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Freq" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="b5ba6600a0fbff4e" memberName="_highCutFreqSlider"
          virtualName="" explicitFocusOrder="0" pos="506 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="2000" max="20000" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="e0f66dc6348e3991" memberName="_highEqButton" virtualName=""
              explicitFocusOrder="0" pos="488 220 72 24" bgColOff="bbbbff"
              bgColOn="2c2cff" textCol="ff202020" textColOn="ff202020" buttonText="High Cut"
              connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <LABEL name="" id="bd223d64f25070f1" memberName="_attackShapeLabel"
         virtualName="" explicitFocusOrder="0" pos="212 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="1.0" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="ee8d936af3b4da1e" memberName="_endLabel" virtualName=""
         explicitFocusOrder="0" pos="76 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="100%" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="69b1c81fb4f7c601" memberName="_endSlider" virtualName=""
          explicitFocusOrder="0" pos="84 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="1" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="4212ad3906b4822c" memberName="_attackShapeSlider"
          virtualName="" explicitFocusOrder="0" pos="220 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="10" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="" id="31ae76799c1691c8" memberName="_decayShapeLabel" virtualName=""
         explicitFocusOrder="0" pos="276 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="1.0" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="23dd6aa127e1d22c" memberName="_decayShapeHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="276 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Shape" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="ec6cfdb461a4ddf" memberName="_decayShapeSlider" virtualName=""
          explicitFocusOrder="0" pos="284 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="10" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="" id="77a4289ec7918eef" memberName="_attackShapeHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="212 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Shape" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="f09d4e48c9272494" memberName="_endHeaderLabel" virtualName=""
         explicitFocusOrder="0" pos="76 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="End" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="83e84ce51a3ad2f0" memberName="_beginLabel" virtualName=""
         explicitFocusOrder="0" pos="40 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="100%" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="abd56bd3093af749" memberName="_beginSlider" virtualName=""
          explicitFocusOrder="0" pos="48 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="1" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="" id="8a8a2c2daac6a39b" memberName="_beginHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="40 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Begin" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="90fac88de886d96b" memberName="_widthLabel" virtualName=""
         explicitFocusOrder="0" pos="340 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="1.0" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="3476a26a68685b9e" memberName="_widthHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="340 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Width" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="31fdb25b044eaf7f" memberName="_widthSlider" virtualName=""
          explicitFocusOrder="0" pos="348 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="10" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.30102"/>
  <LABEL name="" id="d8fd174283fc5f04" memberName="_predelayLabel" virtualName=""
         explicitFocusOrder="0" pos="4 280 52 24" textCol="ff202020" edTextCol="ff000000"
         edBkgCol="0" labelText="0ms" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="11"
         bold="0" italic="0" justification="36"/>
  <LABEL name="" id="c0f7ab9477e45174" memberName="_predelayHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="4 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Gap" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="b8aaa21b836b98e0" memberName="_predelaySlider" virtualName=""
          explicitFocusOrder="0" pos="12 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="1000" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="" id="52624fc555056069" memberName="_stretchLabel" virtualName=""
         explicitFocusOrder="0" pos="112 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="100%" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="5a11a2fc858b9666" memberName="_stretchHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="112 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Stretch" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="6c84a40c942d5b18" memberName="_stretchSlider" virtualName=""
          explicitFocusOrder="0" pos="120 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="2" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="" id="4dd28ec5626fb754" memberName="_attackHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="176 220 88 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Attack" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="a08f22558873cfa8" memberName="_attackLengthLabel"
         virtualName="" explicitFocusOrder="0" pos="176 280 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="0ms" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <SLIDER name="" id="cfcacb5123869692" memberName="_attackLengthSlider"
          virtualName="" explicitFocusOrder="0" pos="184 256 36 28" thumbcol="ffafafff"
          rotarysliderfill="b1606060" min="0" max="1" int="0" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="" id="c1b73cae5819a5a1" memberName="_attackLengthHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="176 236 52 24" textCol="ff202020"
         edTextCol="ff000000" edBkgCol="0" labelText="Length" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="11" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="9ac1cf0b8251e7f3" memberName="_decayHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="256 220 88 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Decay" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="1932084a508f975f" memberName="_impulseResponseHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="12 220 144 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Impulse Response"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="3deee43dc79846d0" memberName="_stereoHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="340 220 52 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Stereo" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
