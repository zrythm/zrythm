/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  18 Jan 2010 1:50:56 pm

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
#include "../DrumSynthPlugin.h"
#include "../DrumSynthComponent.h"
//[/Headers]

#include "DrumSynthMain.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
DrumSynthMain::DrumSynthMain (DrumSynthPlugin* plugin_, DrumSynthComponent* editor_)
    : plugin (plugin_),
      editor (editor_),
      importButton (0),
      exportButton (0),
      currentDrum (0),
      midiKeyboard (0),
      labelDrum (0),
      labelDrumName (0),
      drumName (0),
      versionLabel (0),
      tuningSlider (0),
      stretchSlider (0),
      gainSlider (0),
      resonanceSlider (0),
      filterButton (0),
      hipassButton (0),
      envelopeTabs (0),
      panicButton (0),
      toneLevelSlider (0),
      toneFreq1Slider (0),
      toneFreq2Slider (0),
      toneDroopSlider (0),
      tonePhaseSlider (0),
      toneOnButton (0),
      overOnButton (0),
      overLevelSlider (0),
      overParamSlider (0),
      overFilterButton (0),
      overMethod1Button (0),
      overMethod2Button (0),
      overMethod3Button (0),
      overFreq1Slider (0),
      overFreq2Slider (0),
      overTrack1Button (0),
      overTrack2Button (0),
      overWave1Slider (0),
      overWave2Slider (0),
      band1LevelSlider (0),
      band1FreqSlider (0),
      band1DeltaSlider (0),
      band1OnButton (0),
      band2OnButton (0),
      band2LevelSlider (0),
      band2FreqSlider (0),
      band2DeltaSlider (0),
      noiseOnButton (0),
      noiseLevelSlider (0),
      noiseSlopeSlider (0),
      importBankButton (0),
      exportBankButton (0)
{
    addAndMakeVisible (importButton = new TextButton (String()));
    importButton->setButtonText ("preset");
    importButton->setConnectedEdges (Button::ConnectedOnBottom);
    importButton->addListener (this);
    importButton->setColour (TextButton::buttonColourId, Colour (0xffc8c8c8));

    addAndMakeVisible (exportButton = new TextButton (String()));
    exportButton->setButtonText ("preset");
    exportButton->setConnectedEdges (Button::ConnectedOnBottom);
    exportButton->addListener (this);
    exportButton->setColour (TextButton::buttonColourId, Colour (0xff7c7c7c));

    addAndMakeVisible (currentDrum = new Slider (String()));
    currentDrum->setRange (0, 24, 1);
    currentDrum->setSliderStyle (Slider::IncDecButtons);
    currentDrum->setTextBoxStyle (Slider::TextBoxLeft, false, 40, 20);
    currentDrum->setColour (Slider::thumbColourId, Colours::antiquewhite);
    currentDrum->setColour (Slider::rotarySliderFillColourId, Colours::antiquewhite);
    currentDrum->setColour (Slider::textBoxHighlightColourId, Colours::antiquewhite);
    currentDrum->addListener (this);

    addAndMakeVisible (midiKeyboard = new DrumSynthKeyboard (this,
                                                             *(plugin->getKeyboardState())));

    addAndMakeVisible (labelDrum = new Label (String(),
                                              "Drum\n"));
    labelDrum->setFont (Font (13.6000f, Font::bold | Font::italic));
    labelDrum->setJustificationType (Justification::centredLeft);
    labelDrum->setEditable (false, false, false);
    labelDrum->setColour (Label::backgroundColourId, Colour (0xffffff));
    labelDrum->setColour (Label::textColourId, Colours::black);
    labelDrum->setColour (Label::outlineColourId, Colour (0x808080));
    labelDrum->setColour (TextEditor::textColourId, Colours::black);
    labelDrum->setColour (TextEditor::backgroundColourId, Colour (0xffffff));

    addAndMakeVisible (labelDrumName = new Label (String(),
                                                  "Preset"));
    labelDrumName->setFont (Font (13.6000f, Font::bold | Font::italic));
    labelDrumName->setJustificationType (Justification::centredLeft);
    labelDrumName->setEditable (false, false, false);
    labelDrumName->setColour (Label::backgroundColourId, Colour (0xffffff));
    labelDrumName->setColour (Label::outlineColourId, Colour (0x808080));
    labelDrumName->setColour (TextEditor::textColourId, Colours::black);
    labelDrumName->setColour (TextEditor::backgroundColourId, Colour (0xffffff));

    addAndMakeVisible (drumName = new TextEditor (String()));
    drumName->setMultiLine (false);
    drumName->setReturnKeyStartsNewLine (false);
    drumName->setReadOnly (false);
    drumName->setScrollbarsShown (false);
    drumName->setCaretVisible (true);
    drumName->setPopupMenuEnabled (false);
    drumName->setText ("Unset");

    addAndMakeVisible (versionLabel = new Label (String(),
                                                 "v0.1.0"));
    versionLabel->setFont (Font (Font::getDefaultMonospacedFontName(), 9.3000f, Font::plain));
    versionLabel->setJustificationType (Justification::centredLeft);
    versionLabel->setEditable (false, false, false);
    versionLabel->setColour (TextEditor::textColourId, Colours::black);
    versionLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (tuningSlider = new ParameterSlider (String()));
    tuningSlider->setTooltip ("Tuning");
    tuningSlider->setRange (-24, 24, 0.0001);
    tuningSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    tuningSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    tuningSlider->setColour (Slider::thumbColourId, Colours::white);
    tuningSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff69c369));
    tuningSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    tuningSlider->addListener (this);

    addAndMakeVisible (stretchSlider = new ParameterSlider (String()));
    stretchSlider->setTooltip ("Stretch");
    stretchSlider->setRange (10, 200, 0.0001);
    stretchSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    stretchSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    stretchSlider->setColour (Slider::thumbColourId, Colours::white);
    stretchSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff69c369));
    stretchSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    stretchSlider->addListener (this);

    addAndMakeVisible (gainSlider = new ParameterSlider (String()));
    gainSlider->setTooltip ("Overall gain");
    gainSlider->setRange (-60, 10, 0.0001);
    gainSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    gainSlider->setColour (Slider::thumbColourId, Colours::white);
    gainSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff69c369));
    gainSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    gainSlider->addListener (this);

    addAndMakeVisible (resonanceSlider = new ParameterSlider (String()));
    resonanceSlider->setTooltip ("Filter resonance");
    resonanceSlider->setRange (0, 100, 0.0001);
    resonanceSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    resonanceSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    resonanceSlider->setColour (Slider::thumbColourId, Colours::white);
    resonanceSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff69c369));
    resonanceSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    resonanceSlider->addListener (this);

    addAndMakeVisible (filterButton = new ParameterLedButton (String()));
    filterButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    filterButton->addListener (this);
    filterButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (hipassButton = new ParameterLedButton (String()));
    hipassButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    hipassButton->addListener (this);
    hipassButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (envelopeTabs = new TabbedComponent (TabbedButtonBar::TabsAtBottom));
    envelopeTabs->setTabBarDepth (21);
    envelopeTabs->addTab ("Tone", Colour (0xff9ba6da), new DrumSynthEnvelope (PP_TONE_ENV_T1TIME, this, plugin), true);
    envelopeTabs->addTab ("Overtone 1", Colour (0xffd9def3), new DrumSynthEnvelope (PP_OTON1_ENV_T1TIME, this, plugin), true);
    envelopeTabs->addTab ("Overtone 2", Colour (0xffd9def3), new DrumSynthEnvelope (PP_OTON2_ENV_T1TIME, this, plugin), true);
    envelopeTabs->addTab ("Noise", Colour (0xfffdde0c), new DrumSynthEnvelope (PP_NOIZ_ENV_T1TIME, this, plugin), true);
    envelopeTabs->addTab ("Band 1", Colour (0xfff2843d), new DrumSynthEnvelope (PP_NBA1_ENV_T1TIME, this, plugin), true);
    envelopeTabs->addTab ("Band 2", Colour (0xfff2843d), new DrumSynthEnvelope (PP_NBA2_ENV_T1TIME, this, plugin), true);
    envelopeTabs->addTab ("Filter", Colour (0xff69c369), new DrumSynthEnvelope (PP_MAIN_ENV_T1TIME, this, plugin), true);
    envelopeTabs->setCurrentTabIndex (0);

    addAndMakeVisible (panicButton = new TextButton (String()));
    panicButton->setButtonText ("Panic");
    panicButton->addListener (this);
    panicButton->setColour (TextButton::buttonColourId, Colour (0xffedb292));

    addAndMakeVisible (toneLevelSlider = new ParameterSlider (String()));
    toneLevelSlider->setTooltip ("Tone Level");
    toneLevelSlider->setRange (0, 200, 0.0001);
    toneLevelSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    toneLevelSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    toneLevelSlider->setColour (Slider::thumbColourId, Colours::white);
    toneLevelSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff9ba6da));
    toneLevelSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    toneLevelSlider->addListener (this);

    addAndMakeVisible (toneFreq1Slider = new ParameterSlider (String()));
    toneFreq1Slider->setTooltip ("Tone Frequency 2");
    toneFreq1Slider->setRange (20, 22050, 0.0001);
    toneFreq1Slider->setSliderStyle (Slider::RotaryVerticalDrag);
    toneFreq1Slider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    toneFreq1Slider->setColour (Slider::thumbColourId, Colours::white);
    toneFreq1Slider->setColour (Slider::rotarySliderFillColourId, Colour (0xff9ba6da));
    toneFreq1Slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    toneFreq1Slider->addListener (this);

    addAndMakeVisible (toneFreq2Slider = new ParameterSlider (String()));
    toneFreq2Slider->setTooltip ("Tone Frequency 2");
    toneFreq2Slider->setRange (20, 22050, 0.0001);
    toneFreq2Slider->setSliderStyle (Slider::RotaryVerticalDrag);
    toneFreq2Slider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    toneFreq2Slider->setColour (Slider::thumbColourId, Colours::white);
    toneFreq2Slider->setColour (Slider::rotarySliderFillColourId, Colour (0xff9ba6da));
    toneFreq2Slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    toneFreq2Slider->addListener (this);

    addAndMakeVisible (toneDroopSlider = new ParameterSlider (String()));
    toneDroopSlider->setTooltip ("Tone Droop");
    toneDroopSlider->setRange (0, 100, 0.0001);
    toneDroopSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    toneDroopSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    toneDroopSlider->setColour (Slider::thumbColourId, Colours::white);
    toneDroopSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff9ba6da));
    toneDroopSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    toneDroopSlider->addListener (this);

    addAndMakeVisible (tonePhaseSlider = new ParameterSlider (String()));
    tonePhaseSlider->setTooltip ("Phase");
    tonePhaseSlider->setRange (0, 90, 0.0001);
    tonePhaseSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    tonePhaseSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    tonePhaseSlider->setColour (Slider::thumbColourId, Colours::white);
    tonePhaseSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff9ba6da));
    tonePhaseSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    tonePhaseSlider->addListener (this);

    addAndMakeVisible (toneOnButton = new ParameterLedButton (String()));
    toneOnButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    toneOnButton->addListener (this);
    toneOnButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overOnButton = new ParameterLedButton (String()));
    overOnButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overOnButton->addListener (this);
    overOnButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overLevelSlider = new ParameterSlider (String()));
    overLevelSlider->setTooltip ("Overtone Level");
    overLevelSlider->setRange (0, 200, 0.0001);
    overLevelSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    overLevelSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    overLevelSlider->setColour (Slider::thumbColourId, Colours::white);
    overLevelSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xffd9def3));
    overLevelSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    overLevelSlider->addListener (this);

    addAndMakeVisible (overParamSlider = new ParameterSlider (String()));
    overParamSlider->setTooltip ("Overtone Param");
    overParamSlider->setRange (0, 100, 0.0001);
    overParamSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    overParamSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    overParamSlider->setColour (Slider::thumbColourId, Colours::white);
    overParamSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xffd9def3));
    overParamSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    overParamSlider->addListener (this);

    addAndMakeVisible (overFilterButton = new ParameterLedButton (String()));
    overFilterButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overFilterButton->addListener (this);
    overFilterButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overMethod1Button = new ParameterLedButton (String()));
    overMethod1Button->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overMethod1Button->addListener (this);
    overMethod1Button->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overMethod2Button = new ParameterLedButton (String()));
    overMethod2Button->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overMethod2Button->addListener (this);
    overMethod2Button->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overMethod3Button = new ParameterLedButton (String()));
    overMethod3Button->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overMethod3Button->addListener (this);
    overMethod3Button->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overFreq1Slider = new ParameterSlider (String()));
    overFreq1Slider->setTooltip ("Overtone 1 Frequency");
    overFreq1Slider->setRange (20, 22050, 0.0001);
    overFreq1Slider->setSliderStyle (Slider::RotaryVerticalDrag);
    overFreq1Slider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    overFreq1Slider->setColour (Slider::thumbColourId, Colours::white);
    overFreq1Slider->setColour (Slider::rotarySliderFillColourId, Colour (0xffd9def3));
    overFreq1Slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    overFreq1Slider->addListener (this);

    addAndMakeVisible (overFreq2Slider = new ParameterSlider (String()));
    overFreq2Slider->setTooltip ("Overtone 2 Frequency");
    overFreq2Slider->setRange (20, 22050, 0.0001);
    overFreq2Slider->setSliderStyle (Slider::RotaryVerticalDrag);
    overFreq2Slider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    overFreq2Slider->setColour (Slider::thumbColourId, Colours::white);
    overFreq2Slider->setColour (Slider::rotarySliderFillColourId, Colour (0xffd9def3));
    overFreq2Slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    overFreq2Slider->addListener (this);

    addAndMakeVisible (overTrack1Button = new ParameterLedButton (String()));
    overTrack1Button->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overTrack1Button->addListener (this);
    overTrack1Button->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overTrack2Button = new ParameterLedButton (String()));
    overTrack2Button->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    overTrack2Button->addListener (this);
    overTrack2Button->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (overWave1Slider = new ParameterSlider (String()));
    overWave1Slider->setTooltip ("Overtone 1 Wave");
    overWave1Slider->setRange (0, 4, 1);
    overWave1Slider->setSliderStyle (Slider::RotaryVerticalDrag);
    overWave1Slider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    overWave1Slider->setColour (Slider::thumbColourId, Colours::white);
    overWave1Slider->setColour (Slider::rotarySliderFillColourId, Colour (0xffd9def3));
    overWave1Slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    overWave1Slider->addListener (this);

    addAndMakeVisible (overWave2Slider = new ParameterSlider (String()));
    overWave2Slider->setTooltip ("Overtone 2 Wave");
    overWave2Slider->setRange (0, 4, 1);
    overWave2Slider->setSliderStyle (Slider::RotaryVerticalDrag);
    overWave2Slider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    overWave2Slider->setColour (Slider::thumbColourId, Colours::white);
    overWave2Slider->setColour (Slider::rotarySliderFillColourId, Colour (0xffd9def3));
    overWave2Slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    overWave2Slider->addListener (this);

    addAndMakeVisible (band1LevelSlider = new ParameterSlider (String()));
    band1LevelSlider->setTooltip ("Noise Band 1 Level");
    band1LevelSlider->setRange (0, 200, 0.0001);
    band1LevelSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    band1LevelSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    band1LevelSlider->setColour (Slider::thumbColourId, Colours::white);
    band1LevelSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfff2843d));
    band1LevelSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    band1LevelSlider->addListener (this);

    addAndMakeVisible (band1FreqSlider = new ParameterSlider (String()));
    band1FreqSlider->setTooltip ("Noise Band 1 Frequency");
    band1FreqSlider->setRange (20, 22050, 0.0001);
    band1FreqSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    band1FreqSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    band1FreqSlider->setColour (Slider::thumbColourId, Colours::white);
    band1FreqSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfff2843d));
    band1FreqSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    band1FreqSlider->addListener (this);

    addAndMakeVisible (band1DeltaSlider = new ParameterSlider (String()));
    band1DeltaSlider->setTooltip ("Noise Band 1 Delta Frequency");
    band1DeltaSlider->setRange (0, 100, 0.0001);
    band1DeltaSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    band1DeltaSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    band1DeltaSlider->setColour (Slider::thumbColourId, Colours::white);
    band1DeltaSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfff2843d));
    band1DeltaSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    band1DeltaSlider->addListener (this);

    addAndMakeVisible (band1OnButton = new ParameterLedButton (String()));
    band1OnButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    band1OnButton->addListener (this);
    band1OnButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (band2OnButton = new ParameterLedButton (String()));
    band2OnButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    band2OnButton->addListener (this);
    band2OnButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (band2LevelSlider = new ParameterSlider (String()));
    band2LevelSlider->setTooltip ("Noise Band 2 Level");
    band2LevelSlider->setRange (0, 200, 0.0001);
    band2LevelSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    band2LevelSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    band2LevelSlider->setColour (Slider::thumbColourId, Colours::white);
    band2LevelSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfff2843d));
    band2LevelSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    band2LevelSlider->addListener (this);

    addAndMakeVisible (band2FreqSlider = new ParameterSlider (String()));
    band2FreqSlider->setTooltip ("Noise Band 2 Frequency");
    band2FreqSlider->setRange (20, 22050, 0.0001);
    band2FreqSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    band2FreqSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    band2FreqSlider->setColour (Slider::thumbColourId, Colours::white);
    band2FreqSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfff2843d));
    band2FreqSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    band2FreqSlider->addListener (this);

    addAndMakeVisible (band2DeltaSlider = new ParameterSlider (String()));
    band2DeltaSlider->setTooltip ("Noise Band 2 Delta Frequency");
    band2DeltaSlider->setRange (0, 100, 0.0001);
    band2DeltaSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    band2DeltaSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    band2DeltaSlider->setColour (Slider::thumbColourId, Colours::white);
    band2DeltaSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfff2843d));
    band2DeltaSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    band2DeltaSlider->addListener (this);

    addAndMakeVisible (noiseOnButton = new ParameterLedButton (String()));
    noiseOnButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    noiseOnButton->addListener (this);
    noiseOnButton->setColour (TextButton::buttonColourId, Colour (0xab000000));

    addAndMakeVisible (noiseLevelSlider = new ParameterSlider (String()));
    noiseLevelSlider->setTooltip ("Noise Level");
    noiseLevelSlider->setRange (0, 200, 0.0001);
    noiseLevelSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    noiseLevelSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    noiseLevelSlider->setColour (Slider::thumbColourId, Colours::white);
    noiseLevelSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfffdde0c));
    noiseLevelSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    noiseLevelSlider->addListener (this);

    addAndMakeVisible (noiseSlopeSlider = new ParameterSlider (String()));
    noiseSlopeSlider->setTooltip ("Noise Slope");
    noiseSlopeSlider->setRange (-100, 100, 1);
    noiseSlopeSlider->setSliderStyle (Slider::RotaryVerticalDrag);
    noiseSlopeSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    noiseSlopeSlider->setColour (Slider::thumbColourId, Colours::white);
    noiseSlopeSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xfffdde0c));
    noiseSlopeSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    noiseSlopeSlider->addListener (this);

    addAndMakeVisible (importBankButton = new TextButton (String()));
    importBankButton->setButtonText ("bank");
    importBankButton->setConnectedEdges (Button::ConnectedOnTop);
    importBankButton->addListener (this);
    importBankButton->setColour (TextButton::buttonColourId, Colour (0xffc8c8c8));

    addAndMakeVisible (exportBankButton = new TextButton (String()));
    exportBankButton->setButtonText ("bank");
    exportBankButton->setConnectedEdges (Button::ConnectedOnTop);
    exportBankButton->addListener (this);
    exportBankButton->setColour (TextButton::buttonColourId, Colour (0xff7c7c7c));


    //[UserPreSize]
    //[/UserPreSize]

    setSize (682, 320);

    //[Constructor] You can add your own custom stuff here..
    versionLabel->setText (JucePlugin_VersionString, dontSendNotification);

    filterButton->setCurrentValues (0.0f, 1.0f);
    hipassButton->setCurrentValues (0.0f, 1.0f);
    toneOnButton->setCurrentValues (0.0f, 1.0f);
    overOnButton->setCurrentValues (0.0f, 1.0f);
    overFilterButton->setCurrentValues (0.0f, 1.0f);
    overMethod1Button->setCurrentValues (0.0f, 0.0f);
    overMethod2Button->setCurrentValues (0.0f, 1.0f);
    overMethod3Button->setCurrentValues (0.0f, 2.0f);
    overTrack1Button->setCurrentValues (0.0f, 1.0f);
    overTrack2Button->setCurrentValues (0.0f, 1.0f);
    band1OnButton->setCurrentValues (0.0f, 1.0f);
    band2OnButton->setCurrentValues (0.0f, 1.0f);
    noiseOnButton->setCurrentValues (0.0f, 1.0f);

    toneFreq1Slider->setSkewFactorFromMidPoint (440.0);
    toneFreq2Slider->setSkewFactorFromMidPoint (440.0);
    overFreq1Slider->setSkewFactorFromMidPoint (440.0);
    overFreq2Slider->setSkewFactorFromMidPoint (440.0);
    band1FreqSlider->setSkewFactorFromMidPoint (440.0);
    band2FreqSlider->setSkewFactorFromMidPoint (440.0);

    drumName->addListener (this);

    midiKeyboard->setScrollButtonsVisible (false);
    midiKeyboard->setKeyWidth (midiKeyboard->getWidth() / (TOTAL_DRUM_OCTAVES * 7));
    midiKeyboard->setOctaveForMiddleC (START_DRUM_NOTES_OFFSET / 12);
    midiKeyboard->setLowestVisibleKey (START_DRUM_NOTES_OFFSET);
    midiKeyboard->setAvailableRange (START_DRUM_NOTES_OFFSET,
                                     START_DRUM_NOTES_OFFSET + TOTAL_DRUM_NOTES);

    currentDrum->setRange (0, TOTAL_DRUM_NOTES, 1);

    DBG ("Current Drum On Opening: " + String (plugin->getCurrentDrum()));

    setCurrentDrumNumber (plugin->getCurrentDrum(), true);
    //[/Constructor]
}

DrumSynthMain::~DrumSynthMain()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

    setCurrentDrumNumber (-1, false);

    //[/Destructor_pre]

    deleteAndZero (importButton);
    deleteAndZero (exportButton);
    deleteAndZero (currentDrum);
    deleteAndZero (midiKeyboard);
    deleteAndZero (labelDrum);
    deleteAndZero (labelDrumName);
    deleteAndZero (drumName);
    deleteAndZero (versionLabel);
    deleteAndZero (tuningSlider);
    deleteAndZero (stretchSlider);
    deleteAndZero (gainSlider);
    deleteAndZero (resonanceSlider);
    deleteAndZero (filterButton);
    deleteAndZero (hipassButton);
    deleteAndZero (envelopeTabs);
    deleteAndZero (panicButton);
    deleteAndZero (toneLevelSlider);
    deleteAndZero (toneFreq1Slider);
    deleteAndZero (toneFreq2Slider);
    deleteAndZero (toneDroopSlider);
    deleteAndZero (tonePhaseSlider);
    deleteAndZero (toneOnButton);
    deleteAndZero (overOnButton);
    deleteAndZero (overLevelSlider);
    deleteAndZero (overParamSlider);
    deleteAndZero (overFilterButton);
    deleteAndZero (overMethod1Button);
    deleteAndZero (overMethod2Button);
    deleteAndZero (overMethod3Button);
    deleteAndZero (overFreq1Slider);
    deleteAndZero (overFreq2Slider);
    deleteAndZero (overTrack1Button);
    deleteAndZero (overTrack2Button);
    deleteAndZero (overWave1Slider);
    deleteAndZero (overWave2Slider);
    deleteAndZero (band1LevelSlider);
    deleteAndZero (band1FreqSlider);
    deleteAndZero (band1DeltaSlider);
    deleteAndZero (band1OnButton);
    deleteAndZero (band2OnButton);
    deleteAndZero (band2LevelSlider);
    deleteAndZero (band2FreqSlider);
    deleteAndZero (band2DeltaSlider);
    deleteAndZero (noiseOnButton);
    deleteAndZero (noiseLevelSlider);
    deleteAndZero (noiseSlopeSlider);
    deleteAndZero (importBankButton);
    deleteAndZero (exportBankButton);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void DrumSynthMain::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setGradientFill (ColourGradient (Colour (0xff515151),
                                       232.0f, 320.0f,
                                       Colour (0xffa4a4a4),
                                       231.0f, 79.0f,
                                       false));
    g.fillRect (0, 64, 682, 256);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (5.0f, 163.0f, 192.0f, 153.0f, 6.5000f);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (200.0f, 86.0f, 192.0f, 74.0f, 6.5000f);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (395.0f, 86.0f, 282.0f, 74.0f, 6.5000f);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (395.0f, 163.0f, 282.0f, 153.0f, 6.5000f);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (200.0f, 242.0f, 192.0f, 74.0f, 6.5000f);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (200.0f, 163.0f, 192.0f, 76.0f, 6.5000f);

    g.setGradientFill (ColourGradient (Colour (0xffa4a4a4),
                                       232.0f, 24.0f,
                                       Colour (0xffc7c7c7),
                                       232.0f, 8.0f,
                                       false));
    g.fillRect (0, 0, 682, 64);

    g.setColour (Colour (0xff737272));
    g.setFont (Font (Font::getDefaultMonospacedFontName(), 35.7000f, Font::bold));
    g.drawText ("DRUMSYNTH",
                1, 2, 184, 32,
                Justification::centred, true);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (5.0f, 31.0f, 673.0f, 52.0f, 6.5000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Noise",
                204, 90, 36, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Noise Band 1",
                204, 167, 86, 13,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Noise Band 2",
                204, 246, 86, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Overtones",
                9, 167, 73, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Master",
                399, 90, 45, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Envelopes",
                399, 167, 66, 14,
                Justification::centred, true);

    g.setColour (Colour (0x34323232));
    g.fillRoundedRectangle (5.0f, 86.0f, 192.0f, 74.0f, 6.5000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (14.3000f, Font::bold | Font::italic));
    g.drawText ("Tone",
                9, 89, 32, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Gain",
                9, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Freq1",
                47, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Freq2",
                83, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Droop",
                118, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Phase",
                155, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Activity",
                138, 89, 45, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Activity",
                138, 166, 45, 14,
                Justification::centred, true);

    g.setColour (Colour (0x47ffffff));
    g.fillRoundedRectangle (101.0f, 201.0f, 73.0f, 12.0f, 3.0000f);

    g.setColour (Colour (0x47ffffff));
    g.fillRoundedRectangle (101.0f, 188.0f, 73.0f, 12.0f, 3.0000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (10.0000f, Font::bold | Font::italic));
    g.drawText ("Filter",
                136, 187, 38, 14,
                Justification::centredLeft, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (10.0000f, Font::bold | Font::italic));
    g.drawText ("Method",
                136, 200, 38, 14,
                Justification::centredLeft, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Gain",
                9, 214, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Param",
                46, 214, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Freq1",
                11, 258, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Freq2",
                11, 301, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x47ffffff));
    g.fillRoundedRectangle (101.0f, 233.0f, 73.0f, 12.0f, 3.0000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (10.0000f, Font::bold | Font::italic));
    g.drawText ("Track 1",
                136, 232, 38, 14,
                Justification::centredLeft, true);

    g.setColour (Colour (0x47ffffff));
    g.fillRoundedRectangle (101.0f, 275.0f, 73.0f, 12.0f, 3.0000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (10.0000f, Font::bold | Font::italic));
    g.drawText ("Track 2",
                136, 274, 38, 14,
                Justification::centredLeft, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Wave1",
                48, 258, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Wave2",
                47, 301, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Gain",
                236, 219, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Freq",
                273, 219, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("deltaF",
                310, 219, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Activity",
                333, 166, 45, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Activity",
                332, 246, 45, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Gain",
                237, 297, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Freq",
                274, 297, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("deltaF",
                311, 297, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Activity",
                334, 89, 45, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Gain",
                253, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Gain",
                399, 139, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Tune",
                434, 139, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Stretch",
                470, 139, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Reso",
                506, 139, 39, 14,
                Justification::centred, true);

    g.setColour (Colour (0x47ffffff));
    g.fillRoundedRectangle (571.0f, 112.0f, 73.0f, 12.0f, 3.0000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (10.0000f, Font::bold | Font::italic));
    g.drawText ("Filter",
                606, 111, 38, 14,
                Justification::centredLeft, true);

    g.setColour (Colour (0x47ffffff));
    g.fillRoundedRectangle (571.0f, 125.0f, 73.0f, 12.0f, 3.0000f);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (10.0000f, Font::bold | Font::italic));
    g.drawText ("Hipass",
                606, 124, 38, 14,
                Justification::centredLeft, true);

    g.setColour (Colour (0x99ffffff));
    g.setFont (Font (11.0000f, Font::bold));
    g.drawText ("Slope",
                290, 138, 39, 14,
                Justification::centred, true);

    g.setColour (Colours::black);
    g.setFont (Font (11.2000f, Font::bold | Font::italic));
    g.drawText ("Import   Export",
                585, 34, 89, 14,
                Justification::centred, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void DrumSynthMain::resized()
{
    importButton->setBounds (585, 47, 44, 17);
    exportButton->setBounds (630, 47, 44, 17);
    currentDrum->setBounds (52, 35, 74, 16);
    midiKeyboard->setBounds (10, 55, 572, 24);
    labelDrum->setBounds (8, 35, 41, 16);
    labelDrumName->setBounds (136, 35, 45, 16);
    drumName->setBounds (185, 35, 397, 16);
    versionLabel->setBounds (179, 22, 34, 8);
    tuningSlider->setBounds (437, 109, 32, 32);
    stretchSlider->setBounds (472, 109, 32, 32);
    gainSlider->setBounds (402, 109, 32, 32);
    resonanceSlider->setBounds (508, 109, 32, 32);
    filterButton->setBounds (575, 114, 26, 8);
    hipassButton->setBounds (575, 127, 26, 8);
    envelopeTabs->setBounds (397, 183, 278, 135);
    panicButton->setBounds (629, 7, 44, 17);
    toneLevelSlider->setBounds (13, 109, 32, 32);
    toneFreq1Slider->setBounds (49, 109, 32, 32);
    toneFreq2Slider->setBounds (85, 109, 32, 32);
    toneDroopSlider->setBounds (121, 109, 32, 32);
    tonePhaseSlider->setBounds (157, 109, 32, 32);
    toneOnButton->setBounds (183, 92, 8, 8);
    overOnButton->setBounds (183, 169, 8, 8);
    overLevelSlider->setBounds (13, 186, 32, 32);
    overParamSlider->setBounds (49, 186, 32, 32);
    overFilterButton->setBounds (105, 190, 26, 8);
    overMethod1Button->setBounds (105, 203, 8, 8);
    overMethod2Button->setBounds (114, 203, 8, 8);
    overMethod3Button->setBounds (123, 203, 8, 8);
    overFreq1Slider->setBounds (13, 229, 32, 32);
    overFreq2Slider->setBounds (13, 273, 32, 32);
    overTrack1Button->setBounds (105, 235, 26, 8);
    overTrack2Button->setBounds (105, 277, 26, 8);
    overWave1Slider->setBounds (49, 229, 32, 32);
    overWave2Slider->setBounds (49, 273, 32, 32);
    band1LevelSlider->setBounds (241, 189, 32, 32);
    band1FreqSlider->setBounds (277, 189, 32, 32);
    band1DeltaSlider->setBounds (313, 189, 32, 32);
    band1OnButton->setBounds (378, 169, 8, 8);
    band2OnButton->setBounds (377, 249, 8, 8);
    band2LevelSlider->setBounds (241, 267, 32, 32);
    band2FreqSlider->setBounds (277, 267, 32, 32);
    band2DeltaSlider->setBounds (313, 267, 32, 32);
    noiseOnButton->setBounds (378, 92, 8, 8);
    noiseLevelSlider->setBounds (258, 109, 32, 32);
    noiseSlopeSlider->setBounds (294, 109, 32, 32);
    importBankButton->setBounds (585, 63, 44, 17);
    exportBankButton->setBounds (630, 63, 44, 17);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void DrumSynthMain::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    const int currentDrumNumber = plugin->getCurrentDrum();
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == importButton)
    {
        //[UserButtonCode_importButton] -- add your button handler code here..
        FileChooser myChooser ("Import a DS file...", File(), "*.ds", false);
        if (myChooser.browseForFileToOpen())
        {
            File fileToLoad = myChooser.getResult();
            if (fileToLoad.existsAsFile())
            {
                plugin->importDS (plugin->getCurrentDrum(), fileToLoad);
                plugin->setLastBrowsedDirectory (fileToLoad);
            }
        }
        //[/UserButtonCode_importButton]
    }
    else if (buttonThatWasClicked == exportButton)
    {
        //[UserButtonCode_exportButton] -- add your button handler code here..
        FileChooser myChooser ("Export a DS file...", File(), "*.ds", false);
        if (myChooser.browseForFileToSave(true))
        {
            File fileToSave = myChooser.getResult().withFileExtension ("ds");

            plugin->exportDS (plugin->getCurrentDrum(), fileToSave);
            plugin->setLastBrowsedDirectory (fileToSave);
        }
        //[/UserButtonCode_exportButton]
    }
    else if (buttonThatWasClicked == filterButton)
    {
        //[UserButtonCode_filterButton] -- add your button handler code here..
        filterButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_MAIN_FILTER), filterButton->getCurrentValue());
        //[/UserButtonCode_filterButton]
    }
    else if (buttonThatWasClicked == hipassButton)
    {
        //[UserButtonCode_hipassButton] -- add your button handler code here..
        hipassButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_MAIN_HIGHPASS), hipassButton->getCurrentValue());
        //[/UserButtonCode_hipassButton]
    }
    else if (buttonThatWasClicked == panicButton)
    {
        //[UserButtonCode_panicButton] -- add your button handler code here..
        plugin->triggerPanic ();
        //[/UserButtonCode_panicButton]
    }
    else if (buttonThatWasClicked == toneOnButton)
    {
        //[UserButtonCode_toneOnButton] -- add your button handler code here..
        toneOnButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_TONE_ON), toneOnButton->getCurrentValue());
        //[/UserButtonCode_toneOnButton]
    }
    else if (buttonThatWasClicked == overOnButton)
    {
        //[UserButtonCode_overOnButton] -- add your button handler code here..
        overOnButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_ON), overOnButton->getCurrentValue());
        //[/UserButtonCode_overOnButton]
    }
    else if (buttonThatWasClicked == overFilterButton)
    {
        //[UserButtonCode_overFilterButton] -- add your button handler code here..
        overFilterButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_FILTER), overFilterButton->getCurrentValue());
        //[/UserButtonCode_overFilterButton]
    }
    else if (buttonThatWasClicked == overMethod1Button)
    {
        //[UserButtonCode_overMethod1Button] -- add your button handler code here..
        if (! overMethod1Button->isActivated ())
        {
            overMethod1Button->setActivated (true);
            overMethod2Button->setActivated (false);
            overMethod3Button->setActivated (false);
            plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_METHOD), overMethod1Button->getCurrentValue());
        }
        //[/UserButtonCode_overMethod1Button]
    }
    else if (buttonThatWasClicked == overMethod2Button)
    {
        //[UserButtonCode_overMethod2Button] -- add your button handler code here..
        if (! overMethod2Button->isActivated ())
        {
            overMethod1Button->setActivated (false);
            overMethod2Button->setActivated (true);
            overMethod3Button->setActivated (false);
            plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_METHOD), overMethod2Button->getCurrentValue());
        }
        //[/UserButtonCode_overMethod2Button]
    }
    else if (buttonThatWasClicked == overMethod3Button)
    {
        //[UserButtonCode_overMethod3Button] -- add your button handler code here..
        if (! overMethod3Button->isActivated ())
        {
            overMethod1Button->setActivated (false);
            overMethod2Button->setActivated (false);
            overMethod3Button->setActivated (true);
            plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_METHOD), overMethod3Button->getCurrentValue());
        }
        //[/UserButtonCode_overMethod3Button]
    }
    else if (buttonThatWasClicked == overTrack1Button)
    {
        //[UserButtonCode_overTrack1Button] -- add your button handler code here..
        overTrack1Button->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_TRACK1), overTrack1Button->getCurrentValue());
        //[/UserButtonCode_overTrack1Button]
    }
    else if (buttonThatWasClicked == overTrack2Button)
    {
        //[UserButtonCode_overTrack2Button] -- add your button handler code here..
        overTrack2Button->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_TRACK2), overTrack2Button->getCurrentValue());
        //[/UserButtonCode_overTrack2Button]
    }
    else if (buttonThatWasClicked == band1OnButton)
    {
        //[UserButtonCode_band1OnButton] -- add your button handler code here..
        band1OnButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA1_ON), band1OnButton->getCurrentValue());
        //[/UserButtonCode_band1OnButton]
    }
    else if (buttonThatWasClicked == band2OnButton)
    {
        //[UserButtonCode_band2OnButton] -- add your button handler code here..
        band2OnButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA2_ON), band2OnButton->getCurrentValue());
        //[/UserButtonCode_band2OnButton]
    }
    else if (buttonThatWasClicked == noiseOnButton)
    {
        //[UserButtonCode_noiseOnButton] -- add your button handler code here..
        noiseOnButton->toggleActivated ();
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NOIZ_ON), noiseOnButton->getCurrentValue());
        //[/UserButtonCode_noiseOnButton]
    }
    else if (buttonThatWasClicked == importBankButton)
    {
        //[UserButtonCode_importBankButton] -- add your button handler code here..
        FileChooser myChooser ("Import a DS bank file...", File(), "*.bds", false);
        if (myChooser.browseForFileToOpen())
        {
            MemoryBlock fileData;
            File fileToLoad = myChooser.getResult();

            if (fileToLoad.existsAsFile()
                && fileToLoad.loadFileAsData (fileData))
            {
                plugin->setStateInformation (fileData.getData (), fileData.getSize());
                plugin->setLastBrowsedDirectory (fileToLoad);
            }
        }
        //[/UserButtonCode_importBankButton]
    }
    else if (buttonThatWasClicked == exportBankButton)
    {
        //[UserButtonCode_exportBankButton] -- add your button handler code here..
        FileChooser myChooser ("Save a DS bank file...", File(), "*.bds", false);
        if (myChooser.browseForFileToSave (true))
        {
            MemoryBlock fileData;
            plugin->getStateInformation (fileData);

            File fileToSave = myChooser.getResult().withFileExtension ("bds");
            if (fileToSave.replaceWithData (fileData.getData (), fileData.getSize()))
            {
                plugin->setLastBrowsedDirectory (fileToSave);
            }
        }
        //[/UserButtonCode_exportBankButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void DrumSynthMain::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    Slider* slider = sliderThatWasMoved;
    const int currentDrumNumber = plugin->getCurrentDrum();
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == currentDrum)
    {
        //[UserSliderCode_currentDrum] -- add your slider handling code here..
        setCurrentDrumNumber (roundFloatToInt (slider->getValue ()), true);
        //[/UserSliderCode_currentDrum]
    }
    else if (sliderThatWasMoved == tuningSlider)
    {
        //[UserSliderCode_tuningSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_MAIN_TUNING), slider->getValue());
        //[/UserSliderCode_tuningSlider]
    }
    else if (sliderThatWasMoved == stretchSlider)
    {
        //[UserSliderCode_stretchSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_MAIN_STRETCH), slider->getValue());
        //[/UserSliderCode_stretchSlider]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_MAIN_GAIN), slider->getValue());
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == resonanceSlider)
    {
        //[UserSliderCode_resonanceSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_MAIN_RESONANCE), slider->getValue());
        //[/UserSliderCode_resonanceSlider]
    }
    else if (sliderThatWasMoved == toneLevelSlider)
    {
        //[UserSliderCode_toneLevelSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_TONE_LEVEL), slider->getValue());
        //[/UserSliderCode_toneLevelSlider]
    }
    else if (sliderThatWasMoved == toneFreq1Slider)
    {
        //[UserSliderCode_toneFreq1Slider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_TONE_F1), slider->getValue());
        //[/UserSliderCode_toneFreq1Slider]
    }
    else if (sliderThatWasMoved == toneFreq2Slider)
    {
        //[UserSliderCode_toneFreq2Slider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_TONE_F2), slider->getValue());
        //[/UserSliderCode_toneFreq2Slider]
    }
    else if (sliderThatWasMoved == toneDroopSlider)
    {
        //[UserSliderCode_toneDroopSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_TONE_DROOP), slider->getValue());
        //[/UserSliderCode_toneDroopSlider]
    }
    else if (sliderThatWasMoved == tonePhaseSlider)
    {
        //[UserSliderCode_tonePhaseSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_TONE_PHASE), slider->getValue());
        //[/UserSliderCode_tonePhaseSlider]
    }
    else if (sliderThatWasMoved == overLevelSlider)
    {
        //[UserSliderCode_overLevelSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_LEVEL), slider->getValue());
        //[/UserSliderCode_overLevelSlider]
    }
    else if (sliderThatWasMoved == overParamSlider)
    {
        //[UserSliderCode_overParamSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_PARAM), slider->getValue());
        //[/UserSliderCode_overParamSlider]
    }
    else if (sliderThatWasMoved == overFreq1Slider)
    {
        //[UserSliderCode_overFreq1Slider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_F1), slider->getValue());
        //[/UserSliderCode_overFreq1Slider]
    }
    else if (sliderThatWasMoved == overFreq2Slider)
    {
        //[UserSliderCode_overFreq2Slider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_F2), slider->getValue());
        //[/UserSliderCode_overFreq2Slider]
    }
    else if (sliderThatWasMoved == overWave1Slider)
    {
        //[UserSliderCode_overWave1Slider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_WAVE1), slider->getValue());
        //[/UserSliderCode_overWave1Slider]
    }
    else if (sliderThatWasMoved == overWave2Slider)
    {
        //[UserSliderCode_overWave2Slider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_OTON_WAVE2), slider->getValue());
        //[/UserSliderCode_overWave2Slider]
    }
    else if (sliderThatWasMoved == band1LevelSlider)
    {
        //[UserSliderCode_band1LevelSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA1_LEVEL), slider->getValue());
        //[/UserSliderCode_band1LevelSlider]
    }
    else if (sliderThatWasMoved == band1FreqSlider)
    {
        //[UserSliderCode_band1FreqSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA1_F), slider->getValue());
        //[/UserSliderCode_band1FreqSlider]
    }
    else if (sliderThatWasMoved == band1DeltaSlider)
    {
        //[UserSliderCode_band1DeltaSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA1_DF), slider->getValue());
        //[/UserSliderCode_band1DeltaSlider]
    }
    else if (sliderThatWasMoved == band2LevelSlider)
    {
        //[UserSliderCode_band2LevelSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA2_LEVEL), slider->getValue());
        //[/UserSliderCode_band2LevelSlider]
    }
    else if (sliderThatWasMoved == band2FreqSlider)
    {
        //[UserSliderCode_band2FreqSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA2_F), slider->getValue());
        //[/UserSliderCode_band2FreqSlider]
    }
    else if (sliderThatWasMoved == band2DeltaSlider)
    {
        //[UserSliderCode_band2DeltaSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NBA2_DF), slider->getValue());
        //[/UserSliderCode_band2DeltaSlider]
    }
    else if (sliderThatWasMoved == noiseLevelSlider)
    {
        //[UserSliderCode_noiseLevelSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NOIZ_LEVEL), slider->getValue());
        //[/UserSliderCode_noiseLevelSlider]
    }
    else if (sliderThatWasMoved == noiseSlopeSlider)
    {
        //[UserSliderCode_noiseSlopeSlider] -- add your slider handling code here..
        plugin->setParameterMappedNotifyingHost (PPAR (currentDrumNumber, PP_NOIZ_SLOPE), slider->getValue());
        //[/UserSliderCode_noiseSlopeSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

//==============================================================================
void DrumSynthMain::textEditorTextChanged (TextEditor &editor)
{
    plugin->setDrumName (plugin->getCurrentDrum(), editor.getText ());
}

void DrumSynthMain::textEditorReturnKeyPressed (TextEditor &editor)
{
}

void DrumSynthMain::textEditorEscapeKeyPressed (TextEditor &editor)
{
}

void DrumSynthMain::textEditorFocusLost (TextEditor &editor)
{
}

//==============================================================================
void DrumSynthMain::setCurrentDrumNumber (const int newDrumNumber, const bool updateAllControls)
{
    int currentDrumNumber = plugin->getCurrentDrum();

    if (currentDrumNumber >= 0 && currentDrumNumber < TOTAL_DRUM_NOTES)
    {
        plugin->getParameterLock().enter ();
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_TUNING), tuningSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_STRETCH), stretchSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_GAIN), gainSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_RESONANCE), resonanceSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_FILTER), filterButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_HIGHPASS), hipassButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_TONE_ON), toneOnButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_TONE_LEVEL), toneLevelSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_TONE_F1), toneFreq1Slider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_TONE_F2), toneFreq2Slider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_TONE_DROOP), toneDroopSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_TONE_PHASE), tonePhaseSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_ON), noiseOnButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_LEVEL), noiseLevelSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_SLOPE), noiseSlopeSlider);
//            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_FIXEDSEQ), noiseFixedSeqButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_ON), overOnButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_FILTER), overFilterButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_LEVEL), overLevelSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_F1), overFreq1Slider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_WAVE1), overWave1Slider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_TRACK1), overTrack1Button);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_F2), overFreq2Slider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_WAVE2), overWave2Slider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_TRACK2), overTrack2Button);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_PARAM), overParamSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_METHOD), overMethod1Button);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_METHOD), overMethod2Button);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_OTON_METHOD), overMethod3Button);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_ON), band1OnButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_LEVEL), band1LevelSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_F), band1FreqSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_DF), band1DeltaSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_ON), band2OnButton);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_LEVEL), band2LevelSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_F), band2FreqSlider);
            plugin->removeListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_DF), band2DeltaSlider);

        plugin->getParameterLock().exit ();
    }

    if (newDrumNumber >= 0 && newDrumNumber < TOTAL_DRUM_NOTES)
    {
        plugin->setCurrentDrum(newDrumNumber);
        currentDrumNumber = plugin->getCurrentDrum();

        if (currentDrumNumber >= 0 && currentDrumNumber < TOTAL_DRUM_NOTES)
        {
            plugin->getParameterLock().enter ();
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_TUNING), tuningSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_STRETCH), stretchSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_GAIN), gainSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_RESONANCE), resonanceSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_FILTER), filterButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_MAIN_HIGHPASS), hipassButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_TONE_ON), toneOnButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_TONE_LEVEL), toneLevelSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_TONE_F1), toneFreq1Slider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_TONE_F2), toneFreq2Slider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_TONE_DROOP), toneDroopSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_TONE_PHASE), tonePhaseSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_ON), noiseOnButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_LEVEL), noiseLevelSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_SLOPE), noiseSlopeSlider);
    //            plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NOIZ_FIXEDSEQ), noiseFixedSeqButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_ON), overOnButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_FILTER), overFilterButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_LEVEL), overLevelSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_F1), overFreq1Slider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_WAVE1), overWave1Slider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_TRACK1), overTrack1Button);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_F2), overFreq2Slider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_WAVE2), overWave2Slider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_TRACK2), overTrack2Button);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_PARAM), overParamSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_METHOD), overMethod1Button);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_METHOD), overMethod2Button);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_OTON_METHOD), overMethod3Button);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_ON), band1OnButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_LEVEL), band1LevelSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_F), band1FreqSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA1_DF), band1DeltaSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_ON), band2OnButton);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_LEVEL), band2LevelSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_F), band2FreqSlider);
                plugin->addListenerToParameter (PPAR(currentDrumNumber, PP_NBA2_DF), band2DeltaSlider);

            plugin->getParameterLock().exit ();
        }
    }

    if (updateAllControls)
        updateControls ();
}

void DrumSynthMain::updateControls ()
{
    int currentDrumNumber = plugin->getCurrentDrum();
    if (currentDrumNumber >= 0 && currentDrumNumber < TOTAL_DRUM_NOTES)
    {
        tuningSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_MAIN_TUNING)), dontSendNotification);
        stretchSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_MAIN_STRETCH)), dontSendNotification);
        gainSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_MAIN_GAIN)), dontSendNotification);
        resonanceSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_MAIN_RESONANCE)), dontSendNotification);
        toneLevelSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_TONE_LEVEL)), dontSendNotification);
        toneFreq1Slider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_TONE_F1)), dontSendNotification);
        toneFreq2Slider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_TONE_F2)), dontSendNotification);
        toneDroopSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_TONE_DROOP)), dontSendNotification);
        tonePhaseSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_TONE_PHASE)), dontSendNotification);
        noiseLevelSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NOIZ_LEVEL)), dontSendNotification);
        overLevelSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_LEVEL)), dontSendNotification);
        overFreq1Slider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_F1)), dontSendNotification);
        overWave1Slider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_WAVE1)), dontSendNotification);
        overFreq2Slider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_F2)), dontSendNotification);
        overWave2Slider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_WAVE2)), dontSendNotification);
        overParamSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_PARAM)), dontSendNotification);
        band1LevelSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA1_LEVEL)), dontSendNotification);
        band1FreqSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA1_F)), dontSendNotification);
        band1DeltaSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA1_DF)), dontSendNotification);
        band2LevelSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA2_LEVEL)), dontSendNotification);
        band2FreqSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA2_F)), dontSendNotification);
        band2DeltaSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA2_DF)), dontSendNotification);
        noiseSlopeSlider->setValue (plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NOIZ_SLOPE)), dontSendNotification);

        filterButton->setActivated (filterButton->getActivateValue ()
                                    == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_MAIN_FILTER)));
        hipassButton->setActivated (hipassButton->getActivateValue ()
                                    == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_MAIN_HIGHPASS)));
        toneOnButton->setActivated (toneOnButton->getActivateValue ()
                                    == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_TONE_ON)));
        noiseOnButton->setActivated (noiseOnButton->getActivateValue ()
                                     == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NOIZ_ON)));
//        noiseFixedSeqButton->setActivated (noiseFixedSeqButton->getActivateValue ()
//                                     == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NOIZ_FIXEDSEQ)));
        overOnButton->setActivated (overOnButton->getActivateValue ()
                                    == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_ON)));
        overTrack1Button->setActivated (overTrack1Button->getActivateValue ()
                                        == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_TRACK1)));
        overTrack2Button->setActivated (overTrack2Button->getActivateValue ()
                                        == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_TRACK2)));
        overFilterButton->setActivated (overFilterButton->getActivateValue ()
                                        == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_FILTER)));
        overMethod1Button->setActivated (overMethod1Button->getActivateValue ()
                                         == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_METHOD)));
        overMethod2Button->setActivated (overMethod2Button->getActivateValue ()
                                         == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_METHOD)));
        overMethod3Button->setActivated (overMethod3Button->getActivateValue ()
                                         == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_OTON_METHOD)));
        band1OnButton->setActivated (band1OnButton->getActivateValue ()
                                     == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA1_ON)));
        band2OnButton->setActivated (band2OnButton->getActivateValue ()
                                     == plugin->getParameterMapped (PPAR(currentDrumNumber, PP_NBA2_ON)));

        DrumSynthEnvelope* env = 0;
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (0); env->updateParameters (true);
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (1); env->updateParameters (true);
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (2); env->updateParameters (true);
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (3); env->updateParameters (true);
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (4); env->updateParameters (true);
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (5); env->updateParameters (true);
        env = (DrumSynthEnvelope*) envelopeTabs->getTabContentComponent (6); env->updateParameters (true);

        currentDrum->setValue (currentDrumNumber, dontSendNotification);
        drumName->setText (plugin->getDrumName (currentDrumNumber), false);
        midiKeyboard->setCurrentNoteNumber (currentDrumNumber + START_DRUM_NOTES_OFFSET);
    }
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="DrumSynthMain" componentName=""
                 parentClasses="public Component, public TextEditorListener" constructorParams="DrumSynthPlugin* plugin_, DrumSynthComponent* editor_"
                 variableInitialisers="plugin (plugin_),&#10;editor (editor_)" snapPixels="8"
                 snapActive="0" snapShown="1" overlayOpacity="0.330000013" fixedSize="1"
                 initialWidth="682" initialHeight="320">
  <BACKGROUND backgroundColour="a4a4a4">
    <RECT pos="0 64 682 256" fill="linear: 232 320, 231 79, 0=ff515151, 1=ffa4a4a4"
          hasStroke="0"/>
    <ROUNDRECT pos="5 163 192 153" cornerSize="6.5" fill="solid: 34323232" hasStroke="0"/>
    <ROUNDRECT pos="200 86 192 74" cornerSize="6.5" fill="solid: 34323232" hasStroke="0"/>
    <ROUNDRECT pos="395 86 282 74" cornerSize="6.5" fill="solid: 34323232" hasStroke="0"/>
    <ROUNDRECT pos="395 163 282 153" cornerSize="6.5" fill="solid: 34323232"
               hasStroke="0"/>
    <ROUNDRECT pos="200 242 192 74" cornerSize="6.5" fill="solid: 34323232"
               hasStroke="0"/>
    <ROUNDRECT pos="200 163 192 76" cornerSize="6.5" fill="solid: 34323232"
               hasStroke="0"/>
    <RECT pos="0 0 682 64" fill="linear: 232 24, 232 8, 0=ffa4a4a4, 1=ffc7c7c7"
          hasStroke="0"/>
    <TEXT pos="1 2 184 32" fill="solid: ff737272" hasStroke="0" text="DRUMSYNTH"
          fontname="Default monospaced font" fontsize="35.7" bold="1" italic="0"
          justification="36"/>
    <ROUNDRECT pos="5 31 673 52" cornerSize="6.5" fill="solid: 34323232" hasStroke="0"/>
    <TEXT pos="204 90 36 14" fill="solid: 99ffffff" hasStroke="0" text="Noise"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <TEXT pos="204 167 86 13" fill="solid: 99ffffff" hasStroke="0" text="Noise Band 1"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <TEXT pos="204 246 86 14" fill="solid: 99ffffff" hasStroke="0" text="Noise Band 2"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <TEXT pos="9 167 73 14" fill="solid: 99ffffff" hasStroke="0" text="Overtones"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <TEXT pos="399 90 45 14" fill="solid: 99ffffff" hasStroke="0" text="Master"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <TEXT pos="399 167 66 14" fill="solid: 99ffffff" hasStroke="0" text="Envelopes"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <ROUNDRECT pos="5 86 192 74" cornerSize="6.5" fill="solid: 34323232" hasStroke="0"/>
    <TEXT pos="9 89 32 14" fill="solid: 99ffffff" hasStroke="0" text="Tone"
          fontname="Default font" fontsize="14.3" bold="1" italic="1" justification="36"/>
    <TEXT pos="9 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Gain"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="47 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Freq1"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="83 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Freq2"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="118 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Droop"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="155 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Phase"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="138 89 45 14" fill="solid: 99ffffff" hasStroke="0" text="Activity"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="138 166 45 14" fill="solid: 99ffffff" hasStroke="0" text="Activity"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <ROUNDRECT pos="101 201 73 12" cornerSize="3" fill="solid: 47ffffff" hasStroke="0"/>
    <ROUNDRECT pos="101 188 73 12" cornerSize="3" fill="solid: 47ffffff" hasStroke="0"/>
    <TEXT pos="136 187 38 14" fill="solid: 99ffffff" hasStroke="0" text="Filter"
          fontname="Default font" fontsize="10" bold="1" italic="1" justification="33"/>
    <TEXT pos="136 200 38 14" fill="solid: 99ffffff" hasStroke="0" text="Method"
          fontname="Default font" fontsize="10" bold="1" italic="1" justification="33"/>
    <TEXT pos="9 214 39 14" fill="solid: 99ffffff" hasStroke="0" text="Gain"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="46 214 39 14" fill="solid: 99ffffff" hasStroke="0" text="Param"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="11 258 39 14" fill="solid: 99ffffff" hasStroke="0" text="Freq1"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="11 301 39 14" fill="solid: 99ffffff" hasStroke="0" text="Freq2"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <ROUNDRECT pos="101 233 73 12" cornerSize="3" fill="solid: 47ffffff" hasStroke="0"/>
    <TEXT pos="136 232 38 14" fill="solid: 99ffffff" hasStroke="0" text="Track 1"
          fontname="Default font" fontsize="10" bold="1" italic="1" justification="33"/>
    <ROUNDRECT pos="101 275 73 12" cornerSize="3" fill="solid: 47ffffff" hasStroke="0"/>
    <TEXT pos="136 274 38 14" fill="solid: 99ffffff" hasStroke="0" text="Track 2"
          fontname="Default font" fontsize="10" bold="1" italic="1" justification="33"/>
    <TEXT pos="48 258 39 14" fill="solid: 99ffffff" hasStroke="0" text="Wave1"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="47 301 39 14" fill="solid: 99ffffff" hasStroke="0" text="Wave2"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="236 219 39 14" fill="solid: 99ffffff" hasStroke="0" text="Gain"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="273 219 39 14" fill="solid: 99ffffff" hasStroke="0" text="Freq"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="310 219 39 14" fill="solid: 99ffffff" hasStroke="0" text="deltaF"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="333 166 45 14" fill="solid: 99ffffff" hasStroke="0" text="Activity"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="332 246 45 14" fill="solid: 99ffffff" hasStroke="0" text="Activity"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="237 297 39 14" fill="solid: 99ffffff" hasStroke="0" text="Gain"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="274 297 39 14" fill="solid: 99ffffff" hasStroke="0" text="Freq"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="311 297 39 14" fill="solid: 99ffffff" hasStroke="0" text="deltaF"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="334 89 45 14" fill="solid: 99ffffff" hasStroke="0" text="Activity"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="253 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Gain"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="399 139 39 14" fill="solid: 99ffffff" hasStroke="0" text="Gain"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="434 139 39 14" fill="solid: 99ffffff" hasStroke="0" text="Tune"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="470 139 39 14" fill="solid: 99ffffff" hasStroke="0" text="Stretch"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="506 139 39 14" fill="solid: 99ffffff" hasStroke="0" text="Reso"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <ROUNDRECT pos="571 112 73 12" cornerSize="3" fill="solid: 47ffffff" hasStroke="0"/>
    <TEXT pos="606 111 38 14" fill="solid: 99ffffff" hasStroke="0" text="Filter"
          fontname="Default font" fontsize="10" bold="1" italic="1" justification="33"/>
    <ROUNDRECT pos="571 125 73 12" cornerSize="3" fill="solid: 47ffffff" hasStroke="0"/>
    <TEXT pos="606 124 38 14" fill="solid: 99ffffff" hasStroke="0" text="Hipass"
          fontname="Default font" fontsize="10" bold="1" italic="1" justification="33"/>
    <TEXT pos="290 138 39 14" fill="solid: 99ffffff" hasStroke="0" text="Slope"
          fontname="Default font" fontsize="11" bold="1" italic="0" justification="36"/>
    <TEXT pos="585 34 89 14" fill="solid: ff000000" hasStroke="0" text="Import   Export"
          fontname="Default font" fontsize="11.2" bold="1" italic="1" justification="36"/>
  </BACKGROUND>
  <TEXTBUTTON name="" id="fdcc73833aecefc8" memberName="importButton" virtualName=""
              explicitFocusOrder="0" pos="585 47 44 17" bgColOff="ffc8c8c8"
              buttonText="preset" connectedEdges="8" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="dff5afec9dbccca3" memberName="exportButton" virtualName=""
              explicitFocusOrder="0" pos="630 47 44 17" bgColOff="ff7c7c7c"
              buttonText="preset" connectedEdges="8" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="" id="1aa9aa47577f660d" memberName="currentDrum" virtualName=""
          explicitFocusOrder="0" pos="52 35 74 16" thumbcol="fffaebd7"
          rotarysliderfill="fffaebd7" textboxhighlight="fffaebd7" min="0"
          max="24" int="1" style="IncDecButtons" textBoxPos="TextBoxLeft"
          textBoxEditable="1" textBoxWidth="40" textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="" id="34112f3d024b9329" memberName="midiKeyboard" virtualName=""
                    explicitFocusOrder="0" pos="10 55 572 24" class="DrumSynthKeyboard"
                    params="this,&#10;*(plugin-&gt;getKeyboardState())"/>
  <LABEL name="" id="b78ed9645da3f589" memberName="labelDrum" virtualName=""
         explicitFocusOrder="0" pos="8 35 41 16" bkgCol="ffffff" textCol="ff000000"
         outlineCol="808080" edTextCol="ff000000" edBkgCol="ffffff" labelText="Drum&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="13.6" bold="1" italic="1" justification="33"/>
  <LABEL name="" id="5276f6acfecf726d" memberName="labelDrumName" virtualName=""
         explicitFocusOrder="0" pos="136 35 45 16" bkgCol="ffffff" outlineCol="808080"
         edTextCol="ff000000" edBkgCol="ffffff" labelText="Preset" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="13.6" bold="1" italic="1" justification="33"/>
  <TEXTEDITOR name="" id="1705484c1a6bfd89" memberName="drumName" virtualName=""
              explicitFocusOrder="0" pos="185 35 397 16" initialText="Unset"
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="0"
              caret="1" popupmenu="0"/>
  <LABEL name="" id="fb57b6afa7d98a90" memberName="versionLabel" virtualName=""
         explicitFocusOrder="0" pos="179 22 34 8" edTextCol="ff000000"
         edBkgCol="0" labelText="v0.1.0" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default monospaced font" fontsize="9.3"
         bold="0" italic="0" justification="33"/>
  <SLIDER name="" id="42f95b1ead745959" memberName="tuningSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="437 109 32 32" tooltip="Tuning" thumbcol="ffffffff"
          rotarysliderfill="ff69c369" rotaryslideroutline="66000000" min="-24"
          max="24" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="a674b622130506cc" memberName="stretchSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="472 109 32 32" tooltip="Stretch"
          thumbcol="ffffffff" rotarysliderfill="ff69c369" rotaryslideroutline="66000000"
          min="10" max="200" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="bfe7817a3420d4b2" memberName="gainSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="402 109 32 32" tooltip="Overall gain"
          thumbcol="ffffffff" rotarysliderfill="ff69c369" rotaryslideroutline="66000000"
          min="-60" max="10" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="37a935c2a5f14e32" memberName="resonanceSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="508 109 32 32" tooltip="Filter resonance"
          thumbcol="ffffffff" rotarysliderfill="ff69c369" rotaryslideroutline="66000000"
          min="0" max="100" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="839fbb0e779a18ca" memberName="filterButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="575 114 26 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="7a5d17516e175f4" memberName="hipassButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="575 127 26 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TABBEDCOMPONENT name="" id="62f679a508042b2c" memberName="envelopeTabs" virtualName=""
                   explicitFocusOrder="0" pos="397 183 278 135" orientation="bottom"
                   tabBarDepth="21" initialTab="0">
    <TAB name="Tone" colour="ff9ba6da" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_TONE_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
    <TAB name="Overtone 1" colour="ffd9def3" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_OTON1_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
    <TAB name="Overtone 2" colour="ffd9def3" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_OTON2_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
    <TAB name="Noise" colour="fffdde0c" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_NOIZ_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
    <TAB name="Band 1" colour="fff2843d" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_NBA1_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
    <TAB name="Band 2" colour="fff2843d" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_NBA2_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
    <TAB name="Filter" colour="ff69c369" useJucerComp="0" contentClassName="DrumSynthEnvelope"
         constructorParams="PP_MAIN_ENV_T1TIME, this, plugin" jucerComponentFile=""/>
  </TABBEDCOMPONENT>
  <TEXTBUTTON name="" id="e0b8d1f5b14f35bc" memberName="panicButton" virtualName=""
              explicitFocusOrder="0" pos="629 7 44 17" bgColOff="ffedb292"
              buttonText="Panic" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="" id="34ce85c848c03034" memberName="toneLevelSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="13 109 32 32" tooltip="Tone Level"
          thumbcol="ffffffff" rotarysliderfill="ff9ba6da" rotaryslideroutline="66000000"
          min="0" max="200" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="b787f6cb8182ae16" memberName="toneFreq1Slider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="49 109 32 32" tooltip="Tone Frequency 2"
          thumbcol="ffffffff" rotarysliderfill="ff9ba6da" rotaryslideroutline="66000000"
          min="20" max="22050" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="bcb848a086c99100" memberName="toneFreq2Slider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="85 109 32 32" tooltip="Tone Frequency 2"
          thumbcol="ffffffff" rotarysliderfill="ff9ba6da" rotaryslideroutline="66000000"
          min="20" max="22050" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="4316b24d5c80d151" memberName="toneDroopSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="121 109 32 32" tooltip="Tone Droop"
          thumbcol="ffffffff" rotarysliderfill="ff9ba6da" rotaryslideroutline="66000000"
          min="0" max="100" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="dab3f63d0cdbd294" memberName="tonePhaseSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="157 109 32 32" tooltip="Phase" thumbcol="ffffffff"
          rotarysliderfill="ff9ba6da" rotaryslideroutline="66000000" min="0"
          max="90" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="44484c43b0719f3c" memberName="toneOnButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="183 92 8 8" bgColOff="ab000000" buttonText=""
              connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="894be190527106fc" memberName="overOnButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="183 169 8 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="" id="ad5c6ac27e18f82a" memberName="overLevelSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="13 186 32 32" tooltip="Overtone Level"
          thumbcol="ffffffff" rotarysliderfill="ffd9def3" rotaryslideroutline="66000000"
          min="0" max="200" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="860444b16d2244fd" memberName="overParamSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="49 186 32 32" tooltip="Overtone Param"
          thumbcol="ffffffff" rotarysliderfill="ffd9def3" rotaryslideroutline="66000000"
          min="0" max="100" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="6ae8862999d4d42c" memberName="overFilterButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="105 190 26 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="7133cd96bce69f04" memberName="overMethod1Button"
              virtualName="ParameterLedButton" explicitFocusOrder="0" pos="105 203 8 8"
              bgColOff="ab000000" buttonText="" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="" id="dffa590e74a47080" memberName="overMethod2Button"
              virtualName="ParameterLedButton" explicitFocusOrder="0" pos="114 203 8 8"
              bgColOff="ab000000" buttonText="" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="" id="f689bf2d77a9da8b" memberName="overMethod3Button"
              virtualName="ParameterLedButton" explicitFocusOrder="0" pos="123 203 8 8"
              bgColOff="ab000000" buttonText="" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <SLIDER name="" id="2d53e7190b05cc73" memberName="overFreq1Slider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="13 229 32 32" tooltip="Overtone 1 Frequency"
          thumbcol="ffffffff" rotarysliderfill="ffd9def3" rotaryslideroutline="66000000"
          min="20" max="22050" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="3c1d2cf02d9a36f8" memberName="overFreq2Slider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="13 273 32 32" tooltip="Overtone 2 Frequency"
          thumbcol="ffffffff" rotarysliderfill="ffd9def3" rotaryslideroutline="66000000"
          min="20" max="22050" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="713bcfe5f3c421e6" memberName="overTrack1Button" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="105 235 26 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="ce573d2382f8ecc3" memberName="overTrack2Button" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="105 277 26 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="" id="ab915dc252a33c93" memberName="overWave1Slider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="49 229 32 32" tooltip="Overtone 1 Wave"
          thumbcol="ffffffff" rotarysliderfill="ffd9def3" rotaryslideroutline="66000000"
          min="0" max="4" int="1" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="f544c782aa3130b3" memberName="overWave2Slider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="49 273 32 32" tooltip="Overtone 2 Wave"
          thumbcol="ffffffff" rotarysliderfill="ffd9def3" rotaryslideroutline="66000000"
          min="0" max="4" int="1" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="99cf60e1358775d" memberName="band1LevelSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="241 189 32 32" tooltip="Noise Band 1 Level"
          thumbcol="ffffffff" rotarysliderfill="fff2843d" rotaryslideroutline="66000000"
          min="0" max="200" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="5b1eaf793a6bce62" memberName="band1FreqSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="277 189 32 32" tooltip="Noise Band 1 Frequency"
          thumbcol="ffffffff" rotarysliderfill="fff2843d" rotaryslideroutline="66000000"
          min="20" max="22050" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="b8b200a7d0c15324" memberName="band1DeltaSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="313 189 32 32" tooltip="Noise Band 1 Delta Frequency"
          thumbcol="ffffffff" rotarysliderfill="fff2843d" rotaryslideroutline="66000000"
          min="0" max="100" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="148f32350185a9dc" memberName="band1OnButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="378 169 8 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="9fa142ecfde99ee2" memberName="band2OnButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="377 249 8 8" bgColOff="ab000000"
              buttonText="" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="" id="4ea60e7f75ef17d8" memberName="band2LevelSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="241 267 32 32" tooltip="Noise Band 2 Level"
          thumbcol="ffffffff" rotarysliderfill="fff2843d" rotaryslideroutline="66000000"
          min="0" max="200" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="8073242450f30e0b" memberName="band2FreqSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="277 267 32 32" tooltip="Noise Band 2 Frequency"
          thumbcol="ffffffff" rotarysliderfill="fff2843d" rotaryslideroutline="66000000"
          min="20" max="22050" int="0.0001" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="c394221bef11329f" memberName="band2DeltaSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="313 267 32 32" tooltip="Noise Band 2 Delta Frequency"
          thumbcol="ffffffff" rotarysliderfill="fff2843d" rotaryslideroutline="66000000"
          min="0" max="100" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="d6ce47eed26966ac" memberName="noiseOnButton" virtualName="ParameterLedButton"
              explicitFocusOrder="0" pos="378 92 8 8" bgColOff="ab000000" buttonText=""
              connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="" id="7e86c020206f02e7" memberName="noiseLevelSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="258 109 32 32" tooltip="Noise Level"
          thumbcol="ffffffff" rotarysliderfill="fffdde0c" rotaryslideroutline="66000000"
          min="0" max="200" int="0.0001" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="" id="81edad0da42d0181" memberName="noiseSlopeSlider" virtualName="ParameterSlider"
          explicitFocusOrder="0" pos="294 109 32 32" tooltip="Noise Slope"
          thumbcol="ffffffff" rotarysliderfill="fffdde0c" rotaryslideroutline="66000000"
          min="-100" max="100" int="1" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="" id="88ec0b4e802c7f9c" memberName="importBankButton" virtualName=""
              explicitFocusOrder="0" pos="585 63 44 17" bgColOff="ffc8c8c8"
              buttonText="bank" connectedEdges="4" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="" id="bad0b63ba9584a0b" memberName="exportBankButton" virtualName=""
              explicitFocusOrder="0" pos="630 63 44 17" bgColOff="ff7c7c7c"
              buttonText="bank" connectedEdges="4" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
