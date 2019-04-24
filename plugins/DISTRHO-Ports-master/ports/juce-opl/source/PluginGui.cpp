/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.0.2

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "OPLLookAndFeel.h"
#include "ChannelButtonLookAndFeel.h"
//[/Headers]

#include "PluginGui.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
void PluginGui::updateFromParameters()
{
	emulatorSlider->setValue(processor->getEnumParameter("Emulator"), juce::NotificationType::dontSendNotification);

	sineImageButton->setToggleState(false, dontSendNotification);
	halfsineImageButton->setToggleState(false, dontSendNotification);
	abssineImageButton->setToggleState(false, dontSendNotification);
	quartersineImageButton->setToggleState(false, dontSendNotification);
	alternatingsineImageButton->setToggleState(false, dontSendNotification);
	camelsineImageButton->setToggleState(false, dontSendNotification);
	squareImageButton->setToggleState(false, dontSendNotification);
	logsawImageButton->setToggleState(false, dontSendNotification);
	switch(processor->getEnumParameter("Modulator Wave")) {
		case 0: sineImageButton->setToggleState(true, dontSendNotification); break;
		case 1: halfsineImageButton->setToggleState(true, dontSendNotification); break;
		case 2: abssineImageButton->setToggleState(true, dontSendNotification); break;
		case 3: quartersineImageButton->setToggleState(true, dontSendNotification); break;
		case 4: alternatingsineImageButton->setToggleState(true, dontSendNotification); break;
		case 5: camelsineImageButton->setToggleState(true, dontSendNotification); break;
		case 6: squareImageButton->setToggleState(true, dontSendNotification); break;
		case 7: logsawImageButton->setToggleState(true, dontSendNotification); break;
	}

	sineImageButton2->setToggleState(false, dontSendNotification);
	halfsineImageButton2->setToggleState(false, dontSendNotification);
	abssineImageButton2->setToggleState(false, dontSendNotification);
	quartersineImageButton2->setToggleState(false, dontSendNotification);
	alternatingsineImageButton2->setToggleState(false, dontSendNotification);
	camelsineImageButton2->setToggleState(false, dontSendNotification);
	squareImageButton2->setToggleState(false, dontSendNotification);
	logsawImageButton2->setToggleState(false, dontSendNotification);
	switch(processor->getEnumParameter("Carrier Wave")) {
		case 0: sineImageButton2->setToggleState(true, dontSendNotification); break;
		case 1: halfsineImageButton2->setToggleState(true, dontSendNotification); break;
		case 2: abssineImageButton2->setToggleState(true, dontSendNotification); break;
		case 3: quartersineImageButton2->setToggleState(true, dontSendNotification); break;
		case 4: alternatingsineImageButton2->setToggleState(true, dontSendNotification); break;
		case 5: camelsineImageButton2->setToggleState(true, dontSendNotification); break;
		case 6: squareImageButton2->setToggleState(true, dontSendNotification); break;
		case 7: logsawImageButton2->setToggleState(true, dontSendNotification); break;
	}

	fmButton->setToggleState(false, dontSendNotification);
	additiveButton->setToggleState(false, dontSendNotification);
	switch (processor->getEnumParameter("Algorithm")) {
	case 0: fmButton->setToggleState(true, dontSendNotification); break;
	case 1: additiveButton->setToggleState(true, dontSendNotification); break;
	}

	disablePercussionButton->setToggleState(false, dontSendNotification);
	bassDrumButton->setToggleState(false, dontSendNotification);
	snareDrumButton->setToggleState(false, dontSendNotification);
	tomTomButton->setToggleState(false, dontSendNotification);
	cymbalButton->setToggleState(false, dontSendNotification);
	hiHatButton->setToggleState(false, dontSendNotification);
	switch (processor->getEnumParameter("Percussion Mode")) {
	case 0: disablePercussionButton->setToggleState(true, dontSendNotification); break;
	case 1: bassDrumButton->setToggleState(true, dontSendNotification); break;
	case 2: snareDrumButton->setToggleState(true, dontSendNotification); break;
	case 3: tomTomButton->setToggleState(true, dontSendNotification); break;
	case 4: cymbalButton->setToggleState(true, dontSendNotification); break;
	case 5: hiHatButton->setToggleState(true, dontSendNotification); break;
	}

	frequencyComboBox->setSelectedItemIndex (
                processor->getEnumParameter("Modulator Frequency Multiplier"),
                                            sendNotificationAsync);
	frequencyComboBox2->setSelectedItemIndex (
                processor->getEnumParameter("Carrier Frequency Multiplier"),
                                              sendNotificationAsync);

	attenuationSlider->setValue(processor->getEnumParameter("Modulator Attenuation") * -0.75, juce::NotificationType::dontSendNotification);
	attenuationSlider2->setValue(processor->getEnumParameter("Carrier Attenuation") * -0.75, juce::NotificationType::dontSendNotification);

	aSlider->setValue(processor->getIntParameter("Modulator Attack"), juce::NotificationType::dontSendNotification);
	dSlider->setValue(processor->getIntParameter("Modulator Decay"), juce::NotificationType::dontSendNotification);
	sSlider->setValue(processor->getIntParameter("Modulator Sustain Level"), juce::NotificationType::dontSendNotification);
	rSlider->setValue(processor->getIntParameter("Modulator Release"), juce::NotificationType::dontSendNotification);
	aSlider2->setValue(processor->getIntParameter("Carrier Attack"), juce::NotificationType::dontSendNotification);
	dSlider2->setValue(processor->getIntParameter("Carrier Decay"), juce::NotificationType::dontSendNotification);
	sSlider2->setValue(processor->getIntParameter("Carrier Sustain Level"), juce::NotificationType::dontSendNotification);
	rSlider2->setValue(processor->getIntParameter("Carrier Release"), juce::NotificationType::dontSendNotification);

/// Jeff-Russ replaced the second arg of "true" with "sendNotificationAsync":

	keyscaleAttenuationComboBox->setSelectedItemIndex (
                        processor->getEnumParameter("Modulator Keyscale Level"),
                                                       sendNotificationAsync);
	keyscaleAttenuationComboBox2->setSelectedItemIndex (
                        processor->getEnumParameter("Carrier Keyscale Level"),
                                                        sendNotificationAsync);


	tremoloButton->setToggleState(processor->getBoolParameter("Modulator Tremolo"), dontSendNotification);
	vibratoButton->setToggleState(processor->getBoolParameter("Modulator Vibrato"), dontSendNotification);
	sustainButton->setToggleState(processor->getBoolParameter("Modulator Sustain"), dontSendNotification);
	keyscaleEnvButton->setToggleState(processor->getBoolParameter("Modulator Keyscale Rate"), dontSendNotification);

	tremoloButton2->setToggleState(processor->getBoolParameter("Carrier Tremolo"), dontSendNotification);
	vibratoButton2->setToggleState(processor->getBoolParameter("Carrier Vibrato"), dontSendNotification);
	sustainButton2->setToggleState(processor->getBoolParameter("Carrier Sustain"), dontSendNotification);
	keyscaleEnvButton2->setToggleState(processor->getBoolParameter("Carrier Keyscale Rate"), dontSendNotification);

	vibratoSlider->setValue(processor->getEnumParameter("Vibrato Depth") * 7.0 + 7.0, juce::NotificationType::dontSendNotification);
	tremoloSlider->setValue(processor->getEnumParameter("Tremolo Depth") * 3.8 + 1.0, juce::NotificationType::dontSendNotification);
	feedbackSlider->setValue(processor->getIntParameter("Modulator Feedback"), juce::NotificationType::dontSendNotification);

/// Jeff-Russ replaced the second arg of "true" with "sendNotificationAsync":

	velocityComboBox->setSelectedItemIndex (
                    processor->getEnumParameter("Modulator Velocity Sensitivity"),
                                            sendNotificationAsync);
	velocityComboBox2->setSelectedItemIndex (
                    processor->getEnumParameter("Carrier Velocity Sensitivity"),
                                             sendNotificationAsync);

	tooltipWindow.setColour(tooltipWindow.backgroundColourId, Colour(0x0));
	tooltipWindow.setColour(tooltipWindow.textColourId, Colour(COLOUR_MID));
}

void PluginGui::setRecordButtonState(bool recording) {
	if (recording) {
		recordButton->setColour(TextButton::buttonColourId, Colour(COLOUR_RECORDING));
		recordButton->setButtonText("Recording..");
		recordButton->setColour(ToggleButton::textColourId, Colour(COLOUR_RECORDING));
	} else {
		recordButton->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
		recordButton->setButtonText("Record to DRO");
		recordButton->setColour(ToggleButton::textColourId, Colour(COLOUR_MID));
	}
}

//[/MiscUserDefs]

//==============================================================================
PluginGui::PluginGui (AdlibBlasterAudioProcessor* ownerFilter)
    : AudioProcessorEditor (ownerFilter)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (groupComponent2 = new GroupComponent ("new group",
                                                             TRANS("Carrier")));
    groupComponent2->setTextLabelPosition (Justification::centredLeft);
    groupComponent2->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent2->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (groupComponent4 = new GroupComponent ("new group",
                                                             TRANS("Channels")));
    groupComponent4->setTextLabelPosition (Justification::centredLeft);
    groupComponent4->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent4->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (groupComponent11 = new GroupComponent ("new group",
                                                              TRANS("Percussion")));
    groupComponent11->setTextLabelPosition (Justification::centredLeft);
    groupComponent11->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent11->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (groupComponent10 = new GroupComponent ("new group",
                                                              TRANS("Algorithm")));
    groupComponent10->setTextLabelPosition (Justification::centredLeft);
    groupComponent10->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent10->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (groupComponent9 = new GroupComponent ("new group",
                                                             TRANS("File")));
    groupComponent9->setTextLabelPosition (Justification::centredLeft);
    groupComponent9->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent9->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (groupComponent = new GroupComponent ("new group",
                                                            TRANS("Modulator")));
    groupComponent->setTextLabelPosition (Justification::centredLeft);
    groupComponent->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (frequencyComboBox = new ComboBox ("frequency combo box"));
    frequencyComboBox->setEditableText (false);
    frequencyComboBox->setJustificationType (Justification::centredLeft);
    frequencyComboBox->setTextWhenNothingSelected (String());
    frequencyComboBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    frequencyComboBox->addListener (this);

    addAndMakeVisible (frequencyLabel = new Label ("frequency label",
                                                   TRANS("Frequency Multiplier")));
    frequencyLabel->setTooltip (TRANS("Multiplier applied to base note frequency"));
    frequencyLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel->setJustificationType (Justification::centredLeft);
    frequencyLabel->setEditable (false, false, false);
    frequencyLabel->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (aSlider = new Slider ("a slider"));
    aSlider->setTooltip (TRANS("Envelope attack rate"));
    aSlider->setRange (0, 15, 1);
    aSlider->setSliderStyle (Slider::LinearVertical);
    aSlider->setTextBoxStyle (Slider::TextBoxBelow, true, 30, 20);
    aSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    aSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    aSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    aSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    aSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    aSlider->addListener (this);

    addAndMakeVisible (aLabel = new Label ("a label",
                                           TRANS("A")));
    aLabel->setTooltip (TRANS("Attack rate"));
    aLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    aLabel->setJustificationType (Justification::centred);
    aLabel->setEditable (false, false, false);
    aLabel->setColour (Label::textColourId, Colour (0xff007f00));
    aLabel->setColour (TextEditor::textColourId, Colours::black);
    aLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dSlider = new Slider ("d slider"));
    dSlider->setTooltip (TRANS("Envelope decay rate"));
    dSlider->setRange (0, 15, 1);
    dSlider->setSliderStyle (Slider::LinearVertical);
    dSlider->setTextBoxStyle (Slider::TextBoxBelow, true, 30, 20);
    dSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    dSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    dSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    dSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    dSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    dSlider->addListener (this);

    addAndMakeVisible (dLabel = new Label ("d label",
                                           TRANS("D")));
    dLabel->setTooltip (TRANS("Decay rate"));
    dLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dLabel->setJustificationType (Justification::centred);
    dLabel->setEditable (false, false, false);
    dLabel->setColour (Label::textColourId, Colour (0xff007f00));
    dLabel->setColour (TextEditor::textColourId, Colours::black);
    dLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (sSlider = new Slider ("s slider"));
    sSlider->setTooltip (TRANS("Envelope sustain level"));
    sSlider->setRange (0, 15, 1);
    sSlider->setSliderStyle (Slider::LinearVertical);
    sSlider->setTextBoxStyle (Slider::TextBoxBelow, true, 30, 20);
    sSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    sSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    sSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    sSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    sSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    sSlider->addListener (this);

    addAndMakeVisible (dLabel2 = new Label ("d label",
                                            TRANS("S")));
    dLabel2->setTooltip (TRANS("Sustain level"));
    dLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dLabel2->setJustificationType (Justification::centred);
    dLabel2->setEditable (false, false, false);
    dLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    dLabel2->setColour (TextEditor::textColourId, Colours::black);
    dLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (rSlider = new Slider ("r slider"));
    rSlider->setTooltip (TRANS("Envelope release rate"));
    rSlider->setRange (0, 15, 1);
    rSlider->setSliderStyle (Slider::LinearVertical);
    rSlider->setTextBoxStyle (Slider::TextBoxBelow, true, 30, 20);
    rSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    rSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    rSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    rSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    rSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    rSlider->addListener (this);

    addAndMakeVisible (rLabel = new Label ("r label",
                                           TRANS("R")));
    rLabel->setTooltip (TRANS("Release rate"));
    rLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    rLabel->setJustificationType (Justification::centred);
    rLabel->setEditable (false, false, false);
    rLabel->setColour (Label::textColourId, Colour (0xff007f00));
    rLabel->setColour (TextEditor::textColourId, Colours::black);
    rLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (attenuationSlider = new Slider ("attenuation slider"));
    attenuationSlider->setRange (-47.25, 0, 0.75);
    attenuationSlider->setSliderStyle (Slider::LinearVertical);
    attenuationSlider->setTextBoxStyle (Slider::TextBoxBelow, true, 64, 20);
    attenuationSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    attenuationSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    attenuationSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    attenuationSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    attenuationSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    attenuationSlider->addListener (this);

    addAndMakeVisible (attenuationLabel = new Label ("attenuation label",
                                                     TRANS("Attenuation")));
    attenuationLabel->setTooltip (TRANS("Final output level adjustment"));
    attenuationLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    attenuationLabel->setJustificationType (Justification::centred);
    attenuationLabel->setEditable (false, false, false);
    attenuationLabel->setColour (Label::textColourId, Colour (0xff007f00));
    attenuationLabel->setColour (TextEditor::textColourId, Colours::black);
    attenuationLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dbLabel = new Label ("db label",
                                            TRANS("dB")));
    dbLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel->setJustificationType (Justification::centred);
    dbLabel->setEditable (false, false, false);
    dbLabel->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel->setColour (TextEditor::textColourId, Colours::black);
    dbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (sineImageButton = new ImageButton ("sine image button"));
    sineImageButton->setButtonText (TRANS("Sine"));
    sineImageButton->setRadioGroupId (1);
    sineImageButton->addListener (this);

    sineImageButton->setImages (false, true, true,
                                ImageCache::getFromMemory (full_sine_png, full_sine_pngSize), 0.500f, Colour (0x00000000),
                                Image(), 0.500f, Colour (0x00000000),
                                Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (halfsineImageButton = new ImageButton ("half sine image button"));
    halfsineImageButton->setButtonText (TRANS("Half Sine"));
    halfsineImageButton->setRadioGroupId (1);
    halfsineImageButton->addListener (this);

    halfsineImageButton->setImages (false, true, true,
                                    ImageCache::getFromMemory (half_sine_png, half_sine_pngSize), 0.500f, Colour (0x00000000),
                                    Image(), 0.500f, Colour (0x00000000),
                                    Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (abssineImageButton = new ImageButton ("abs sine image button"));
    abssineImageButton->setButtonText (TRANS("Abs Sine"));
    abssineImageButton->setRadioGroupId (1);
    abssineImageButton->addListener (this);

    abssineImageButton->setImages (false, true, true,
                                   ImageCache::getFromMemory (abs_sine_png, abs_sine_pngSize), 0.500f, Colour (0x00000000),
                                   Image(), 0.500f, Colour (0x00000000),
                                   Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (quartersineImageButton = new ImageButton ("quarter sine image button"));
    quartersineImageButton->setButtonText (TRANS("Quarter Sine"));
    quartersineImageButton->setRadioGroupId (1);
    quartersineImageButton->addListener (this);

    quartersineImageButton->setImages (false, true, true,
                                       ImageCache::getFromMemory (quarter_sine_png, quarter_sine_pngSize), 0.500f, Colour (0x00000000),
                                       Image(), 0.500f, Colour (0x00000000),
                                       Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (waveLabel = new Label ("wave label",
                                              TRANS("Wave")));
    waveLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    waveLabel->setJustificationType (Justification::centredLeft);
    waveLabel->setEditable (false, false, false);
    waveLabel->setColour (Label::textColourId, Colour (0xff007f00));
    waveLabel->setColour (TextEditor::textColourId, Colours::black);
    waveLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (tremoloButton = new ToggleButton ("tremolo button"));
    tremoloButton->setTooltip (TRANS("Modulate amplitude at 3.7 Hz"));
    tremoloButton->setButtonText (TRANS("Tremolo"));
    tremoloButton->addListener (this);
    tremoloButton->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (vibratoButton = new ToggleButton ("vibrato button"));
    vibratoButton->setTooltip (TRANS("Modulate frequency at 6.1 Hz"));
    vibratoButton->setButtonText (TRANS("Vibrato"));
    vibratoButton->addListener (this);
    vibratoButton->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (sustainButton = new ToggleButton ("sustain button"));
    sustainButton->setTooltip (TRANS("Enable or disable sustain when note is held"));
    sustainButton->setButtonText (TRANS("Sustain"));
    sustainButton->addListener (this);
    sustainButton->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (keyscaleEnvButton = new ToggleButton ("keyscale env button"));
    keyscaleEnvButton->setTooltip (TRANS("Speed up envelope rate with note frequency"));
    keyscaleEnvButton->setButtonText (TRANS("Keyscale Env. Rate"));
    keyscaleEnvButton->addListener (this);
    keyscaleEnvButton->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (dbLabel2 = new Label ("db label",
                                             TRANS("dB/8ve\n")));
    dbLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel2->setJustificationType (Justification::centred);
    dbLabel2->setEditable (false, false, false);
    dbLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel2->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel2->setColour (TextEditor::textColourId, Colours::black);
    dbLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (frequencyComboBox2 = new ComboBox ("frequency combo box"));
    frequencyComboBox2->setEditableText (false);
    frequencyComboBox2->setJustificationType (Justification::centredLeft);
    frequencyComboBox2->setTextWhenNothingSelected (String());
    frequencyComboBox2->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    frequencyComboBox2->addListener (this);

    addAndMakeVisible (frequencyLabel3 = new Label ("frequency label",
                                                    TRANS("Frequency Multiplier")));
    frequencyLabel3->setTooltip (TRANS("Multiplier applied to base note frequency"));
    frequencyLabel3->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel3->setJustificationType (Justification::centredLeft);
    frequencyLabel3->setEditable (false, false, false);
    frequencyLabel3->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel3->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (aSlider2 = new Slider ("a slider"));
    aSlider2->setRange (0, 15, 1);
    aSlider2->setSliderStyle (Slider::LinearVertical);
    aSlider2->setTextBoxStyle (Slider::TextBoxBelow, true, 40, 20);
    aSlider2->setColour (Slider::thumbColourId, Colour (0xff007f00));
    aSlider2->setColour (Slider::trackColourId, Colour (0x7f007f00));
    aSlider2->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    aSlider2->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    aSlider2->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    aSlider2->addListener (this);

    addAndMakeVisible (aLabel2 = new Label ("a label",
                                            TRANS("A")));
    aLabel2->setTooltip (TRANS("Attack rate"));
    aLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    aLabel2->setJustificationType (Justification::centred);
    aLabel2->setEditable (false, false, false);
    aLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    aLabel2->setColour (TextEditor::textColourId, Colours::black);
    aLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dSlider2 = new Slider ("d slider"));
    dSlider2->setRange (0, 15, 1);
    dSlider2->setSliderStyle (Slider::LinearVertical);
    dSlider2->setTextBoxStyle (Slider::TextBoxBelow, true, 40, 20);
    dSlider2->setColour (Slider::thumbColourId, Colour (0xff007f00));
    dSlider2->setColour (Slider::trackColourId, Colour (0x7f007f00));
    dSlider2->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    dSlider2->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    dSlider2->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    dSlider2->addListener (this);

    addAndMakeVisible (dLabel3 = new Label ("d label",
                                            TRANS("D")));
    dLabel3->setTooltip (TRANS("Decay rate"));
    dLabel3->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dLabel3->setJustificationType (Justification::centred);
    dLabel3->setEditable (false, false, false);
    dLabel3->setColour (Label::textColourId, Colour (0xff007f00));
    dLabel3->setColour (TextEditor::textColourId, Colours::black);
    dLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (sSlider2 = new Slider ("s slider"));
    sSlider2->setRange (0, 15, 1);
    sSlider2->setSliderStyle (Slider::LinearVertical);
    sSlider2->setTextBoxStyle (Slider::TextBoxBelow, true, 40, 20);
    sSlider2->setColour (Slider::thumbColourId, Colour (0xff007f00));
    sSlider2->setColour (Slider::trackColourId, Colour (0x7f007f00));
    sSlider2->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    sSlider2->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    sSlider2->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    sSlider2->addListener (this);

    addAndMakeVisible (dLabel4 = new Label ("d label",
                                            TRANS("S")));
    dLabel4->setTooltip (TRANS("Sustain level"));
    dLabel4->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dLabel4->setJustificationType (Justification::centred);
    dLabel4->setEditable (false, false, false);
    dLabel4->setColour (Label::textColourId, Colour (0xff007f00));
    dLabel4->setColour (TextEditor::textColourId, Colours::black);
    dLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (rSlider2 = new Slider ("r slider"));
    rSlider2->setRange (0, 15, 1);
    rSlider2->setSliderStyle (Slider::LinearVertical);
    rSlider2->setTextBoxStyle (Slider::TextBoxBelow, true, 40, 20);
    rSlider2->setColour (Slider::thumbColourId, Colour (0xff007f00));
    rSlider2->setColour (Slider::trackColourId, Colour (0x7f007f00));
    rSlider2->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    rSlider2->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    rSlider2->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    rSlider2->addListener (this);

    addAndMakeVisible (rLabel2 = new Label ("r label",
                                            TRANS("R")));
    rLabel2->setTooltip (TRANS("Release rate"));
    rLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    rLabel2->setJustificationType (Justification::centred);
    rLabel2->setEditable (false, false, false);
    rLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    rLabel2->setColour (TextEditor::textColourId, Colours::black);
    rLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (attenuationSlider2 = new Slider ("attenuation slider"));
    attenuationSlider2->setRange (-47.25, 0, 0.75);
    attenuationSlider2->setSliderStyle (Slider::LinearVertical);
    attenuationSlider2->setTextBoxStyle (Slider::TextBoxBelow, true, 64, 20);
    attenuationSlider2->setColour (Slider::thumbColourId, Colour (0xff007f00));
    attenuationSlider2->setColour (Slider::trackColourId, Colour (0x7f007f00));
    attenuationSlider2->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    attenuationSlider2->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    attenuationSlider2->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    attenuationSlider2->addListener (this);

    addAndMakeVisible (attenuationLabel2 = new Label ("attenuation label",
                                                      TRANS("Attenuation")));
    attenuationLabel2->setTooltip (TRANS("Final output level adjustment"));
    attenuationLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    attenuationLabel2->setJustificationType (Justification::centred);
    attenuationLabel2->setEditable (false, false, false);
    attenuationLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    attenuationLabel2->setColour (TextEditor::textColourId, Colours::black);
    attenuationLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dbLabel3 = new Label ("db label",
                                             TRANS("dB")));
    dbLabel3->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel3->setJustificationType (Justification::centred);
    dbLabel3->setEditable (false, false, false);
    dbLabel3->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel3->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel3->setColour (TextEditor::textColourId, Colours::black);
    dbLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (sineImageButton2 = new ImageButton ("sine image button"));
    sineImageButton2->setButtonText (TRANS("Sine"));
    sineImageButton2->setRadioGroupId (2);
    sineImageButton2->addListener (this);

    sineImageButton2->setImages (false, true, true,
                                 ImageCache::getFromMemory (full_sine_png, full_sine_pngSize), 0.500f, Colour (0x00000000),
                                 Image(), 0.500f, Colour (0x00000000),
                                 Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (halfsineImageButton2 = new ImageButton ("half sine image button"));
    halfsineImageButton2->setButtonText (TRANS("Half Sine"));
    halfsineImageButton2->setRadioGroupId (2);
    halfsineImageButton2->addListener (this);

    halfsineImageButton2->setImages (false, true, true,
                                     ImageCache::getFromMemory (half_sine_png, half_sine_pngSize), 0.500f, Colour (0x00000000),
                                     Image(), 0.500f, Colour (0x00000000),
                                     Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (abssineImageButton2 = new ImageButton ("abs sine image button"));
    abssineImageButton2->setButtonText (TRANS("Abs Sine"));
    abssineImageButton2->setRadioGroupId (2);
    abssineImageButton2->addListener (this);

    abssineImageButton2->setImages (false, true, true,
                                    ImageCache::getFromMemory (abs_sine_png, abs_sine_pngSize), 0.500f, Colour (0x00000000),
                                    Image(), 0.500f, Colour (0x00000000),
                                    Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (quartersineImageButton2 = new ImageButton ("quarter sine image button"));
    quartersineImageButton2->setButtonText (TRANS("Quarter Sine"));
    quartersineImageButton2->setRadioGroupId (2);
    quartersineImageButton2->addListener (this);

    quartersineImageButton2->setImages (false, true, true,
                                        ImageCache::getFromMemory (quarter_sine_png, quarter_sine_pngSize), 0.500f, Colour (0x00000000),
                                        Image(), 0.500f, Colour (0x00000000),
                                        Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (waveLabel2 = new Label ("wave label",
                                               TRANS("Wave")));
    waveLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    waveLabel2->setJustificationType (Justification::centredLeft);
    waveLabel2->setEditable (false, false, false);
    waveLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    waveLabel2->setColour (TextEditor::textColourId, Colours::black);
    waveLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (tremoloButton2 = new ToggleButton ("tremolo button"));
    tremoloButton2->setTooltip (TRANS("Modulate amplitude at 3.7 Hz"));
    tremoloButton2->setButtonText (TRANS("Tremolo"));
    tremoloButton2->addListener (this);
    tremoloButton2->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (vibratoButton2 = new ToggleButton ("vibrato button"));
    vibratoButton2->setTooltip (TRANS("Modulate frequency at 6.1 Hz"));
    vibratoButton2->setButtonText (TRANS("Vibrato"));
    vibratoButton2->addListener (this);
    vibratoButton2->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (sustainButton2 = new ToggleButton ("sustain button"));
    sustainButton2->setTooltip (TRANS("Enable or disable sustain when note is held"));
    sustainButton2->setButtonText (TRANS("Sustain"));
    sustainButton2->addListener (this);
    sustainButton2->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (keyscaleEnvButton2 = new ToggleButton ("keyscale env button"));
    keyscaleEnvButton2->setTooltip (TRANS("Speed up envelope rate with note frequency"));
    keyscaleEnvButton2->setButtonText (TRANS("Keyscale Env. Rate"));
    keyscaleEnvButton2->addListener (this);
    keyscaleEnvButton2->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (frequencyLabel4 = new Label ("frequency label",
                                                    TRANS("Keyscale Attenuation")));
    frequencyLabel4->setTooltip (TRANS("Attenuate amplitude with note frequency in dB per octave"));
    frequencyLabel4->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel4->setJustificationType (Justification::centred);
    frequencyLabel4->setEditable (false, false, false);
    frequencyLabel4->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel4->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (groupComponent3 = new GroupComponent ("new group",
                                                             TRANS("Effect depth")));
    groupComponent3->setTextLabelPosition (Justification::centredLeft);
    groupComponent3->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent3->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (tremoloSlider = new Slider ("tremolo slider"));
    tremoloSlider->setRange (1, 4.8, 3.8);
    tremoloSlider->setSliderStyle (Slider::LinearHorizontal);
    tremoloSlider->setTextBoxStyle (Slider::TextBoxRight, true, 32, 20);
    tremoloSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    tremoloSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    tremoloSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    tremoloSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    tremoloSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    tremoloSlider->addListener (this);

    addAndMakeVisible (frequencyLabel5 = new Label ("frequency label",
                                                    TRANS("Tremolo\n")));
    frequencyLabel5->setTooltip (TRANS("OPL global tremolo depth"));
    frequencyLabel5->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel5->setJustificationType (Justification::centredLeft);
    frequencyLabel5->setEditable (false, false, false);
    frequencyLabel5->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel5->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel5->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dbLabel5 = new Label ("db label",
                                             TRANS("dB")));
    dbLabel5->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel5->setJustificationType (Justification::centredLeft);
    dbLabel5->setEditable (false, false, false);
    dbLabel5->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel5->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel5->setColour (TextEditor::textColourId, Colours::black);
    dbLabel5->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (vibratoSlider = new Slider ("vibrato slider"));
    vibratoSlider->setRange (7, 14, 7);
    vibratoSlider->setSliderStyle (Slider::LinearHorizontal);
    vibratoSlider->setTextBoxStyle (Slider::TextBoxRight, true, 32, 20);
    vibratoSlider->setColour (Slider::thumbColourId, Colour (0xff007f00));
    vibratoSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    vibratoSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    vibratoSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    vibratoSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    vibratoSlider->addListener (this);

    addAndMakeVisible (frequencyLabel6 = new Label ("frequency label",
                                                    TRANS("Vibrato")));
    frequencyLabel6->setTooltip (TRANS("OPL global vibrato depth"));
    frequencyLabel6->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel6->setJustificationType (Justification::centredLeft);
    frequencyLabel6->setEditable (false, false, false);
    frequencyLabel6->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel6->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel6->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dbLabel6 = new Label ("db label",
                                             TRANS("cents\n")));
    dbLabel6->setTooltip (TRANS("A unit of pitch; 100 cents per semitone"));
    dbLabel6->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel6->setJustificationType (Justification::centredLeft);
    dbLabel6->setEditable (false, false, false);
    dbLabel6->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel6->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel6->setColour (TextEditor::textColourId, Colours::black);
    dbLabel6->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (feedbackSlider = new Slider ("feedback slider"));
    feedbackSlider->setRange (0, 7, 1);
    feedbackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider->setTextBoxStyle (Slider::TextBoxBelow, true, 30, 20);
    feedbackSlider->setColour (Slider::thumbColourId, Colour (0xff00af00));
    feedbackSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    feedbackSlider->setColour (Slider::rotarySliderFillColourId, Colour (0xff00af00));
    feedbackSlider->setColour (Slider::rotarySliderOutlineColourId, Colour (0xff007f00));
    feedbackSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    feedbackSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    feedbackSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    feedbackSlider->addListener (this);

    addAndMakeVisible (frequencyLabel7 = new Label ("frequency label",
                                                    TRANS("Feedback")));
    frequencyLabel7->setTooltip (TRANS("Extent to which modulator output is fed back into itself"));
    frequencyLabel7->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel7->setJustificationType (Justification::centred);
    frequencyLabel7->setEditable (false, false, false);
    frequencyLabel7->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel7->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel7->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (velocityComboBox = new ComboBox ("velocity combo box"));
    velocityComboBox->setEditableText (false);
    velocityComboBox->setJustificationType (Justification::centredLeft);
    velocityComboBox->setTextWhenNothingSelected (String());
    velocityComboBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    velocityComboBox->addItem (TRANS("Off"), 1);
    velocityComboBox->addItem (TRANS("Light"), 2);
    velocityComboBox->addItem (TRANS("Heavy"), 3);
    velocityComboBox->addListener (this);

    addAndMakeVisible (velocityComboBox2 = new ComboBox ("velocity combo box"));
    velocityComboBox2->setEditableText (false);
    velocityComboBox2->setJustificationType (Justification::centredLeft);
    velocityComboBox2->setTextWhenNothingSelected (String());
    velocityComboBox2->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    velocityComboBox2->addItem (TRANS("Off"), 1);
    velocityComboBox2->addItem (TRANS("Light"), 2);
    velocityComboBox2->addItem (TRANS("Heavy"), 3);
    velocityComboBox2->addListener (this);

    addAndMakeVisible (attenuationLabel4 = new Label ("attenuation label",
                                                      TRANS("Velocity Sensitivity")));
    attenuationLabel4->setTooltip (TRANS("Set or disable velocity senstivity"));
    attenuationLabel4->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    attenuationLabel4->setJustificationType (Justification::centred);
    attenuationLabel4->setEditable (false, false, false);
    attenuationLabel4->setColour (Label::textColourId, Colour (0xff007f00));
    attenuationLabel4->setColour (TextEditor::textColourId, Colours::black);
    attenuationLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (alternatingsineImageButton = new ImageButton ("alternating sine image button"));
    alternatingsineImageButton->setButtonText (TRANS("Alternating Sine"));
    alternatingsineImageButton->setRadioGroupId (1);
    alternatingsineImageButton->addListener (this);

    alternatingsineImageButton->setImages (false, true, true,
                                           ImageCache::getFromMemory (alternating_sine_png, alternating_sine_pngSize), 0.500f, Colour (0x00000000),
                                           Image(), 0.500f, Colour (0x00000000),
                                           Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (camelsineImageButton = new ImageButton ("camel sine image button"));
    camelsineImageButton->setButtonText (TRANS("Camel Sine"));
    camelsineImageButton->setRadioGroupId (1);
    camelsineImageButton->addListener (this);

    camelsineImageButton->setImages (false, true, true,
                                     ImageCache::getFromMemory (camel_sine_png, camel_sine_pngSize), 0.500f, Colour (0x00000000),
                                     Image(), 0.500f, Colour (0x00000000),
                                     Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (squareImageButton = new ImageButton ("square image button"));
    squareImageButton->setButtonText (TRANS("Square"));
    squareImageButton->setRadioGroupId (1);
    squareImageButton->addListener (this);

    squareImageButton->setImages (false, true, true,
                                  ImageCache::getFromMemory (square_png, square_pngSize), 0.500f, Colour (0x00000000),
                                  Image(), 0.500f, Colour (0x00000000),
                                  Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (logsawImageButton = new ImageButton ("logsaw image button"));
    logsawImageButton->setButtonText (TRANS("Logarithmic Sawtooth"));
    logsawImageButton->setRadioGroupId (1);
    logsawImageButton->addListener (this);

    logsawImageButton->setImages (false, true, true,
                                  ImageCache::getFromMemory (logarithmic_saw_png, logarithmic_saw_pngSize), 0.500f, Colour (0x00000000),
                                  Image(), 0.500f, Colour (0x00000000),
                                  Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (alternatingsineImageButton2 = new ImageButton ("alternating sine image button"));
    alternatingsineImageButton2->setButtonText (TRANS("Alternating Sine"));
    alternatingsineImageButton2->setRadioGroupId (2);
    alternatingsineImageButton2->addListener (this);

    alternatingsineImageButton2->setImages (false, true, true,
                                            ImageCache::getFromMemory (alternating_sine_png, alternating_sine_pngSize), 0.500f, Colour (0x00000000),
                                            Image(), 0.500f, Colour (0x00000000),
                                            Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (camelsineImageButton2 = new ImageButton ("camel sine image button"));
    camelsineImageButton2->setButtonText (TRANS("Camel Sine"));
    camelsineImageButton2->setRadioGroupId (2);
    camelsineImageButton2->addListener (this);

    camelsineImageButton2->setImages (false, true, true,
                                      ImageCache::getFromMemory (camel_sine_png, camel_sine_pngSize), 0.500f, Colour (0x00000000),
                                      Image(), 0.500f, Colour (0x00000000),
                                      Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (squareImageButton2 = new ImageButton ("square image button"));
    squareImageButton2->setButtonText (TRANS("Square"));
    squareImageButton2->setRadioGroupId (2);
    squareImageButton2->addListener (this);

    squareImageButton2->setImages (false, true, true,
                                   ImageCache::getFromMemory (square_png, square_pngSize), 0.500f, Colour (0x00000000),
                                   Image(), 0.500f, Colour (0x00000000),
                                   Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (logsawImageButton2 = new ImageButton ("logsaw image button"));
    logsawImageButton2->setButtonText (TRANS("Logarithmic Sawtooth"));
    logsawImageButton2->setRadioGroupId (2);
    logsawImageButton2->addListener (this);

    logsawImageButton2->setImages (false, true, true,
                                   ImageCache::getFromMemory (logarithmic_saw_png, logarithmic_saw_pngSize), 0.500f, Colour (0x00000000),
                                   Image(), 0.500f, Colour (0x00000000),
                                   Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (dbLabel4 = new Label ("db label",
                                             TRANS("dB/8ve\n")));
    dbLabel4->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel4->setJustificationType (Justification::centred);
    dbLabel4->setEditable (false, false, false);
    dbLabel4->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel4->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel4->setColour (TextEditor::textColourId, Colours::black);
    dbLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (keyscaleAttenuationComboBox2 = new ComboBox ("keyscale combo box"));
    keyscaleAttenuationComboBox2->setEditableText (false);
    keyscaleAttenuationComboBox2->setJustificationType (Justification::centredLeft);
    keyscaleAttenuationComboBox2->setTextWhenNothingSelected (String());
    keyscaleAttenuationComboBox2->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    keyscaleAttenuationComboBox2->addItem (TRANS("-0.0"), 1);
    keyscaleAttenuationComboBox2->addItem (TRANS("-3.0"), 2);
    keyscaleAttenuationComboBox2->addItem (TRANS("-1.5"), 3);
    keyscaleAttenuationComboBox2->addItem (TRANS("-6.0"), 4);
    keyscaleAttenuationComboBox2->addListener (this);

    addAndMakeVisible (keyscaleAttenuationComboBox = new ComboBox ("keyscale combo box"));
    keyscaleAttenuationComboBox->setEditableText (false);
    keyscaleAttenuationComboBox->setJustificationType (Justification::centredLeft);
    keyscaleAttenuationComboBox->setTextWhenNothingSelected (String());
    keyscaleAttenuationComboBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    keyscaleAttenuationComboBox->addItem (TRANS("-0.0"), 1);
    keyscaleAttenuationComboBox->addItem (TRANS("-3.0"), 2);
    keyscaleAttenuationComboBox->addItem (TRANS("-1.5"), 3);
    keyscaleAttenuationComboBox->addItem (TRANS("-6.0"), 4);
    keyscaleAttenuationComboBox->addListener (this);

    addAndMakeVisible (groupComponent5 = new GroupComponent ("new group",
                                                             TRANS("Emulator (currently locked)")));
    groupComponent5->setTextLabelPosition (Justification::centredLeft);
    groupComponent5->setColour (GroupComponent::outlineColourId, Colour (0xff007f00));
    groupComponent5->setColour (GroupComponent::textColourId, Colour (0xff007f00));

    addAndMakeVisible (emulatorSlider = new Slider ("emulator slider"));
    emulatorSlider->setRange (0, 1, 1);
    emulatorSlider->setSliderStyle (Slider::LinearHorizontal);
    emulatorSlider->setTextBoxStyle (Slider::NoTextBox, true, 44, 20);
    emulatorSlider->setColour (Slider::thumbColourId, Colour (0xff00af00));
    emulatorSlider->setColour (Slider::trackColourId, Colour (0x7f007f00));
    emulatorSlider->setColour (Slider::textBoxTextColourId, Colour (0xff007f00));
    emulatorSlider->setColour (Slider::textBoxBackgroundColourId, Colours::black);
    emulatorSlider->setColour (Slider::textBoxHighlightColourId, Colour (0xff00af00));
    emulatorSlider->addListener (this);

    addAndMakeVisible (emulatorLabel = new Label ("emulator label",
                                                  TRANS("DOSBox")));
    emulatorLabel->setTooltip (TRANS("Use the OPL emulator from the DOSBox project"));
    emulatorLabel->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    emulatorLabel->setJustificationType (Justification::centredRight);
    emulatorLabel->setEditable (false, false, false);
    emulatorLabel->setColour (Label::textColourId, Colour (0xff007f00));
    emulatorLabel->setColour (TextEditor::textColourId, Colours::black);
    emulatorLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (emulatorLabel2 = new Label ("emulator label",
                                                   TRANS("ZDoom")));
    emulatorLabel2->setTooltip (TRANS("Use the OPL emulator from the ZDoom project"));
    emulatorLabel2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    emulatorLabel2->setJustificationType (Justification::centredLeft);
    emulatorLabel2->setEditable (false, false, false);
    emulatorLabel2->setColour (Label::textColourId, Colour (0xff007f00));
    emulatorLabel2->setColour (TextEditor::textColourId, Colours::black);
    emulatorLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (recordButton = new ToggleButton ("record button"));
    recordButton->setTooltip (TRANS("Start recording all register writes to a DRO file - an OPL recording file format defined by DOSBox"));
    recordButton->setButtonText (TRANS("Record to DRO (not working yet)"));
    recordButton->addListener (this);
    recordButton->setColour (ToggleButton::textColourId, Colour (0xff007f00));

    addAndMakeVisible (exportButton = new TextButton ("export button"));
    exportButton->setButtonText (TRANS("Export .SBI"));
    exportButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    exportButton->addListener (this);
    exportButton->setColour (TextButton::buttonColourId, Colour (0xff007f00));
    exportButton->setColour (TextButton::buttonOnColourId, Colours::lime);

    addAndMakeVisible (loadButton = new TextButton ("load button"));
    loadButton->setButtonText (TRANS("Load .SBI"));
    loadButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    loadButton->addListener (this);
    loadButton->setColour (TextButton::buttonColourId, Colour (0xff007f00));
    loadButton->setColour (TextButton::buttonOnColourId, Colours::lime);

    addAndMakeVisible (versionLabel = new Label ("version label",
                                                 String()));
    versionLabel->setFont (Font (12.00f, Font::plain).withTypefaceStyle ("Regular"));
    versionLabel->setJustificationType (Justification::centredRight);
    versionLabel->setEditable (false, false, false);
    versionLabel->setColour (Label::textColourId, Colour (0xff007f00));
    versionLabel->setColour (TextEditor::textColourId, Colours::black);
    versionLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (ToggleButtonOffExample = new ImageButton ("Toggle Button Off Example"));
    ToggleButtonOffExample->setButtonText (TRANS("new button"));
    ToggleButtonOffExample->addListener (this);

    ToggleButtonOffExample->setImages (false, true, true,
                                       ImageCache::getFromMemory (toggle_off_sq_png, toggle_off_sq_pngSize), 1.000f, Colour (0x00000000),
                                       Image(), 1.000f, Colour (0x00000000),
                                       Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (ToggleButtonOnExample = new ImageButton ("Toggle Button On Example"));
    ToggleButtonOnExample->setButtonText (TRANS("new button"));
    ToggleButtonOnExample->addListener (this);

    ToggleButtonOnExample->setImages (false, true, true,
                                      ImageCache::getFromMemory (toggle_on_sq_png, toggle_on_sq_pngSize), 1.000f, Colour (0x00000000),
                                      Image(), 1.000f, Colour (0x00000000),
                                      Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("Toggle buttons")));
    label->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label->setJustificationType (Justification::centred);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colours::green);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label2 = new Label ("new label",
                                           TRANS("Line borders")));
    label2->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label2->setJustificationType (Justification::centred);
    label2->setEditable (false, false, false);
    label2->setColour (Label::textColourId, Colours::green);
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (LineBorderButton1C = new ImageButton ("Line Border 1C"));
    LineBorderButton1C->setButtonText (TRANS("new button"));

    LineBorderButton1C->setImages (false, true, false,
                                   ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                   ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                   ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (LineBorderButton1A = new ImageButton ("Line Border 1A"));
    LineBorderButton1A->setButtonText (TRANS("new button"));

    LineBorderButton1A->setImages (false, true, false,
                                   ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                   ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                   ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (LineBorderButton1B = new ImageButton ("Line Border 1B"));
    LineBorderButton1B->setButtonText (TRANS("new button"));

    LineBorderButton1B->setImages (false, true, false,
                                   ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000),
                                   ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000),
                                   ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (label3 = new Label ("new label",
                                           TRANS("Temporarily removed labels to avoid making wider boxes.")));
    label3->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label3->setJustificationType (Justification::centred);
    label3->setEditable (false, false, false);
    label3->setColour (Label::textColourId, Colours::green);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (LineBorderButton1C2 = new ImageButton ("Line Border 1C"));
    LineBorderButton1C2->setButtonText (TRANS("new button"));

    LineBorderButton1C2->setImages (false, true, false,
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (LineBorderButton1A2 = new ImageButton ("Line Border 1A"));
    LineBorderButton1A2->setButtonText (TRANS("new button"));

    LineBorderButton1A2->setImages (false, true, false,
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (LineBorderButton1B2 = new ImageButton ("Line Border 1B"));
    LineBorderButton1B2->setButtonText (TRANS("new button"));

    LineBorderButton1B2->setImages (false, true, false,
                                    ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (LineBorderButton1C3 = new ImageButton ("Line Border 1C"));
    LineBorderButton1C3->setButtonText (TRANS("new button"));

    LineBorderButton1C3->setImages (false, true, false,
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_horiz_png, line_border_horiz_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (LineBorderButton1B3 = new ImageButton ("Line Border 1B"));
    LineBorderButton1B3->setButtonText (TRANS("new button"));

    LineBorderButton1B3->setImages (false, true, false,
                                    ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000),
                                    ImageCache::getFromMemory (line_border_vert_png, line_border_vert_pngSize), 0.600f, Colour (0x00000000));
    addAndMakeVisible (algoSwitchButtonOffEx1 = new ImageButton ("Algorithm Switch Off AM"));
    algoSwitchButtonOffEx1->setButtonText (TRANS("new button"));
    algoSwitchButtonOffEx1->addListener (this);

    algoSwitchButtonOffEx1->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (algoSwitchButtonOffEx2 = new ImageButton ("Algorithm Switch Off FM"));
    algoSwitchButtonOffEx2->setButtonText (TRANS("new button"));
    algoSwitchButtonOffEx2->addListener (this);

    algoSwitchButtonOffEx2->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (algoSwitchButtonOnEx1 = new ImageButton ("Algorithm Switch On AM"));
    algoSwitchButtonOnEx1->setButtonText (TRANS("new button"));
    algoSwitchButtonOnEx1->addListener (this);

    algoSwitchButtonOnEx1->setImages (false, true, true,
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000),
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000),
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (algoSwitchButtonOnEx2 = new ImageButton ("Algorithm Switch On FM"));
    algoSwitchButtonOnEx2->setButtonText (TRANS("new button"));
    algoSwitchButtonOnEx2->addListener (this);

    algoSwitchButtonOnEx2->setImages (false, true, true,
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000),
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000),
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label4 = new Label ("new label",
                                           TRANS("AM")));
    label4->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label4->setJustificationType (Justification::centredLeft);
    label4->setEditable (false, false, false);
    label4->setColour (Label::textColourId, Colours::green);
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label5 = new Label ("new label",
                                           TRANS("FM")));
    label5->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label5->setJustificationType (Justification::centredLeft);
    label5->setEditable (false, false, false);
    label5->setColour (Label::textColourId, Colours::green);
    label5->setColour (TextEditor::textColourId, Colours::black);
    label5->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label6 = new Label ("new label",
                                           TRANS("AM")));
    label6->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label6->setJustificationType (Justification::centredLeft);
    label6->setEditable (false, false, false);
    label6->setColour (Label::textColourId, Colours::black);
    label6->setColour (TextEditor::textColourId, Colours::black);
    label6->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label7 = new Label ("new label",
                                           TRANS("FM")));
    label7->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label7->setJustificationType (Justification::centredLeft);
    label7->setEditable (false, false, false);
    label7->setColour (Label::textColourId, Colours::black);
    label7->setColour (TextEditor::textColourId, Colours::black);
    label7->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label8 = new Label ("new label",
                                           TRANS("Example AM/FM switches")));
    label8->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label8->setJustificationType (Justification::centred);
    label8->setEditable (false, false, false);
    label8->setColour (Label::textColourId, Colours::green);
    label8->setColour (TextEditor::textColourId, Colours::black);
    label8->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (algoSwitchButtonOn2Ex1 = new ImageButton ("Algorithm Switch On2 AM"));
    algoSwitchButtonOn2Ex1->setButtonText (TRANS("new button"));
    algoSwitchButtonOn2Ex1->addListener (this);

    algoSwitchButtonOn2Ex1->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_on2_png, algo_switch_on2_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on2_png, algo_switch_on2_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on2_png, algo_switch_on2_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (algoSwitchButtonOn2Ex2 = new ImageButton ("Algorithm Switch On2 FM"));
    algoSwitchButtonOn2Ex2->setButtonText (TRANS("new button"));
    algoSwitchButtonOn2Ex2->addListener (this);

    algoSwitchButtonOn2Ex2->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_on2_png, algo_switch_on2_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on2_png, algo_switch_on2_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on2_png, algo_switch_on2_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label9 = new Label ("new label",
                                           TRANS("AM")));
    label9->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label9->setJustificationType (Justification::centredLeft);
    label9->setEditable (false, false, false);
    label9->setColour (Label::textColourId, Colours::black);
    label9->setColour (TextEditor::textColourId, Colours::black);
    label9->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label10 = new Label ("new label",
                                            TRANS("FM")));
    label10->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label10->setJustificationType (Justification::centredLeft);
    label10->setEditable (false, false, false);
    label10->setColour (Label::textColourId, Colours::black);
    label10->setColour (TextEditor::textColourId, Colours::black);
    label10->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (algoSwitchButtonOn3Ex1 = new ImageButton ("Algorithm Switch On3 AM"));
    algoSwitchButtonOn3Ex1->setButtonText (TRANS("new button"));
    algoSwitchButtonOn3Ex1->addListener (this);

    algoSwitchButtonOn3Ex1->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (algoSwitchButtonOn3Ex2 = new ImageButton ("Algorithm Switch On3 FM"));
    algoSwitchButtonOn3Ex2->setButtonText (TRANS("new button"));
    algoSwitchButtonOn3Ex2->addListener (this);

    algoSwitchButtonOn3Ex2->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label11 = new Label ("new label",
                                            TRANS("AM")));
    label11->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label11->setJustificationType (Justification::centredLeft);
    label11->setEditable (false, false, false);
    label11->setColour (Label::textColourId, Colours::black);
    label11->setColour (TextEditor::textColourId, Colours::black);
    label11->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label12 = new Label ("new label",
                                            TRANS("FM")));
    label12->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label12->setJustificationType (Justification::centredLeft);
    label12->setEditable (false, false, false);
    label12->setColour (Label::textColourId, Colours::black);
    label12->setColour (TextEditor::textColourId, Colours::black);
    label12->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (TwoOpAMButton = new ImageButton ("Two OP AM Button"));
    TwoOpAMButton->setButtonText (TRANS("new button"));
    TwoOpAMButton->addListener (this);

    TwoOpAMButton->setImages (false, true, false,
                              ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 1.000f, Colour (0x00000000),
                              ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 1.000f, Colour (0x00000000),
                              ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (TwoOpFMButton = new ImageButton ("Two OP FM Button"));
    TwoOpFMButton->setButtonText (TRANS("new button"));
    TwoOpFMButton->addListener (this);

    TwoOpFMButton->setImages (false, true, true,
                              ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 1.000f, Colour (0x00000000),
                              ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 1.000f, Colour (0x00000000),
                              ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label13 = new Label ("new label",
                                            TRANS("M")));
    label13->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label13->setJustificationType (Justification::centred);
    label13->setEditable (false, false, false);
    label13->setColour (TextEditor::textColourId, Colours::black);
    label13->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label14 = new Label ("new label",
                                            TRANS("C")));
    label14->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label14->setJustificationType (Justification::centred);
    label14->setEditable (false, false, false);
    label14->setColour (TextEditor::textColourId, Colours::black);
    label14->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label15 = new Label ("new label",
                                            TRANS("M")));
    label15->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label15->setJustificationType (Justification::centred);
    label15->setEditable (false, false, false);
    label15->setColour (TextEditor::textColourId, Colours::black);
    label15->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label16 = new Label ("new label",
                                            TRANS("C")));
    label16->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label16->setJustificationType (Justification::centred);
    label16->setEditable (false, false, false);
    label16->setColour (TextEditor::textColourId, Colours::black);
    label16->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label17 = new Label ("new label",
                                            TRANS("Example Algorithms")));
    label17->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label17->setJustificationType (Justification::centred);
    label17->setEditable (false, false, false);
    label17->setColour (Label::textColourId, Colours::green);
    label17->setColour (TextEditor::textColourId, Colours::black);
    label17->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (groupComponent6 = new GroupComponent ("new group",
                                                             String()));
    groupComponent6->setColour (GroupComponent::outlineColourId, Colours::green);

    addAndMakeVisible (algoSwitchButtonOnEx3 = new ImageButton ("Algorithm Switch On AM"));
    algoSwitchButtonOnEx3->setButtonText (TRANS("new button"));
    algoSwitchButtonOnEx3->addListener (this);

    algoSwitchButtonOnEx3->setImages (false, true, true,
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000),
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000),
                                      ImageCache::getFromMemory (algo_switch_on_png, algo_switch_on_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label18 = new Label ("new label",
                                            TRANS("AM")));
    label18->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label18->setJustificationType (Justification::centredLeft);
    label18->setEditable (false, false, false);
    label18->setColour (Label::textColourId, Colours::black);
    label18->setColour (TextEditor::textColourId, Colours::black);
    label18->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (algoSwitchButtonOffEx3 = new ImageButton ("Algorithm Switch Off FM"));
    algoSwitchButtonOffEx3->setButtonText (TRANS("new button"));
    algoSwitchButtonOffEx3->addListener (this);

    algoSwitchButtonOffEx3->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label19 = new Label ("new label",
                                            TRANS("FM")));
    label19->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label19->setJustificationType (Justification::centredLeft);
    label19->setEditable (false, false, false);
    label19->setColour (Label::textColourId, Colours::green);
    label19->setColour (TextEditor::textColourId, Colours::black);
    label19->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (TwoOpAMButton2 = new ImageButton ("Two OP AM Button"));
    TwoOpAMButton2->setButtonText (TRANS("new button"));
    TwoOpAMButton2->addListener (this);

    TwoOpAMButton2->setImages (false, true, false,
                               ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 1.000f, Colour (0x00000000),
                               ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 1.000f, Colour (0x00000000),
                               ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label20 = new Label ("new label",
                                            TRANS("M")));
    label20->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label20->setJustificationType (Justification::centred);
    label20->setEditable (false, false, false);
    label20->setColour (TextEditor::textColourId, Colours::black);
    label20->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label21 = new Label ("new label",
                                            TRANS("C")));
    label21->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label21->setJustificationType (Justification::centred);
    label21->setEditable (false, false, false);
    label21->setColour (TextEditor::textColourId, Colours::black);
    label21->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label22 = new Label ("new label",
                                            TRANS("Example Algo Sections w/ Diagram")));
    label22->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label22->setJustificationType (Justification::centred);
    label22->setEditable (false, false, false);
    label22->setColour (Label::textColourId, Colours::green);
    label22->setColour (TextEditor::textColourId, Colours::black);
    label22->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (algoSwitchButtonOffEx4 = new ImageButton ("Algorithm Switch Off AM"));
    algoSwitchButtonOffEx4->setButtonText (TRANS("new button"));
    algoSwitchButtonOffEx4->addListener (this);

    algoSwitchButtonOffEx4->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label23 = new Label ("new label",
                                            TRANS("AM")));
    label23->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label23->setJustificationType (Justification::centredLeft);
    label23->setEditable (false, false, false);
    label23->setColour (Label::textColourId, Colours::green);
    label23->setColour (TextEditor::textColourId, Colours::black);
    label23->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (algoSwitchButtonOn3Ex3 = new ImageButton ("Algorithm Switch On3 FM"));
    algoSwitchButtonOn3Ex3->setButtonText (TRANS("new button"));
    algoSwitchButtonOn3Ex3->addListener (this);

    algoSwitchButtonOn3Ex3->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label24 = new Label ("new label",
                                            TRANS("FM")));
    label24->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label24->setJustificationType (Justification::centredLeft);
    label24->setEditable (false, false, false);
    label24->setColour (Label::textColourId, Colours::black);
    label24->setColour (TextEditor::textColourId, Colours::black);
    label24->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (TwoOpFMButton2 = new ImageButton ("Two OP FM Button"));
    TwoOpFMButton2->setButtonText (TRANS("new button"));
    TwoOpFMButton2->addListener (this);

    TwoOpFMButton2->setImages (false, true, true,
                               ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 1.000f, Colour (0x00000000),
                               ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 1.000f, Colour (0x00000000),
                               ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label25 = new Label ("new label",
                                            TRANS("M")));
    label25->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label25->setJustificationType (Justification::centred);
    label25->setEditable (false, false, false);
    label25->setColour (TextEditor::textColourId, Colours::black);
    label25->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label26 = new Label ("new label",
                                            TRANS("C")));
    label26->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label26->setJustificationType (Justification::centred);
    label26->setEditable (false, false, false);
    label26->setColour (TextEditor::textColourId, Colours::black);
    label26->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (groupComponent7 = new GroupComponent ("new group",
                                                             String()));
    groupComponent7->setColour (GroupComponent::outlineColourId, Colours::green);

    addAndMakeVisible (algoSwitchButtonOffEx5 = new ImageButton ("Algorithm Switch Off AM"));
    algoSwitchButtonOffEx5->setButtonText (TRANS("new button"));
    algoSwitchButtonOffEx5->addListener (this);

    algoSwitchButtonOffEx5->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_off_png, algo_switch_off_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label27 = new Label ("new label",
                                            TRANS("AM")));
    label27->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label27->setJustificationType (Justification::centredLeft);
    label27->setEditable (false, false, false);
    label27->setColour (Label::textColourId, Colours::green);
    label27->setColour (TextEditor::textColourId, Colours::black);
    label27->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (algoSwitchButtonOn3Ex4 = new ImageButton ("Algorithm Switch On3 FM"));
    algoSwitchButtonOn3Ex4->setButtonText (TRANS("new button"));
    algoSwitchButtonOn3Ex4->addListener (this);

    algoSwitchButtonOn3Ex4->setImages (false, true, true,
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000),
                                       ImageCache::getFromMemory (algo_switch_on3_png, algo_switch_on3_pngSize), 1.000f, Colour (0x00000000));
    addAndMakeVisible (label28 = new Label ("new label",
                                            TRANS("FM")));
    label28->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label28->setJustificationType (Justification::centredLeft);
    label28->setEditable (false, false, false);
    label28->setColour (Label::textColourId, Colours::black);
    label28->setColour (TextEditor::textColourId, Colours::black);
    label28->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (groupComponent8 = new GroupComponent ("new group",
                                                             String()));
    groupComponent8->setColour (GroupComponent::outlineColourId, Colours::green);

    addAndMakeVisible (frequencyLabel9 = new Label ("frequency label",
                                                    TRANS("Algorithm")));
    frequencyLabel9->setTooltip (TRANS("In additive mode, carrier and modulator output are simply summed rather than modulated"));
    frequencyLabel9->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel9->setJustificationType (Justification::centredLeft);
    frequencyLabel9->setEditable (false, false, false);
    frequencyLabel9->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel9->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel9->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label29 = new Label ("new label",
                                            TRANS("Example Algo Section w/o Diagram")));
    label29->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label29->setJustificationType (Justification::centred);
    label29->setEditable (false, false, false);
    label29->setColour (Label::textColourId, Colours::green);
    label29->setColour (TextEditor::textColourId, Colours::black);
    label29->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label30 = new Label ("new label",
                                            TRANS("Off             On (Bright)          On (Dark)       On (Solid)")));
    label30->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label30->setJustificationType (Justification::centred);
    label30->setEditable (false, false, false);
    label30->setColour (Label::textColourId, Colours::green);
    label30->setColour (TextEditor::textColourId, Colours::black);
    label30->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (frequencyLabel10 = new Label ("frequency label",
                                                     TRANS("Keyscale Attenuation")));
    frequencyLabel10->setTooltip (TRANS("Attenuate amplitude with note frequency in dB per octave"));
    frequencyLabel10->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    frequencyLabel10->setJustificationType (Justification::centred);
    frequencyLabel10->setEditable (false, false, false);
    frequencyLabel10->setColour (Label::textColourId, Colour (0xff007f00));
    frequencyLabel10->setColour (TextEditor::textColourId, Colours::black);
    frequencyLabel10->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (attenuationLabel5 = new Label ("attenuation label",
                                                      TRANS("Velocity Sensitivity")));
    attenuationLabel5->setTooltip (TRANS("Set or disable velocity senstivity"));
    attenuationLabel5->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    attenuationLabel5->setJustificationType (Justification::centred);
    attenuationLabel5->setEditable (false, false, false);
    attenuationLabel5->setColour (Label::textColourId, Colour (0xff007f00));
    attenuationLabel5->setColour (TextEditor::textColourId, Colours::black);
    attenuationLabel5->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (fmButton = new ImageButton ("fm button"));
    fmButton->setTooltip (TRANS("FM: carrier frequency is modulated by the modulator"));
    fmButton->setButtonText (TRANS("FM"));
    fmButton->setRadioGroupId (3);
    fmButton->addListener (this);

    fmButton->setImages (false, true, true,
                         ImageCache::getFromMemory (twoopAm_png, twoopAm_pngSize), 0.500f, Colour (0x00000000),
                         Image(), 0.500f, Colour (0x00000000),
                         Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (additiveButton = new ImageButton ("Additive mode button"));
    additiveButton->setTooltip (TRANS("Additive: output the sum of the modulator and carrier"));
    additiveButton->setButtonText (TRANS("Additive Mode"));
    additiveButton->setRadioGroupId (3);
    additiveButton->addListener (this);

    additiveButton->setImages (false, true, true,
                               ImageCache::getFromMemory (twoopFm_png, twoopFm_pngSize), 0.500f, Colour (0x00000000),
                               Image(), 0.500f, Colour (0x00000000),
                               Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (bassDrumButton = new ImageButton ("bass drum button"));
    bassDrumButton->setTooltip (TRANS("Bass drum"));
    bassDrumButton->setButtonText (TRANS("bass drum"));
    bassDrumButton->setRadioGroupId (4);
    bassDrumButton->addListener (this);

    bassDrumButton->setImages (false, true, true,
                               ImageCache::getFromMemory (bassdrum_png, bassdrum_pngSize), 0.500f, Colour (0x00000000),
                               Image(), 0.500f, Colour (0x00000000),
                               Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (snareDrumButton = new ImageButton ("snare drum button"));
    snareDrumButton->setTooltip (TRANS("Snare"));
    snareDrumButton->setButtonText (TRANS("snare"));
    snareDrumButton->setRadioGroupId (4);
    snareDrumButton->addListener (this);

    snareDrumButton->setImages (false, true, true,
                                ImageCache::getFromMemory (snare_png, snare_pngSize), 0.500f, Colour (0x00000000),
                                Image(), 0.500f, Colour (0x00000000),
                                Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (disablePercussionButton = new ImageButton ("percussion disabled button"));
    disablePercussionButton->setTooltip (TRANS("Disable percussion"));
    disablePercussionButton->setButtonText (TRANS("disabled"));
    disablePercussionButton->setRadioGroupId (4);
    disablePercussionButton->addListener (this);

    disablePercussionButton->setImages (false, true, true,
                                        ImageCache::getFromMemory (disabled_png, disabled_pngSize), 0.500f, Colour (0x00000000),
                                        Image(), 0.500f, Colour (0x00000000),
                                        Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (tomTomButton = new ImageButton ("tom tom button"));
    tomTomButton->setTooltip (TRANS("Tom-tom"));
    tomTomButton->setButtonText (TRANS("tom tom"));
    tomTomButton->setRadioGroupId (4);
    tomTomButton->addListener (this);

    tomTomButton->setImages (false, true, true,
                             ImageCache::getFromMemory (tom_png, tom_pngSize), 0.500f, Colour (0x00000000),
                             Image(), 0.500f, Colour (0x00000000),
                             Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (cymbalButton = new ImageButton ("cymbalButton"));
    cymbalButton->setTooltip (TRANS("Cymbal"));
    cymbalButton->setButtonText (TRANS("snare"));
    cymbalButton->setRadioGroupId (4);
    cymbalButton->addListener (this);

    cymbalButton->setImages (false, true, true,
                             ImageCache::getFromMemory (cymbal_png, cymbal_pngSize), 0.500f, Colour (0x00000000),
                             Image(), 0.500f, Colour (0x00000000),
                             Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (hiHatButton = new ImageButton ("hi hat button"));
    hiHatButton->setTooltip (TRANS("Hi-hat"));
    hiHatButton->setButtonText (TRANS("hi-hat"));
    hiHatButton->setRadioGroupId (4);
    hiHatButton->addListener (this);

    hiHatButton->setImages (false, true, true,
                            ImageCache::getFromMemory (hihat_png, hihat_pngSize), 0.500f, Colour (0x00000000),
                            Image(), 0.500f, Colour (0x00000000),
                            Image(), 1.000f, Colour (0x00000000));
    addAndMakeVisible (dbLabel7 = new Label ("db label",
                                             TRANS("FM")));
    dbLabel7->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel7->setJustificationType (Justification::centredLeft);
    dbLabel7->setEditable (false, false, false);
    dbLabel7->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel7->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel7->setColour (TextEditor::textColourId, Colours::black);
    dbLabel7->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dbLabel8 = new Label ("db label",
                                             TRANS("Additive")));
    dbLabel8->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    dbLabel8->setJustificationType (Justification::centred);
    dbLabel8->setEditable (false, false, false);
    dbLabel8->setColour (Label::textColourId, Colour (0xff007f00));
    dbLabel8->setColour (Label::outlineColourId, Colour (0x00000000));
    dbLabel8->setColour (TextEditor::textColourId, Colours::black);
    dbLabel8->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]
	LookAndFeel::setDefaultLookAndFeel(new OPLLookAndFeel());

	frequencyComboBox->setColour (ComboBox::textColourId, Colour (COLOUR_MID));
	frequencyComboBox->setColour (ComboBox::outlineColourId, Colour (COLOUR_MID));
	frequencyComboBox->setColour (ComboBox::arrowColourId, Colour (COLOUR_MID));
	frequencyComboBox->setColour (ComboBox::buttonColourId, Colours::black);
	frequencyComboBox->setColour (ComboBox::backgroundColourId, Colours::black);

	frequencyComboBox->addItem ("x1/2", 16);		// can't use 0 :(
    frequencyComboBox->addItem ("x1", 1);
    frequencyComboBox->addItem ("x2", 2);
    frequencyComboBox->addItem ("x3", 3);
    frequencyComboBox->addItem ("x4", 4);
    frequencyComboBox->addItem ("x5", 5);
    frequencyComboBox->addItem ("x6", 6);
    frequencyComboBox->addItem ("x7", 7);
    frequencyComboBox->addItem ("x8", 8);
    frequencyComboBox->addItem ("x9", 9);
    frequencyComboBox->addItem ("x10", 10);
    frequencyComboBox->addItem ("x12", 12);
    frequencyComboBox->addItem ("x15", 15);

	frequencyComboBox2->setColour (ComboBox::textColourId, Colour (COLOUR_MID));
	frequencyComboBox2->setColour (ComboBox::outlineColourId, Colour (COLOUR_MID));
	frequencyComboBox2->setColour (ComboBox::arrowColourId, Colour (COLOUR_MID));
	frequencyComboBox2->setColour (ComboBox::buttonColourId, Colours::black);
	frequencyComboBox2->setColour (ComboBox::backgroundColourId, Colours::black);
	frequencyComboBox2->addItem ("x1/2", 16);		// can't use 0 :(
    frequencyComboBox2->addItem ("x1", 1);
    frequencyComboBox2->addItem ("x2", 2);
    frequencyComboBox2->addItem ("x3", 3);
    frequencyComboBox2->addItem ("x4", 4);
    frequencyComboBox2->addItem ("x5", 5);
    frequencyComboBox2->addItem ("x6", 6);
    frequencyComboBox2->addItem ("x7", 7);
    frequencyComboBox2->addItem ("x8", 8);
    frequencyComboBox2->addItem ("x9", 9);
    frequencyComboBox2->addItem ("x10", 10);
    frequencyComboBox2->addItem ("x12", 12);
    frequencyComboBox2->addItem ("x15", 15);

	velocityComboBox->setColour (ComboBox::textColourId, Colour (COLOUR_MID));
	velocityComboBox->setColour (ComboBox::outlineColourId, Colour (COLOUR_MID));
	velocityComboBox->setColour (ComboBox::arrowColourId, Colour (COLOUR_MID));
	velocityComboBox->setColour (ComboBox::buttonColourId, Colours::black);
	velocityComboBox->setColour (ComboBox::backgroundColourId, Colours::black);
	velocityComboBox2->setColour (ComboBox::textColourId, Colour (COLOUR_MID));
	velocityComboBox2->setColour (ComboBox::outlineColourId, Colour (COLOUR_MID));
	velocityComboBox2->setColour (ComboBox::arrowColourId, Colour (COLOUR_MID));
	velocityComboBox2->setColour (ComboBox::buttonColourId, Colours::black);
	velocityComboBox2->setColour (ComboBox::backgroundColourId, Colours::black);

	keyscaleAttenuationComboBox->setColour (ComboBox::textColourId, Colour (COLOUR_MID));
	keyscaleAttenuationComboBox->setColour (ComboBox::outlineColourId, Colour (COLOUR_MID));
	keyscaleAttenuationComboBox->setColour (ComboBox::arrowColourId, Colour (COLOUR_MID));
	keyscaleAttenuationComboBox->setColour (ComboBox::buttonColourId, Colours::black);
	keyscaleAttenuationComboBox->setColour (ComboBox::backgroundColourId, Colours::black);
	keyscaleAttenuationComboBox2->setColour (ComboBox::textColourId, Colour (COLOUR_MID));
	keyscaleAttenuationComboBox2->setColour (ComboBox::outlineColourId, Colour (COLOUR_MID));
	keyscaleAttenuationComboBox2->setColour (ComboBox::arrowColourId, Colour (COLOUR_MID));
	keyscaleAttenuationComboBox2->setColour (ComboBox::buttonColourId, Colours::black);
	keyscaleAttenuationComboBox2->setColour (ComboBox::backgroundColourId, Colours::black);

	sineImageButton->setClickingTogglesState(true);
	sineImageButton->setRepaintsOnMouseActivity(false);
	abssineImageButton->setClickingTogglesState(true);
	abssineImageButton->setRepaintsOnMouseActivity(false);
	halfsineImageButton->setClickingTogglesState(true);
	halfsineImageButton->setRepaintsOnMouseActivity(false);
	quartersineImageButton->setClickingTogglesState(true);
	quartersineImageButton->setRepaintsOnMouseActivity(false);
	alternatingsineImageButton->setClickingTogglesState(true);
	alternatingsineImageButton->setRepaintsOnMouseActivity(false);
	camelsineImageButton->setClickingTogglesState(true);
	camelsineImageButton->setRepaintsOnMouseActivity(false);
	squareImageButton->setClickingTogglesState(true);
	squareImageButton->setRepaintsOnMouseActivity(false);
	logsawImageButton->setClickingTogglesState(true);
	logsawImageButton->setRepaintsOnMouseActivity(false);

	sineImageButton2->setClickingTogglesState(true);
	sineImageButton2->setRepaintsOnMouseActivity(false);
	abssineImageButton2->setClickingTogglesState(true);
	abssineImageButton2->setRepaintsOnMouseActivity(false);
	halfsineImageButton2->setClickingTogglesState(true);
	halfsineImageButton2->setRepaintsOnMouseActivity(false);
	quartersineImageButton2->setClickingTogglesState(true);
	quartersineImageButton2->setRepaintsOnMouseActivity(false);
	alternatingsineImageButton2->setClickingTogglesState(true);
	alternatingsineImageButton2->setRepaintsOnMouseActivity(false);
	camelsineImageButton2->setClickingTogglesState(true);
	camelsineImageButton2->setRepaintsOnMouseActivity(false);
	squareImageButton2->setClickingTogglesState(true);
	squareImageButton2->setRepaintsOnMouseActivity(false);
	logsawImageButton2->setClickingTogglesState(true);
	logsawImageButton2->setRepaintsOnMouseActivity(false);

	fmButton->setClickingTogglesState(true);
	fmButton->setRepaintsOnMouseActivity(false);
	additiveButton->setClickingTogglesState(true);
	additiveButton->setRepaintsOnMouseActivity(false);

	disablePercussionButton->setClickingTogglesState(true);
	disablePercussionButton->setRepaintsOnMouseActivity(false);
	bassDrumButton->setClickingTogglesState(true);
	bassDrumButton->setRepaintsOnMouseActivity(false);
	snareDrumButton->setClickingTogglesState(true);
	snareDrumButton->setRepaintsOnMouseActivity(false);

	tomTomButton->setClickingTogglesState(true);
	tomTomButton->setRepaintsOnMouseActivity(false);
	cymbalButton->setClickingTogglesState(true);
	cymbalButton->setRepaintsOnMouseActivity(false);
	hiHatButton->setClickingTogglesState(true);
	hiHatButton->setRepaintsOnMouseActivity(false);

	recordButton->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	tremoloButton->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	vibratoButton->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	keyscaleEnvButton->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	sustainButton->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	tremoloButton2->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	vibratoButton2->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	keyscaleEnvButton2->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));
	sustainButton2->setColour(TextButton::buttonColourId, Colour(COLOUR_MID));

	Font fw(Font::getDefaultMonospacedFontName(), 14, Font::bold);
	ChannelButtonLookAndFeel *channelButtonLookAndFeel = new ChannelButtonLookAndFeel();
	String context = String("Disable channel ");
	for (unsigned int i = 0; i < channels.size(); ++i)
	{
		TextButton *channel = new TextButton(TRANS("-"), context + String(i + 1));
		channel->setLookAndFeel(channelButtonLookAndFeel);
		channel->setColour(TextButton::ColourIds::buttonColourId, Colours::black);
		channel->setColour(TextButton::ColourIds::buttonOnColourId, Colours::black);
		channel->setColour(TextButton::ColourIds::textColourOnId, OPLLookAndFeel::DOS_GREEN);
		channel->setColour(TextButton::ColourIds::textColourOffId, OPLLookAndFeel::DOS_GREEN);
		channel->addListener(this);
		addAndMakeVisible(channel);
		channels[i] = channel;
	}
	versionLabel->setText(String(ProjectInfo::versionString), NotificationType::dontSendNotification);
    //[/UserPreSize]

    setSize (860, 580);


    //[Constructor] You can add your own custom stuff here..
	processor = ownerFilter;
	startTimer(1000/30);
    //[/Constructor]
}

PluginGui::~PluginGui()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    groupComponent2 = nullptr;
    groupComponent4 = nullptr;
    groupComponent11 = nullptr;
    groupComponent10 = nullptr;
    groupComponent9 = nullptr;
    groupComponent = nullptr;
    frequencyComboBox = nullptr;
    frequencyLabel = nullptr;
    aSlider = nullptr;
    aLabel = nullptr;
    dSlider = nullptr;
    dLabel = nullptr;
    sSlider = nullptr;
    dLabel2 = nullptr;
    rSlider = nullptr;
    rLabel = nullptr;
    attenuationSlider = nullptr;
    attenuationLabel = nullptr;
    dbLabel = nullptr;
    sineImageButton = nullptr;
    halfsineImageButton = nullptr;
    abssineImageButton = nullptr;
    quartersineImageButton = nullptr;
    waveLabel = nullptr;
    tremoloButton = nullptr;
    vibratoButton = nullptr;
    sustainButton = nullptr;
    keyscaleEnvButton = nullptr;
    dbLabel2 = nullptr;
    frequencyComboBox2 = nullptr;
    frequencyLabel3 = nullptr;
    aSlider2 = nullptr;
    aLabel2 = nullptr;
    dSlider2 = nullptr;
    dLabel3 = nullptr;
    sSlider2 = nullptr;
    dLabel4 = nullptr;
    rSlider2 = nullptr;
    rLabel2 = nullptr;
    attenuationSlider2 = nullptr;
    attenuationLabel2 = nullptr;
    dbLabel3 = nullptr;
    sineImageButton2 = nullptr;
    halfsineImageButton2 = nullptr;
    abssineImageButton2 = nullptr;
    quartersineImageButton2 = nullptr;
    waveLabel2 = nullptr;
    tremoloButton2 = nullptr;
    vibratoButton2 = nullptr;
    sustainButton2 = nullptr;
    keyscaleEnvButton2 = nullptr;
    frequencyLabel4 = nullptr;
    groupComponent3 = nullptr;
    tremoloSlider = nullptr;
    frequencyLabel5 = nullptr;
    dbLabel5 = nullptr;
    vibratoSlider = nullptr;
    frequencyLabel6 = nullptr;
    dbLabel6 = nullptr;
    feedbackSlider = nullptr;
    frequencyLabel7 = nullptr;
    velocityComboBox = nullptr;
    velocityComboBox2 = nullptr;
    attenuationLabel4 = nullptr;
    alternatingsineImageButton = nullptr;
    camelsineImageButton = nullptr;
    squareImageButton = nullptr;
    logsawImageButton = nullptr;
    alternatingsineImageButton2 = nullptr;
    camelsineImageButton2 = nullptr;
    squareImageButton2 = nullptr;
    logsawImageButton2 = nullptr;
    dbLabel4 = nullptr;
    keyscaleAttenuationComboBox2 = nullptr;
    keyscaleAttenuationComboBox = nullptr;
    groupComponent5 = nullptr;
    emulatorSlider = nullptr;
    emulatorLabel = nullptr;
    emulatorLabel2 = nullptr;
    recordButton = nullptr;
    exportButton = nullptr;
    loadButton = nullptr;
    versionLabel = nullptr;
    ToggleButtonOffExample = nullptr;
    ToggleButtonOnExample = nullptr;
    label = nullptr;
    label2 = nullptr;
    LineBorderButton1C = nullptr;
    LineBorderButton1A = nullptr;
    LineBorderButton1B = nullptr;
    label3 = nullptr;
    LineBorderButton1C2 = nullptr;
    LineBorderButton1A2 = nullptr;
    LineBorderButton1B2 = nullptr;
    LineBorderButton1C3 = nullptr;
    LineBorderButton1B3 = nullptr;
    algoSwitchButtonOffEx1 = nullptr;
    algoSwitchButtonOffEx2 = nullptr;
    algoSwitchButtonOnEx1 = nullptr;
    algoSwitchButtonOnEx2 = nullptr;
    label4 = nullptr;
    label5 = nullptr;
    label6 = nullptr;
    label7 = nullptr;
    label8 = nullptr;
    algoSwitchButtonOn2Ex1 = nullptr;
    algoSwitchButtonOn2Ex2 = nullptr;
    label9 = nullptr;
    label10 = nullptr;
    algoSwitchButtonOn3Ex1 = nullptr;
    algoSwitchButtonOn3Ex2 = nullptr;
    label11 = nullptr;
    label12 = nullptr;
    TwoOpAMButton = nullptr;
    TwoOpFMButton = nullptr;
    label13 = nullptr;
    label14 = nullptr;
    label15 = nullptr;
    label16 = nullptr;
    label17 = nullptr;
    groupComponent6 = nullptr;
    algoSwitchButtonOnEx3 = nullptr;
    label18 = nullptr;
    algoSwitchButtonOffEx3 = nullptr;
    label19 = nullptr;
    TwoOpAMButton2 = nullptr;
    label20 = nullptr;
    label21 = nullptr;
    label22 = nullptr;
    algoSwitchButtonOffEx4 = nullptr;
    label23 = nullptr;
    algoSwitchButtonOn3Ex3 = nullptr;
    label24 = nullptr;
    TwoOpFMButton2 = nullptr;
    label25 = nullptr;
    label26 = nullptr;
    groupComponent7 = nullptr;
    algoSwitchButtonOffEx5 = nullptr;
    label27 = nullptr;
    algoSwitchButtonOn3Ex4 = nullptr;
    label28 = nullptr;
    groupComponent8 = nullptr;
    frequencyLabel9 = nullptr;
    label29 = nullptr;
    label30 = nullptr;
    frequencyLabel10 = nullptr;
    attenuationLabel5 = nullptr;
    fmButton = nullptr;
    additiveButton = nullptr;
    bassDrumButton = nullptr;
    snareDrumButton = nullptr;
    disablePercussionButton = nullptr;
    tomTomButton = nullptr;
    cymbalButton = nullptr;
    hiHatButton = nullptr;
    dbLabel7 = nullptr;
    dbLabel8 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PluginGui::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::black);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PluginGui::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    groupComponent2->setBounds (440, 88, 408, 344);
    groupComponent4->setBounds (16, 8, 832, 64);
    groupComponent11->setBounds (496, 440, 192, 120);
    groupComponent10->setBounds (280, 440, 200, 120);
    groupComponent9->setBounds (704, 440, 144, 120);
    groupComponent->setBounds (16, 88, 408, 344);
    frequencyComboBox->setBounds (200, 168, 64, 24);
    frequencyLabel->setBounds (40, 168, 152, 24);
    aSlider->setBounds (40, 208, 30, 88);
    aLabel->setBounds (40, 304, 30, 24);
    dSlider->setBounds (88, 208, 30, 88);
    dLabel->setBounds (88, 304, 30, 24);
    sSlider->setBounds (136, 208, 30, 88);
    dLabel2->setBounds (136, 304, 30, 24);
    rSlider->setBounds (184, 208, 30, 88);
    rLabel->setBounds (184, 304, 30, 24);
    attenuationSlider->setBounds (328, 184, 56, 142);
    attenuationLabel->setBounds (304, 160, 112, 24);
    dbLabel->setBounds (384, 304, 32, 24);
    sineImageButton->setBounds (88, 113, 34, 30);
    halfsineImageButton->setBounds (128, 113, 34, 30);
    abssineImageButton->setBounds (168, 113, 34, 30);
    quartersineImageButton->setBounds (208, 113, 34, 30);
    waveLabel->setBounds (32, 115, 48, 24);
    tremoloButton->setBounds (120, 352, 80, 24);
    vibratoButton->setBounds (32, 352, 72, 24);
    sustainButton->setBounds (32, 384, 70, 32);
    keyscaleEnvButton->setBounds (120, 376, 101, 48);
    dbLabel2->setBounds (792, 712, 72, 16);
    frequencyComboBox2->setBounds (624, 168, 66, 24);
    frequencyLabel3->setBounds (464, 168, 152, 24);
    aSlider2->setBounds (464, 208, 30, 88);
    aLabel2->setBounds (464, 304, 30, 24);
    dSlider2->setBounds (512, 208, 30, 88);
    dLabel3->setBounds (512, 304, 30, 24);
    sSlider2->setBounds (560, 208, 30, 88);
    dLabel4->setBounds (560, 304, 30, 24);
    rSlider2->setBounds (608, 208, 30, 88);
    rLabel2->setBounds (608, 304, 30, 24);
    attenuationSlider2->setBounds (752, 184, 56, 142);
    attenuationLabel2->setBounds (728, 160, 112, 24);
    dbLabel3->setBounds (800, 304, 40, 24);
    sineImageButton2->setBounds (512, 113, 34, 30);
    halfsineImageButton2->setBounds (552, 113, 34, 30);
    abssineImageButton2->setBounds (592, 113, 34, 30);
    quartersineImageButton2->setBounds (632, 113, 34, 30);
    waveLabel2->setBounds (456, 115, 48, 24);
    tremoloButton2->setBounds (544, 352, 80, 24);
    vibratoButton2->setBounds (456, 352, 72, 24);
    sustainButton2->setBounds (456, 384, 70, 24);
    keyscaleEnvButton2->setBounds (544, 376, 102, 48);
    frequencyLabel4->setBounds (656, 376, 88, 48);
    groupComponent3->setBounds (16, 440, 248, 120);
    tremoloSlider->setBounds (112, 472, 80, 24);
    frequencyLabel5->setBounds (32, 472, 80, 24);
    dbLabel5->setBounds (200, 464, 32, 40);
    vibratoSlider->setBounds (112, 512, 80, 24);
    frequencyLabel6->setBounds (32, 512, 80, 24);
    dbLabel6->setBounds (200, 504, 48, 40);
    feedbackSlider->setBounds (248, 237, 30, 59);
    frequencyLabel7->setBounds (224, 304, 80, 24);
    velocityComboBox->setBounds (328, 352, 76, 24);
    velocityComboBox2->setBounds (760, 352, 72, 24);
    attenuationLabel4->setBounds (760, 376, 80, 48);
    alternatingsineImageButton->setBounds (288, 113, 34, 30);
    camelsineImageButton->setBounds (248, 113, 34, 30);
    squareImageButton->setBounds (328, 113, 34, 30);
    logsawImageButton->setBounds (368, 113, 34, 30);
    alternatingsineImageButton2->setBounds (714, 114, 34, 30);
    camelsineImageButton2->setBounds (674, 114, 34, 30);
    squareImageButton2->setBounds (754, 114, 34, 30);
    logsawImageButton2->setBounds (794, 114, 34, 30);
    dbLabel4->setBounds (792, 688, 72, 16);
    keyscaleAttenuationComboBox2->setBounds (664, 352, 76, 24);
    keyscaleAttenuationComboBox->setBounds (232, 352, 76, 24);
    groupComponent5->setBounds (24, 712, 408, 64);
    emulatorSlider->setBounds (208, 736, 40, 24);
    emulatorLabel->setBounds (120, 736, 72, 24);
    emulatorLabel2->setBounds (256, 736, 72, 24);
    recordButton->setBounds (32, 680, 224, 24);
    exportButton->setBounds (728, 512, 96, 24);
    loadButton->setBounds (728, 472, 96, 24);
    versionLabel->setBounds (648, 560, 198, 16);
    ToggleButtonOffExample->setBounds (1032, 584, 12, 12);
    ToggleButtonOnExample->setBounds (1064, 584, 12, 12);
    label->setBounds (1000, 608, 104, 24);
    label2->setBounds (872, 608, 104, 24);
    LineBorderButton1C->setBounds (20, 336, 400, 6);
    LineBorderButton1A->setBounds (20, 152, 400, 6);
    LineBorderButton1B->setBounds (296, 156, 6, 182);
    label3->setBounds (776, 736, 104, 56);
    LineBorderButton1C2->setBounds (444, 336, 400, 6);
    LineBorderButton1A2->setBounds (444, 152, 400, 6);
    LineBorderButton1B2->setBounds (720, 156, 6, 182);
    LineBorderButton1C3->setBounds (892, 584, 20, 6);
    LineBorderButton1B3->setBounds (936, 576, 6, 20);
    algoSwitchButtonOffEx1->setBounds (952, 701, 64, 24);
    algoSwitchButtonOffEx2->setBounds (952, 727, 64, 24);
    algoSwitchButtonOnEx1->setBounds (1040, 701, 64, 24);
    algoSwitchButtonOnEx2->setBounds (1040, 727, 64, 24);
    label4->setBounds (970, 701, 32, 24);
    label5->setBounds (971, 727, 32, 24);
    label6->setBounds (1057, 701, 32, 24);
    label7->setBounds (1058, 727, 32, 24);
    label8->setBounds (944, 816, 320, 24);
    algoSwitchButtonOn2Ex1->setBounds (1128, 700, 64, 24);
    algoSwitchButtonOn2Ex2->setBounds (1128, 727, 64, 24);
    label9->setBounds (1145, 700, 32, 24);
    label10->setBounds (1146, 727, 32, 24);
    algoSwitchButtonOn3Ex1->setBounds (1216, 700, 64, 24);
    algoSwitchButtonOn3Ex2->setBounds (1216, 727, 64, 24);
    label11->setBounds (1233, 700, 31, 24);
    label12->setBounds (1234, 727, 32, 24);
    TwoOpAMButton->setBounds (1173, 484, 60, 56);
    TwoOpFMButton->setBounds (1156, 568, 80, 26);
    label13->setBounds (1179, 489, 24, 24);
    label14->setBounds (1179, 518, 24, 24);
    label15->setBounds (1166, 572, 24, 24);
    label16->setBounds (1195, 572, 24, 24);
    label17->setBounds (1128, 608, 136, 24);
    groupComponent6->setBounds (933, 56, 168, 95);
    algoSwitchButtonOnEx3->setBounds (949, 82, 64, 24);
    label18->setBounds (966, 82, 32, 24);
    algoSwitchButtonOffEx3->setBounds (949, 108, 64, 24);
    label19->setBounds (968, 108, 32, 24);
    TwoOpAMButton2->setBounds (1029, 77, 60, 56);
    label20->setBounds (1035, 82, 24, 24);
    label21->setBounds (1035, 111, 24, 24);
    label22->setBounds (952, 160, 328, 40);
    algoSwitchButtonOffEx4->setBounds (1125, 82, 64, 24);
    label23->setBounds (1143, 82, 32, 24);
    algoSwitchButtonOn3Ex3->setBounds (1125, 109, 64, 24);
    label24->setBounds (1143, 109, 32, 24);
    TwoOpFMButton2->setBounds (1196, 94, 80, 26);
    label25->setBounds (1206, 98, 24, 24);
    label26->setBounds (1235, 98, 24, 24);
    groupComponent7->setBounds (1112, 56, 168, 95);
    algoSwitchButtonOffEx5->setBounds (1037, 250, 64, 24);
    label27->setBounds (1055, 250, 32, 24);
    algoSwitchButtonOn3Ex4->setBounds (1103, 250, 64, 24);
    label28->setBounds (1121, 250, 32, 24);
    groupComponent8->setBounds (1008, 208, 168, 95);
    frequencyLabel9->setBounds (1067, 216, 72, 24);
    label29->setBounds (944, 304, 328, 40);
    label30->setBounds (961, 768, 319, 24);
    frequencyLabel10->setBounds (224, 376, 88, 48);
    attenuationLabel5->setBounds (328, 376, 80, 48);
    fmButton->setBounds (304, 472, 56, 56);
    additiveButton->setBounds (392, 464, 72, 72);
    bassDrumButton->setBounds (576, 464, 30, 30);
    snareDrumButton->setBounds (632, 464, 30, 30);
    disablePercussionButton->setBounds (520, 464, 30, 30);
    tomTomButton->setBounds (520, 512, 30, 30);
    cymbalButton->setBounds (576, 512, 30, 30);
    hiHatButton->setBounds (632, 512, 30, 30);
    dbLabel7->setBounds (320, 520, 32, 40);
    dbLabel8->setBounds (392, 520, 72, 40);
    //[UserResized] Add your own custom resize handling here..
	for (unsigned int i = 0; i < channels.size(); ++i)
		channels[i]->setBounds(64+88*i, 30, 28, 28);
    //[/UserResized]
}

void PluginGui::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]

    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == frequencyComboBox)
    {
        //[UserComboBoxCode_frequencyComboBox] -- add your combo box handling code here..
		int id = comboBoxThatHasChanged->getSelectedId();
		if (id > 15) id = 0;
		processor->setEnumParameter("Modulator Frequency Multiplier", id);
        //[/UserComboBoxCode_frequencyComboBox]
    }
    else if (comboBoxThatHasChanged == frequencyComboBox2)
    {
        //[UserComboBoxCode_frequencyComboBox2] -- add your combo box handling code here..
		int id = comboBoxThatHasChanged->getSelectedId();
		if (id > 15) id = 0;
		processor->setEnumParameter("Carrier Frequency Multiplier", id);
        //[/UserComboBoxCode_frequencyComboBox2]
    }
    else if (comboBoxThatHasChanged == velocityComboBox)
    {
        //[UserComboBoxCode_velocityComboBox] -- add your combo box handling code here..
		int id = comboBoxThatHasChanged->getSelectedId() - 1;
		processor->setEnumParameter("Modulator Velocity Sensitivity", id);
        //[/UserComboBoxCode_velocityComboBox]
    }
    else if (comboBoxThatHasChanged == velocityComboBox2)
    {
        //[UserComboBoxCode_velocityComboBox2] -- add your combo box handling code here..
		int id = comboBoxThatHasChanged->getSelectedId() - 1;
		processor->setEnumParameter("Carrier Velocity Sensitivity", id);
        //[/UserComboBoxCode_velocityComboBox2]
    }
    else if (comboBoxThatHasChanged == keyscaleAttenuationComboBox2)
    {
        //[UserComboBoxCode_keyscaleAttenuationComboBox2] -- add your combo box handling code here..
		int id = comboBoxThatHasChanged->getSelectedId() - 1;
		processor->setEnumParameter("Carrier Keyscale Level", id);
        //[/UserComboBoxCode_keyscaleAttenuationComboBox2]
    }
    else if (comboBoxThatHasChanged == keyscaleAttenuationComboBox)
    {
        //[UserComboBoxCode_keyscaleAttenuationComboBox] -- add your combo box handling code here..
		int id = comboBoxThatHasChanged->getSelectedId() - 1;
		processor->setEnumParameter("Modulator Keyscale Level", id);
        //[/UserComboBoxCode_keyscaleAttenuationComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void PluginGui::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == aSlider)
    {
        //[UserSliderCode_aSlider] -- add your slider handling code here..
		processor->setIntParameter("Modulator Attack", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_aSlider]
    }
    else if (sliderThatWasMoved == dSlider)
    {
        //[UserSliderCode_dSlider] -- add your slider handling code here..
		processor->setIntParameter("Modulator Decay", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_dSlider]
    }
    else if (sliderThatWasMoved == sSlider)
    {
        //[UserSliderCode_sSlider] -- add your slider handling code here..
		processor->setIntParameter("Modulator Sustain Level", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_sSlider]
    }
    else if (sliderThatWasMoved == rSlider)
    {
        //[UserSliderCode_rSlider] -- add your slider handling code here..
		processor->setIntParameter("Modulator Release", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_rSlider]
    }
    else if (sliderThatWasMoved == attenuationSlider)
    {
        //[UserSliderCode_attenuationSlider] -- add your slider handling code here..
		processor->setEnumParameter("Modulator Attenuation", -(int)(sliderThatWasMoved->getValue()/0.75));
        //[/UserSliderCode_attenuationSlider]
    }
    else if (sliderThatWasMoved == aSlider2)
    {
        //[UserSliderCode_aSlider2] -- add your slider handling code here..
		processor->setIntParameter("Carrier Attack", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_aSlider2]
    }
    else if (sliderThatWasMoved == dSlider2)
    {
        //[UserSliderCode_dSlider2] -- add your slider handling code here..
		processor->setIntParameter("Carrier Decay", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_dSlider2]
    }
    else if (sliderThatWasMoved == sSlider2)
    {
        //[UserSliderCode_sSlider2] -- add your slider handling code here..
		processor->setIntParameter("Carrier Sustain Level", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_sSlider2]
    }
    else if (sliderThatWasMoved == rSlider2)
    {
        //[UserSliderCode_rSlider2] -- add your slider handling code here..
		processor->setIntParameter("Carrier Release", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_rSlider2]
    }
    else if (sliderThatWasMoved == attenuationSlider2)
    {
        //[UserSliderCode_attenuationSlider2] -- add your slider handling code here..
		processor->setEnumParameter("Carrier Attenuation", -(int)(sliderThatWasMoved->getValue()/0.75));
        //[/UserSliderCode_attenuationSlider2]
    }
    else if (sliderThatWasMoved == tremoloSlider)
    {
        //[UserSliderCode_tremoloSlider] -- add your slider handling code here..
		processor->setEnumParameter("Tremolo Depth", sliderThatWasMoved->getValue() < 2.0 ? 0 : 1);
        //[/UserSliderCode_tremoloSlider]
    }
    else if (sliderThatWasMoved == vibratoSlider)
    {
        //[UserSliderCode_vibratoSlider] -- add your slider handling code here..
		processor->setEnumParameter("Vibrato Depth", sliderThatWasMoved->getValue() < 8.0 ? 0 : 1);
        //[/UserSliderCode_vibratoSlider]
    }
    else if (sliderThatWasMoved == feedbackSlider)
    {
        //[UserSliderCode_feedbackSlider] -- add your slider handling code here..
		processor->setIntParameter("Modulator Feedback", (int)sliderThatWasMoved->getValue());
        //[/UserSliderCode_feedbackSlider]
    }
    else if (sliderThatWasMoved == emulatorSlider)
    {
        //[UserSliderCode_emulatorSlider] -- add your slider handling code here..
        //[/UserSliderCode_emulatorSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void PluginGui::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
	// TODO:
	// - fix button tooltip text
	// - automatically select channel(s) by default?
	// - record output to file
	for (int i = 1; i <= Hiopl::CHANNELS; ++i) {
		Button* channelButton = channels[i - 1];
		if (buttonThatWasClicked == channelButton) {
			if (processor->nChannelsEnabled() > 1 || !processor->isChannelEnabled(i)) {
				processor->toggleChannel(i);
			}
			const bool channelEnabled = processor->isChannelEnabled(i);
			Colour textColour = channelEnabled ? OPLLookAndFeel::DOS_GREEN : OPLLookAndFeel::DOS_GREEN_DARK;
			channelButton->setColour(TextButton::ColourIds::textColourOnId, textColour);
			channelButton->setColour(TextButton::ColourIds::textColourOffId, textColour);
			return;
		}
	}
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == sineImageButton)
    {
        //[UserButtonCode_sineImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 0);
        //[/UserButtonCode_sineImageButton]
    }
    else if (buttonThatWasClicked == halfsineImageButton)
    {
        //[UserButtonCode_halfsineImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 1);
        //[/UserButtonCode_halfsineImageButton]
    }
    else if (buttonThatWasClicked == abssineImageButton)
    {
        //[UserButtonCode_abssineImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 2);
        //[/UserButtonCode_abssineImageButton]
    }
    else if (buttonThatWasClicked == quartersineImageButton)
    {
        //[UserButtonCode_quartersineImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 3);
        //[/UserButtonCode_quartersineImageButton]
    }
    else if (buttonThatWasClicked == tremoloButton)
    {
        //[UserButtonCode_tremoloButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Tremolo", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_tremoloButton]
    }
    else if (buttonThatWasClicked == vibratoButton)
    {
        //[UserButtonCode_vibratoButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Vibrato", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_vibratoButton]
    }
    else if (buttonThatWasClicked == sustainButton)
    {
        //[UserButtonCode_sustainButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Sustain", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_sustainButton]
    }
    else if (buttonThatWasClicked == keyscaleEnvButton)
    {
        //[UserButtonCode_keyscaleEnvButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Keyscale Rate", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_keyscaleEnvButton]
    }
    else if (buttonThatWasClicked == sineImageButton2)
    {
        //[UserButtonCode_sineImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 0);
        //[/UserButtonCode_sineImageButton2]
    }
    else if (buttonThatWasClicked == halfsineImageButton2)
    {
        //[UserButtonCode_halfsineImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 1);
        //[/UserButtonCode_halfsineImageButton2]
    }
    else if (buttonThatWasClicked == abssineImageButton2)
    {
        //[UserButtonCode_abssineImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 2);
        //[/UserButtonCode_abssineImageButton2]
    }
    else if (buttonThatWasClicked == quartersineImageButton2)
    {
        //[UserButtonCode_quartersineImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 3);
        //[/UserButtonCode_quartersineImageButton2]
    }
    else if (buttonThatWasClicked == tremoloButton2)
    {
        //[UserButtonCode_tremoloButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Tremolo", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_tremoloButton2]
    }
    else if (buttonThatWasClicked == vibratoButton2)
    {
        //[UserButtonCode_vibratoButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Vibrato", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_vibratoButton2]
    }
    else if (buttonThatWasClicked == sustainButton2)
    {
        //[UserButtonCode_sustainButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Sustain", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_sustainButton2]
    }
    else if (buttonThatWasClicked == keyscaleEnvButton2)
    {
        //[UserButtonCode_keyscaleEnvButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Keyscale Rate", buttonThatWasClicked->getToggleState() ? 1 : 0);
        //[/UserButtonCode_keyscaleEnvButton2]
    }
    else if (buttonThatWasClicked == alternatingsineImageButton)
    {
        //[UserButtonCode_alternatingsineImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 4);
        //[/UserButtonCode_alternatingsineImageButton]
    }
    else if (buttonThatWasClicked == camelsineImageButton)
    {
        //[UserButtonCode_camelsineImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 5);
        //[/UserButtonCode_camelsineImageButton]
    }
    else if (buttonThatWasClicked == squareImageButton)
    {
        //[UserButtonCode_squareImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 6);
        //[/UserButtonCode_squareImageButton]
    }
    else if (buttonThatWasClicked == logsawImageButton)
    {
        //[UserButtonCode_logsawImageButton] -- add your button handler code here..
		processor->setEnumParameter("Modulator Wave", 7);
        //[/UserButtonCode_logsawImageButton]
    }
    else if (buttonThatWasClicked == alternatingsineImageButton2)
    {
        //[UserButtonCode_alternatingsineImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 4);
        //[/UserButtonCode_alternatingsineImageButton2]
    }
    else if (buttonThatWasClicked == camelsineImageButton2)
    {
        //[UserButtonCode_camelsineImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 5);
        //[/UserButtonCode_camelsineImageButton2]
    }
    else if (buttonThatWasClicked == squareImageButton2)
    {
        //[UserButtonCode_squareImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 6);
        //[/UserButtonCode_squareImageButton2]
    }
    else if (buttonThatWasClicked == logsawImageButton2)
    {
        //[UserButtonCode_logsawImageButton2] -- add your button handler code here..
		processor->setEnumParameter("Carrier Wave", 7);
        //[/UserButtonCode_logsawImageButton2]
    }
    else if (buttonThatWasClicked == recordButton)
    {
        //[UserButtonCode_recordButton] -- add your button handler code here..
        //[/UserButtonCode_recordButton]
    }
    else if (buttonThatWasClicked == exportButton)
    {
        //[UserButtonCode_exportButton] -- add your button handler code here..
		WildcardFileFilter wildcardFilter("*.sbi", String(), "SBI files");
		FileBrowserComponent browser(FileBrowserComponent::saveMode + FileBrowserComponent::canSelectFiles,
			instrumentSaveDirectory,
			&wildcardFilter,
			nullptr);
		FileChooserDialogBox dialogBox("Export to",
			"Specify SBI output file",
			browser,
			true,
			Colours::darkgreen);
		if (dialogBox.show())
		{
			File selectedFile = browser.getSelectedFile(0);
			instrumentSaveDirectory = browser.getRoot();
			processor->saveInstrumentToFile(selectedFile.getFullPathName());
		}
        //[/UserButtonCode_exportButton]
    }
    else if (buttonThatWasClicked == loadButton)
    {
        //[UserButtonCode_loadButton] -- add your button handler code here..
		WildcardFileFilter wildcardFilter("*.sbi", String(), "SBI files");
		FileBrowserComponent browser(FileBrowserComponent::openMode + FileBrowserComponent::canSelectFiles,
			instrumentLoadDirectory,
			&wildcardFilter,
			nullptr);
		FileChooserDialogBox dialogBox("Load",
			"Select SBI instrument file",
			browser,
			false,
			Colours::darkgreen);
		if (dialogBox.show())
		{
			File selectedFile = browser.getSelectedFile(0);
			instrumentLoadDirectory = browser.getRoot();
			processor->loadInstrumentFromFile(selectedFile.getFullPathName());
		}
        //[/UserButtonCode_loadButton]
    }
    else if (buttonThatWasClicked == ToggleButtonOffExample)
    {
        //[UserButtonCode_ToggleButtonOffExample] -- add your button handler code here..
        //[/UserButtonCode_ToggleButtonOffExample]
    }
    else if (buttonThatWasClicked == ToggleButtonOnExample)
    {
        //[UserButtonCode_ToggleButtonOnExample] -- add your button handler code here..
        //[/UserButtonCode_ToggleButtonOnExample]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOffEx1)
    {
        //[UserButtonCode_algoSwitchButtonOffEx1] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOffEx1]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOffEx2)
    {
        //[UserButtonCode_algoSwitchButtonOffEx2] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOffEx2]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOnEx1)
    {
        //[UserButtonCode_algoSwitchButtonOnEx1] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOnEx1]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOnEx2)
    {
        //[UserButtonCode_algoSwitchButtonOnEx2] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOnEx2]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOn2Ex1)
    {
        //[UserButtonCode_algoSwitchButtonOn2Ex1] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOn2Ex1]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOn2Ex2)
    {
        //[UserButtonCode_algoSwitchButtonOn2Ex2] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOn2Ex2]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOn3Ex1)
    {
        //[UserButtonCode_algoSwitchButtonOn3Ex1] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOn3Ex1]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOn3Ex2)
    {
        //[UserButtonCode_algoSwitchButtonOn3Ex2] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOn3Ex2]
    }
    else if (buttonThatWasClicked == TwoOpAMButton)
    {
        //[UserButtonCode_TwoOpAMButton] -- add your button handler code here..
        //[/UserButtonCode_TwoOpAMButton]
    }
    else if (buttonThatWasClicked == TwoOpFMButton)
    {
        //[UserButtonCode_TwoOpFMButton] -- add your button handler code here..
        //[/UserButtonCode_TwoOpFMButton]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOnEx3)
    {
        //[UserButtonCode_algoSwitchButtonOnEx3] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOnEx3]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOffEx3)
    {
        //[UserButtonCode_algoSwitchButtonOffEx3] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOffEx3]
    }
    else if (buttonThatWasClicked == TwoOpAMButton2)
    {
        //[UserButtonCode_TwoOpAMButton2] -- add your button handler code here..
        //[/UserButtonCode_TwoOpAMButton2]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOffEx4)
    {
        //[UserButtonCode_algoSwitchButtonOffEx4] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOffEx4]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOn3Ex3)
    {
        //[UserButtonCode_algoSwitchButtonOn3Ex3] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOn3Ex3]
    }
    else if (buttonThatWasClicked == TwoOpFMButton2)
    {
        //[UserButtonCode_TwoOpFMButton2] -- add your button handler code here..
        //[/UserButtonCode_TwoOpFMButton2]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOffEx5)
    {
        //[UserButtonCode_algoSwitchButtonOffEx5] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOffEx5]
    }
    else if (buttonThatWasClicked == algoSwitchButtonOn3Ex4)
    {
        //[UserButtonCode_algoSwitchButtonOn3Ex4] -- add your button handler code here..
        //[/UserButtonCode_algoSwitchButtonOn3Ex4]
    }
    else if (buttonThatWasClicked == fmButton)
    {
        //[UserButtonCode_fmButton] -- add your button handler code here..
		processor->setEnumParameter("Algorithm", 0);
        //[/UserButtonCode_fmButton]
    }
    else if (buttonThatWasClicked == additiveButton)
    {
        //[UserButtonCode_additiveButton] -- add your button handler code here..
		processor->setEnumParameter("Algorithm", 1);
        //[/UserButtonCode_additiveButton]
    }
    else if (buttonThatWasClicked == bassDrumButton)
    {
        //[UserButtonCode_bassDrumButton] -- add your button handler code here..
		processor->setEnumParameter("Percussion Mode", 1);
        //[/UserButtonCode_bassDrumButton]
    }
    else if (buttonThatWasClicked == snareDrumButton)
    {
        //[UserButtonCode_snareDrumButton] -- add your button handler code here..
		processor->setEnumParameter("Percussion Mode", 2);
        //[/UserButtonCode_snareDrumButton]
    }
    else if (buttonThatWasClicked == disablePercussionButton)
    {
        //[UserButtonCode_disablePercussionButton] -- add your button handler code here..
		processor->setEnumParameter("Percussion Mode", 0);
        //[/UserButtonCode_disablePercussionButton]
    }
    else if (buttonThatWasClicked == tomTomButton)
    {
        //[UserButtonCode_tomTomButton] -- add your button handler code here..
		processor->setEnumParameter("Percussion Mode", 3);
        //[/UserButtonCode_tomTomButton]
    }
    else if (buttonThatWasClicked == cymbalButton)
    {
        //[UserButtonCode_cymbalButton] -- add your button handler code here..
		processor->setEnumParameter("Percussion Mode", 4);
        //[/UserButtonCode_cymbalButton]
    }
    else if (buttonThatWasClicked == hiHatButton)
    {
        //[UserButtonCode_hiHatButton] -- add your button handler code here..
		processor->setEnumParameter("Percussion Mode", 5);
        //[/UserButtonCode_hiHatButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
	//==============================================================================
    // These methods implement the FileDragAndDropTarget interface, and allow our component
    // to accept drag-and-drop of files..

    bool PluginGui::isInterestedInFileDrag (const StringArray& files)
    {
        return 1 == files.size() && (
			files[0].toLowerCase().endsWith(".sbi")
			|| files[0].toLowerCase().endsWith(".sb2")
			|| files[0].toLowerCase().endsWith(".sb0")
		);
    }

    void PluginGui::fileDragEnter (const StringArray& files, int x, int y)
    {
    }

    void PluginGui::fileDragMove (const StringArray& files, int x, int y)
    {
    }

    void PluginGui::fileDragExit (const StringArray& files)
    {
    }

    void PluginGui::filesDropped (const StringArray& files, int x, int y)
    {
		if (isInterestedInFileDrag(files)) {
			processor->loadInstrumentFromFile(files[0]);
		}
    }

	void PluginGui::timerCallback()
	{
		for (int i = 0; i < Hiopl::CHANNELS; ++i) {
			channels[i]->setButtonText(processor->getChannelEnvelopeStage(i + 1));
		}
	}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PluginGui" componentName=""
                 parentClasses="public AudioProcessorEditor, public FileDragAndDropTarget, public DragAndDropContainer, public Timer"
                 constructorParams="AdlibBlasterAudioProcessor* ownerFilter" variableInitialisers=" AudioProcessorEditor (ownerFilter)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="860" initialHeight="580">
  <BACKGROUND backgroundColour="ff000000"/>
  <GROUPCOMPONENT name="new group" id="93b9aaeb75040aed" memberName="groupComponent2"
                  virtualName="" explicitFocusOrder="0" pos="440 88 408 344" outlinecol="ff007f00"
                  textcol="ff007f00" title="Carrier" textpos="33"/>
  <GROUPCOMPONENT name="new group" id="52f9803abb342980" memberName="groupComponent4"
                  virtualName="" explicitFocusOrder="0" pos="16 8 832 64" outlinecol="ff007f00"
                  textcol="ff007f00" title="Channels" textpos="33"/>
  <GROUPCOMPONENT name="new group" id="6cc98dbf76b41b7b" memberName="groupComponent11"
                  virtualName="" explicitFocusOrder="0" pos="496 440 192 120" outlinecol="ff007f00"
                  textcol="ff007f00" title="Percussion" textpos="33"/>
  <GROUPCOMPONENT name="new group" id="e8d476c7f6d163e9" memberName="groupComponent10"
                  virtualName="" explicitFocusOrder="0" pos="280 440 200 120" outlinecol="ff007f00"
                  textcol="ff007f00" title="Algorithm" textpos="33"/>
  <GROUPCOMPONENT name="new group" id="791b6f04e9fd52bb" memberName="groupComponent9"
                  virtualName="" explicitFocusOrder="0" pos="704 440 144 120" outlinecol="ff007f00"
                  textcol="ff007f00" title="File" textpos="33"/>
  <GROUPCOMPONENT name="new group" id="d2c7c07bf2d78c30" memberName="groupComponent"
                  virtualName="" explicitFocusOrder="0" pos="16 88 408 344" outlinecol="ff007f00"
                  textcol="ff007f00" title="Modulator" textpos="33"/>
  <COMBOBOX name="frequency combo box" id="4e65faf3d9442460" memberName="frequencyComboBox"
            virtualName="" explicitFocusOrder="0" pos="200 168 64 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="frequency label" id="7414532477c7f744" memberName="frequencyLabel"
         virtualName="" explicitFocusOrder="0" pos="40 168 152 24" tooltip="Multiplier applied to base note frequency"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Frequency Multiplier"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <SLIDER name="a slider" id="1b9be27726a5b3ae" memberName="aSlider" virtualName=""
          explicitFocusOrder="0" pos="40 208 30 88" tooltip="Envelope attack rate"
          thumbcol="ff007f00" trackcol="7f007f00" textboxtext="ff007f00"
          textboxbkgd="ff000000" textboxhighlight="ff00af00" min="0" max="15"
          int="1" style="LinearVertical" textBoxPos="TextBoxBelow" textBoxEditable="0"
          textBoxWidth="30" textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="a label" id="9dd0b13f00b4de42" memberName="aLabel" virtualName=""
         explicitFocusOrder="0" pos="40 304 30 24" tooltip="Attack rate"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="A"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="d slider" id="d4cc8ddf2fc9cf2b" memberName="dSlider" virtualName=""
          explicitFocusOrder="0" pos="88 208 30 88" tooltip="Envelope decay rate"
          thumbcol="ff007f00" trackcol="7f007f00" textboxtext="ff007f00"
          textboxbkgd="ff000000" textboxhighlight="ff00af00" min="0" max="15"
          int="1" style="LinearVertical" textBoxPos="TextBoxBelow" textBoxEditable="0"
          textBoxWidth="30" textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="d label" id="a7f17b098b85f10b" memberName="dLabel" virtualName=""
         explicitFocusOrder="0" pos="88 304 30 24" tooltip="Decay rate"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="D"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="s slider" id="9bcadfc61e498bce" memberName="sSlider" virtualName=""
          explicitFocusOrder="0" pos="136 208 30 88" tooltip="Envelope sustain level"
          thumbcol="ff007f00" trackcol="7f007f00" textboxtext="ff007f00"
          textboxbkgd="ff000000" textboxhighlight="ff00af00" min="0" max="15"
          int="1" style="LinearVertical" textBoxPos="TextBoxBelow" textBoxEditable="0"
          textBoxWidth="30" textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="d label" id="6467455c7573fefa" memberName="dLabel2" virtualName=""
         explicitFocusOrder="0" pos="136 304 30 24" tooltip="Sustain level"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="S"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="r slider" id="5616976a8c5a3f5f" memberName="rSlider" virtualName=""
          explicitFocusOrder="0" pos="184 208 30 88" tooltip="Envelope release rate"
          thumbcol="ff007f00" trackcol="7f007f00" textboxtext="ff007f00"
          textboxbkgd="ff000000" textboxhighlight="ff00af00" min="0" max="15"
          int="1" style="LinearVertical" textBoxPos="TextBoxBelow" textBoxEditable="0"
          textBoxWidth="30" textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="r label" id="ef30d2907e867666" memberName="rLabel" virtualName=""
         explicitFocusOrder="0" pos="184 304 30 24" tooltip="Release rate"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="R"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="attenuation slider" id="dfb943cd83b3977f" memberName="attenuationSlider"
          virtualName="" explicitFocusOrder="0" pos="328 184 56 142" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="-47.25" max="0" int="0.75" style="LinearVertical"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="64"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="attenuation label" id="643f88854c82ca3e" memberName="attenuationLabel"
         virtualName="" explicitFocusOrder="0" pos="304 160 112 24" tooltip="Final output level adjustment"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Attenuation"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <LABEL name="db label" id="666be8c96c85c9f1" memberName="dbLabel" virtualName=""
         explicitFocusOrder="0" pos="384 304 32 24" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="sine image button" id="5e72e0ec4fc09c1a" memberName="sineImageButton"
               virtualName="" explicitFocusOrder="0" pos="88 113 34 30" buttonText="Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="full_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="half sine image button" id="bf9e0504c5e9e5d5" memberName="halfsineImageButton"
               virtualName="" explicitFocusOrder="0" pos="128 113 34 30" buttonText="Half Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="half_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="abs sine image button" id="1b0b532ac934edae" memberName="abssineImageButton"
               virtualName="" explicitFocusOrder="0" pos="168 113 34 30" buttonText="Abs Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="abs_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="quarter sine image button" id="47d1bd1fd4ae011d" memberName="quartersineImageButton"
               virtualName="" explicitFocusOrder="0" pos="208 113 34 30" buttonText="Quarter Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="quarter_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <LABEL name="wave label" id="d35c942584ea52a6" memberName="waveLabel"
         virtualName="" explicitFocusOrder="0" pos="32 115 48 24" textCol="ff007f00"
         edTextCol="ff000000" edBkgCol="0" labelText="Wave" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <TOGGLEBUTTON name="tremolo button" id="1e6ab9b2f1fee312" memberName="tremoloButton"
                virtualName="" explicitFocusOrder="0" pos="120 352 80 24" tooltip="Modulate amplitude at 3.7 Hz"
                txtcol="ff007f00" buttonText="Tremolo" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="vibrato button" id="a989eb6692e3dbd8" memberName="vibratoButton"
                virtualName="" explicitFocusOrder="0" pos="32 352 72 24" tooltip="Modulate frequency at 6.1 Hz"
                txtcol="ff007f00" buttonText="Vibrato" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="sustain button" id="e0ae2bc46ec1861c" memberName="sustainButton"
                virtualName="" explicitFocusOrder="0" pos="32 384 70 32" tooltip="Enable or disable sustain when note is held"
                txtcol="ff007f00" buttonText="Sustain" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="keyscale env button" id="a3f62a22526b4b49" memberName="keyscaleEnvButton"
                virtualName="" explicitFocusOrder="0" pos="120 376 101 48" tooltip="Speed up envelope rate with note frequency"
                txtcol="ff007f00" buttonText="Keyscale Env. Rate" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="db label" id="b9b3cedf2b541262" memberName="dbLabel2" virtualName=""
         explicitFocusOrder="0" pos="792 712 72 16" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="dB/8ve&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <COMBOBOX name="frequency combo box" id="30b8c81b6bd2a17" memberName="frequencyComboBox2"
            virtualName="" explicitFocusOrder="0" pos="624 168 66 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="frequency label" id="65d58d2259c13bf1" memberName="frequencyLabel3"
         virtualName="" explicitFocusOrder="0" pos="464 168 152 24" tooltip="Multiplier applied to base note frequency"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Frequency Multiplier"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <SLIDER name="a slider" id="d6d2f4556ea9394" memberName="aSlider2" virtualName=""
          explicitFocusOrder="0" pos="464 208 30 88" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="0" max="15" int="1" style="LinearVertical"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="a label" id="9ec6412cc79720bc" memberName="aLabel2" virtualName=""
         explicitFocusOrder="0" pos="464 304 30 24" tooltip="Attack rate"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="A"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="d slider" id="4a1f1b6038500f67" memberName="dSlider2" virtualName=""
          explicitFocusOrder="0" pos="512 208 30 88" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="0" max="15" int="1" style="LinearVertical"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="d label" id="10231adaf9e23e14" memberName="dLabel3" virtualName=""
         explicitFocusOrder="0" pos="512 304 30 24" tooltip="Decay rate"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="D"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="s slider" id="2fc057248a815958" memberName="sSlider2" virtualName=""
          explicitFocusOrder="0" pos="560 208 30 88" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="0" max="15" int="1" style="LinearVertical"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="d label" id="5b881f2381defac" memberName="dLabel4" virtualName=""
         explicitFocusOrder="0" pos="560 304 30 24" tooltip="Sustain level"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="S"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="r slider" id="5474ad005fb58e97" memberName="rSlider2" virtualName=""
          explicitFocusOrder="0" pos="608 208 30 88" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="0" max="15" int="1" style="LinearVertical"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="r label" id="ca2834438bee82a9" memberName="rLabel2" virtualName=""
         explicitFocusOrder="0" pos="608 304 30 24" tooltip="Release rate"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="R"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <SLIDER name="attenuation slider" id="edb48da87d7535dd" memberName="attenuationSlider2"
          virtualName="" explicitFocusOrder="0" pos="752 184 56 142" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="-47.25" max="0" int="0.75" style="LinearVertical"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="64"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="attenuation label" id="958314f88253f461" memberName="attenuationLabel2"
         virtualName="" explicitFocusOrder="0" pos="728 160 112 24" tooltip="Final output level adjustment"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Attenuation"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <LABEL name="db label" id="7efc6195ef5e25d1" memberName="dbLabel3" virtualName=""
         explicitFocusOrder="0" pos="800 304 40 24" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="sine image button" id="27e01d31ba835965" memberName="sineImageButton2"
               virtualName="" explicitFocusOrder="0" pos="512 113 34 30" buttonText="Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="full_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="half sine image button" id="6e9afdb08dd4edac" memberName="halfsineImageButton2"
               virtualName="" explicitFocusOrder="0" pos="552 113 34 30" buttonText="Half Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="half_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="abs sine image button" id="361941cfa04130c1" memberName="abssineImageButton2"
               virtualName="" explicitFocusOrder="0" pos="592 113 34 30" buttonText="Abs Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="abs_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="quarter sine image button" id="3fa62f49fdd1a41f" memberName="quartersineImageButton2"
               virtualName="" explicitFocusOrder="0" pos="632 113 34 30" buttonText="Quarter Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="quarter_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <LABEL name="wave label" id="c810628f3c772781" memberName="waveLabel2"
         virtualName="" explicitFocusOrder="0" pos="456 115 48 24" textCol="ff007f00"
         edTextCol="ff000000" edBkgCol="0" labelText="Wave" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <TOGGLEBUTTON name="tremolo button" id="a517934e39704073" memberName="tremoloButton2"
                virtualName="" explicitFocusOrder="0" pos="544 352 80 24" tooltip="Modulate amplitude at 3.7 Hz"
                txtcol="ff007f00" buttonText="Tremolo" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="vibrato button" id="736b965a99641077" memberName="vibratoButton2"
                virtualName="" explicitFocusOrder="0" pos="456 352 72 24" tooltip="Modulate frequency at 6.1 Hz"
                txtcol="ff007f00" buttonText="Vibrato" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="sustain button" id="a3832cb840cae1f2" memberName="sustainButton2"
                virtualName="" explicitFocusOrder="0" pos="456 384 70 24" tooltip="Enable or disable sustain when note is held"
                txtcol="ff007f00" buttonText="Sustain" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="keyscale env button" id="4cd968dae86d143c" memberName="keyscaleEnvButton2"
                virtualName="" explicitFocusOrder="0" pos="544 376 102 48" tooltip="Speed up envelope rate with note frequency"
                txtcol="ff007f00" buttonText="Keyscale Env. Rate" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="frequency label" id="a1e2dd50c2835d73" memberName="frequencyLabel4"
         virtualName="" explicitFocusOrder="0" pos="656 376 88 48" tooltip="Attenuate amplitude with note frequency in dB per octave"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Keyscale Attenuation"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <GROUPCOMPONENT name="new group" id="7392f7d1c8cf6e74" memberName="groupComponent3"
                  virtualName="" explicitFocusOrder="0" pos="16 440 248 120" outlinecol="ff007f00"
                  textcol="ff007f00" title="Effect depth" textpos="33"/>
  <SLIDER name="tremolo slider" id="ab64abee7ac8874b" memberName="tremoloSlider"
          virtualName="" explicitFocusOrder="0" pos="112 472 80 24" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="1" max="4.7999999999999998224"
          int="3.7999999999999998224" style="LinearHorizontal" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="32" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <LABEL name="frequency label" id="134ce8f87da62b88" memberName="frequencyLabel5"
         virtualName="" explicitFocusOrder="0" pos="32 472 80 24" tooltip="OPL global tremolo depth"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Tremolo&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <LABEL name="db label" id="720df8e7c502dd91" memberName="dbLabel5" virtualName=""
         explicitFocusOrder="0" pos="200 464 32 40" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <SLIDER name="vibrato slider" id="b45a1f20f22cf5ca" memberName="vibratoSlider"
          virtualName="" explicitFocusOrder="0" pos="112 512 80 24" thumbcol="ff007f00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="7" max="14" int="7" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="32"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="frequency label" id="1412b9d14e37bcbe" memberName="frequencyLabel6"
         virtualName="" explicitFocusOrder="0" pos="32 512 80 24" tooltip="OPL global vibrato depth"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Vibrato"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <LABEL name="db label" id="e13e0aff8b974a36" memberName="dbLabel6" virtualName=""
         explicitFocusOrder="0" pos="200 504 48 40" tooltip="A unit of pitch; 100 cents per semitone"
         textCol="ff007f00" outlineCol="0" edTextCol="ff000000" edBkgCol="0"
         labelText="cents&#10;" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="33"/>
  <SLIDER name="feedback slider" id="f9d22e12f5e417e4" memberName="feedbackSlider"
          virtualName="" explicitFocusOrder="0" pos="248 237 30 59" thumbcol="ff00af00"
          trackcol="7f007f00" rotarysliderfill="ff00af00" rotaryslideroutline="ff007f00"
          textboxtext="ff007f00" textboxbkgd="ff000000" textboxhighlight="ff00af00"
          min="0" max="7" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxBelow" textBoxEditable="0" textBoxWidth="30"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="frequency label" id="880eaf14af62578a" memberName="frequencyLabel7"
         virtualName="" explicitFocusOrder="0" pos="224 304 80 24" tooltip="Extent to which modulator output is fed back into itself"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Feedback"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <COMBOBOX name="velocity combo box" id="cbe10e5236447f15" memberName="velocityComboBox"
            virtualName="" explicitFocusOrder="0" pos="328 352 76 24" editable="0"
            layout="33" items="Off&#10;Light&#10;Heavy" textWhenNonSelected=""
            textWhenNoItems="(no choices)"/>
  <COMBOBOX name="velocity combo box" id="f5c4883d9feaa700" memberName="velocityComboBox2"
            virtualName="" explicitFocusOrder="0" pos="760 352 72 24" editable="0"
            layout="33" items="Off&#10;Light&#10;Heavy" textWhenNonSelected=""
            textWhenNoItems="(no choices)"/>
  <LABEL name="attenuation label" id="d9297cdef25630de" memberName="attenuationLabel4"
         virtualName="" explicitFocusOrder="0" pos="760 376 80 48" tooltip="Set or disable velocity senstivity"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Velocity Sensitivity"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="alternating sine image button" id="2a054359a782e92d" memberName="alternatingsineImageButton"
               virtualName="" explicitFocusOrder="0" pos="288 113 34 30" buttonText="Alternating Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="alternating_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="camel sine image button" id="d6f66822f7f64480" memberName="camelsineImageButton"
               virtualName="" explicitFocusOrder="0" pos="248 113 34 30" buttonText="Camel Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="camel_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="square image button" id="85e53fb506289115" memberName="squareImageButton"
               virtualName="" explicitFocusOrder="0" pos="328 113 34 30" buttonText="Square"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="square_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="logsaw image button" id="fca4c858138cdd7b" memberName="logsawImageButton"
               virtualName="" explicitFocusOrder="0" pos="368 113 34 30" buttonText="Logarithmic Sawtooth"
               connectedEdges="0" needsCallback="1" radioGroupId="1" keepProportions="1"
               resourceNormal="logarithmic_saw_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="alternating sine image button" id="32c5f60cc145d464" memberName="alternatingsineImageButton2"
               virtualName="" explicitFocusOrder="0" pos="714 114 34 30" buttonText="Alternating Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="alternating_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="camel sine image button" id="215395763c6a03f2" memberName="camelsineImageButton2"
               virtualName="" explicitFocusOrder="0" pos="674 114 34 30" buttonText="Camel Sine"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="camel_sine_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="square image button" id="d85202a2e5f8b158" memberName="squareImageButton2"
               virtualName="" explicitFocusOrder="0" pos="754 114 34 30" buttonText="Square"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="square_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="logsaw image button" id="d713984cff8b67b5" memberName="logsawImageButton2"
               virtualName="" explicitFocusOrder="0" pos="794 114 34 30" buttonText="Logarithmic Sawtooth"
               connectedEdges="0" needsCallback="1" radioGroupId="2" keepProportions="1"
               resourceNormal="logarithmic_saw_png" opacityNormal="0.5" colourNormal="0"
               resourceOver="" opacityOver="0.5" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <LABEL name="db label" id="1f10b7e3cf477c89" memberName="dbLabel4" virtualName=""
         explicitFocusOrder="0" pos="792 688 72 16" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="dB/8ve&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <COMBOBOX name="keyscale combo box" id="9b766b7b6a67cbf4" memberName="keyscaleAttenuationComboBox2"
            virtualName="" explicitFocusOrder="0" pos="664 352 76 24" editable="0"
            layout="33" items="-0.0&#10;-3.0&#10;-1.5&#10;-6.0" textWhenNonSelected=""
            textWhenNoItems="(no choices)"/>
  <COMBOBOX name="keyscale combo box" id="7d8e1de0e1579999" memberName="keyscaleAttenuationComboBox"
            virtualName="" explicitFocusOrder="0" pos="232 352 76 24" editable="0"
            layout="33" items="-0.0&#10;-3.0&#10;-1.5&#10;-6.0" textWhenNonSelected=""
            textWhenNoItems="(no choices)"/>
  <GROUPCOMPONENT name="new group" id="7abc643f4d6a2dbf" memberName="groupComponent5"
                  virtualName="" explicitFocusOrder="0" pos="24 712 408 64" outlinecol="ff007f00"
                  textcol="ff007f00" title="Emulator (currently locked)" textpos="33"/>
  <SLIDER name="emulator slider" id="88ec3755c4760ed9" memberName="emulatorSlider"
          virtualName="" explicitFocusOrder="0" pos="208 736 40 24" thumbcol="ff00af00"
          trackcol="7f007f00" textboxtext="ff007f00" textboxbkgd="ff000000"
          textboxhighlight="ff00af00" min="0" max="1" int="1" style="LinearHorizontal"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="44"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <LABEL name="emulator label" id="22c2c30d0f337081" memberName="emulatorLabel"
         virtualName="" explicitFocusOrder="0" pos="120 736 72 24" tooltip="Use the OPL emulator from the DOSBox project"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="DOSBox"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="34"/>
  <LABEL name="emulator label" id="4f8869b5724c0195" memberName="emulatorLabel2"
         virtualName="" explicitFocusOrder="0" pos="256 736 72 24" tooltip="Use the OPL emulator from the ZDoom project"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="ZDoom"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <TOGGLEBUTTON name="record button" id="880010ee79039cbe" memberName="recordButton"
                virtualName="" explicitFocusOrder="0" pos="32 680 224 24" tooltip="Start recording all register writes to a DRO file - an OPL recording file format defined by DOSBox"
                txtcol="ff007f00" buttonText="Record to DRO (not working yet)"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TEXTBUTTON name="export button" id="88c84ed1e2b284d3" memberName="exportButton"
              virtualName="" explicitFocusOrder="0" pos="728 512 96 24" bgColOff="ff007f00"
              bgColOn="ff00ff00" buttonText="Export .SBI" connectedEdges="3"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="load button" id="a42176161523f448" memberName="loadButton"
              virtualName="" explicitFocusOrder="0" pos="728 472 96 24" bgColOff="ff007f00"
              bgColOn="ff00ff00" buttonText="Load .SBI" connectedEdges="3"
              needsCallback="1" radioGroupId="0"/>
  <LABEL name="version label" id="cd68ca110847cc18" memberName="versionLabel"
         virtualName="" explicitFocusOrder="0" pos="648 560 198 16" textCol="ff007f00"
         edTextCol="ff000000" edBkgCol="0" labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="12" kerning="0" bold="0" italic="0" justification="34"/>
  <IMAGEBUTTON name="Toggle Button Off Example" id="672bea5ea2e1fabd" memberName="ToggleButtonOffExample"
               virtualName="" explicitFocusOrder="0" pos="1032 584 12 12" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="toggle_off_sq_png" opacityNormal="1" colourNormal="0"
               resourceOver="" opacityOver="1" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Toggle Button On Example" id="1a4b1e2ee10b30aa" memberName="ToggleButtonOnExample"
               virtualName="" explicitFocusOrder="0" pos="1064 584 12 12" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="toggle_on_sq_png" opacityNormal="1" colourNormal="0"
               resourceOver="" opacityOver="1" colourOver="0" resourceDown=""
               opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="d00839172c49b458" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="1000 608 104 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Toggle buttons"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <LABEL name="new label" id="75faa73445635a7f" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="872 608 104 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Line borders" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="36"/>
  <IMAGEBUTTON name="Line Border 1C" id="d189b7564dfbe6f4" memberName="LineBorderButton1C"
               virtualName="" explicitFocusOrder="0" pos="20 336 400 6" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_horiz_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_horiz_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_horiz_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Line Border 1A" id="e2102e76055ea2d2" memberName="LineBorderButton1A"
               virtualName="" explicitFocusOrder="0" pos="20 152 400 6" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_horiz_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_horiz_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_horiz_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Line Border 1B" id="c602d4512bd5e4ad" memberName="LineBorderButton1B"
               virtualName="" explicitFocusOrder="0" pos="296 156 6 182" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_vert_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_vert_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_vert_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <LABEL name="new label" id="96790ccaf0f7ecec" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="776 736 104 56" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Temporarily removed labels to avoid making wider boxes."
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="Line Border 1C" id="fb69fc397f48c0b2" memberName="LineBorderButton1C2"
               virtualName="" explicitFocusOrder="0" pos="444 336 400 6" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_horiz_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_horiz_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_horiz_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Line Border 1A" id="2096630c63845b7d" memberName="LineBorderButton1A2"
               virtualName="" explicitFocusOrder="0" pos="444 152 400 6" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_horiz_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_horiz_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_horiz_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Line Border 1B" id="84b521f64fc5ec24" memberName="LineBorderButton1B2"
               virtualName="" explicitFocusOrder="0" pos="720 156 6 182" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_vert_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_vert_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_vert_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Line Border 1C" id="d45929173c0e1a86" memberName="LineBorderButton1C3"
               virtualName="" explicitFocusOrder="0" pos="892 584 20 6" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_horiz_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_horiz_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_horiz_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Line Border 1B" id="1755b1c2b6e4ae68" memberName="LineBorderButton1B3"
               virtualName="" explicitFocusOrder="0" pos="936 576 6 20" buttonText="new button"
               connectedEdges="0" needsCallback="0" radioGroupId="0" keepProportions="0"
               resourceNormal="line_border_vert_png" opacityNormal="0.60000002384185791016"
               colourNormal="0" resourceOver="line_border_vert_png" opacityOver="0.60000002384185791016"
               colourOver="0" resourceDown="line_border_vert_png" opacityDown="0.60000002384185791016"
               colourDown="0"/>
  <IMAGEBUTTON name="Algorithm Switch Off AM" id="c840af0d765d6eb3" memberName="algoSwitchButtonOffEx1"
               virtualName="" explicitFocusOrder="0" pos="952 701 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_off_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_off_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_off_png" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Algorithm Switch Off FM" id="aa0f44b1ed8dad85" memberName="algoSwitchButtonOffEx2"
               virtualName="" explicitFocusOrder="0" pos="952 727 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_off_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_off_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_off_png" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Algorithm Switch On AM" id="e876ffbe79764275" memberName="algoSwitchButtonOnEx1"
               virtualName="" explicitFocusOrder="0" pos="1040 701 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on_png" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Algorithm Switch On FM" id="b215e3921423b6e4" memberName="algoSwitchButtonOnEx2"
               virtualName="" explicitFocusOrder="0" pos="1040 727 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="8e80bd8b126eeb36" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="970 701 32 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="1f98e50cc47ec1a6" memberName="label5" virtualName=""
         explicitFocusOrder="0" pos="971 727 32 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="8cfbc479cf413916" memberName="label6" virtualName=""
         explicitFocusOrder="0" pos="1057 701 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="e231c8016dbdd4b" memberName="label7" virtualName=""
         explicitFocusOrder="0" pos="1058 727 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="58e93cab537ef6c0" memberName="label8" virtualName=""
         explicitFocusOrder="0" pos="944 816 320 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Example AM/FM switches"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="Algorithm Switch On2 AM" id="afdb65f653352953" memberName="algoSwitchButtonOn2Ex1"
               virtualName="" explicitFocusOrder="0" pos="1128 700 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on2_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on2_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on2_png" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Algorithm Switch On2 FM" id="92f052947cb1a55" memberName="algoSwitchButtonOn2Ex2"
               virtualName="" explicitFocusOrder="0" pos="1128 727 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on2_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on2_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on2_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="247e4f52e4cfd135" memberName="label9" virtualName=""
         explicitFocusOrder="0" pos="1145 700 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="aec882448be58719" memberName="label10" virtualName=""
         explicitFocusOrder="0" pos="1146 727 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Algorithm Switch On3 AM" id="9c9fbd61392d18d7" memberName="algoSwitchButtonOn3Ex1"
               virtualName="" explicitFocusOrder="0" pos="1216 700 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on3_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on3_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on3_png" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Algorithm Switch On3 FM" id="7c15f9c7da34e18d" memberName="algoSwitchButtonOn3Ex2"
               virtualName="" explicitFocusOrder="0" pos="1216 727 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on3_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on3_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on3_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="336158e70e8469ef" memberName="label11" virtualName=""
         explicitFocusOrder="0" pos="1233 700 31 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="281e5575b4c17d57" memberName="label12" virtualName=""
         explicitFocusOrder="0" pos="1234 727 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Two OP AM Button" id="bc89b5f960a478ae" memberName="TwoOpAMButton"
               virtualName="" explicitFocusOrder="0" pos="1173 484 60 56" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="0"
               resourceNormal="twoopAm_png" opacityNormal="1" colourNormal="0"
               resourceOver="twoopAm_png" opacityOver="1" colourOver="0" resourceDown="twoopAm_png"
               opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Two OP FM Button" id="5dbdd24f69156c98" memberName="TwoOpFMButton"
               virtualName="" explicitFocusOrder="0" pos="1156 568 80 26" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="twoopFm_png" opacityNormal="1" colourNormal="0"
               resourceOver="twoopFm_png" opacityOver="1" colourOver="0" resourceDown="twoopFm_png"
               opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="54bf3742f6cf39a7" memberName="label13" virtualName=""
         explicitFocusOrder="0" pos="1179 489 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="M" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="a73d54281a9f1e4b" memberName="label14" virtualName=""
         explicitFocusOrder="0" pos="1179 518 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="C" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="c7714e4c9c108a80" memberName="label15" virtualName=""
         explicitFocusOrder="0" pos="1166 572 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="M" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="6fad65a5c825f676" memberName="label16" virtualName=""
         explicitFocusOrder="0" pos="1195 572 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="C" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="d9d895b8fa9bea7f" memberName="label17" virtualName=""
         explicitFocusOrder="0" pos="1128 608 136 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Example Algorithms"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <GROUPCOMPONENT name="new group" id="d489f4c4cbfaf3a" memberName="groupComponent6"
                  virtualName="" explicitFocusOrder="0" pos="933 56 168 95" outlinecol="ff008000"
                  title=""/>
  <IMAGEBUTTON name="Algorithm Switch On AM" id="3b9987473ffb3a54" memberName="algoSwitchButtonOnEx3"
               virtualName="" explicitFocusOrder="0" pos="949 82 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="ddfd6855a5c3769a" memberName="label18" virtualName=""
         explicitFocusOrder="0" pos="966 82 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Algorithm Switch Off FM" id="3bbe951e7d48f558" memberName="algoSwitchButtonOffEx3"
               virtualName="" explicitFocusOrder="0" pos="949 108 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_off_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_off_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_off_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="ec3250e2f0f72c27" memberName="label19" virtualName=""
         explicitFocusOrder="0" pos="968 108 32 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Two OP AM Button" id="6dd4e125e7f2454f" memberName="TwoOpAMButton2"
               virtualName="" explicitFocusOrder="0" pos="1029 77 60 56" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="0"
               resourceNormal="twoopAm_png" opacityNormal="1" colourNormal="0"
               resourceOver="twoopAm_png" opacityOver="1" colourOver="0" resourceDown="twoopAm_png"
               opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="6c2ac34805e7a509" memberName="label20" virtualName=""
         explicitFocusOrder="0" pos="1035 82 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="M" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="68b10a34cd551295" memberName="label21" virtualName=""
         explicitFocusOrder="0" pos="1035 111 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="C" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="5c48c62a06b13a38" memberName="label22" virtualName=""
         explicitFocusOrder="0" pos="952 160 328 40" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Example Algo Sections w/ Diagram"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="Algorithm Switch Off AM" id="1ca80deedba9b959" memberName="algoSwitchButtonOffEx4"
               virtualName="" explicitFocusOrder="0" pos="1125 82 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_off_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_off_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_off_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="c7f07212d02cdf5b" memberName="label23" virtualName=""
         explicitFocusOrder="0" pos="1143 82 32 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Algorithm Switch On3 FM" id="840e067b2b3498f8" memberName="algoSwitchButtonOn3Ex3"
               virtualName="" explicitFocusOrder="0" pos="1125 109 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on3_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on3_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on3_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="4f6fc36b09626a98" memberName="label24" virtualName=""
         explicitFocusOrder="0" pos="1143 109 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Two OP FM Button" id="6de80642ad3057e6" memberName="TwoOpFMButton2"
               virtualName="" explicitFocusOrder="0" pos="1196 94 80 26" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="twoopFm_png" opacityNormal="1" colourNormal="0"
               resourceOver="twoopFm_png" opacityOver="1" colourOver="0" resourceDown="twoopFm_png"
               opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="5d9df21ba856feea" memberName="label25" virtualName=""
         explicitFocusOrder="0" pos="1206 98 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="M" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="bc2f20892df7121b" memberName="label26" virtualName=""
         explicitFocusOrder="0" pos="1235 98 24 24" edTextCol="ff000000"
         edBkgCol="0" labelText="C" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         kerning="0" bold="0" italic="0" justification="36"/>
  <GROUPCOMPONENT name="new group" id="35d4aeb27da92db" memberName="groupComponent7"
                  virtualName="" explicitFocusOrder="0" pos="1112 56 168 95" outlinecol="ff008000"
                  title=""/>
  <IMAGEBUTTON name="Algorithm Switch Off AM" id="186e15fd17374b39" memberName="algoSwitchButtonOffEx5"
               virtualName="" explicitFocusOrder="0" pos="1037 250 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_off_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_off_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_off_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="18a95b2639e6ca06" memberName="label27" virtualName=""
         explicitFocusOrder="0" pos="1055 250 32 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="AM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <IMAGEBUTTON name="Algorithm Switch On3 FM" id="a280aa6d341570b7" memberName="algoSwitchButtonOn3Ex4"
               virtualName="" explicitFocusOrder="0" pos="1103 250 64 24" buttonText="new button"
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="algo_switch_on3_png" opacityNormal="1" colourNormal="0"
               resourceOver="algo_switch_on3_png" opacityOver="1" colourOver="0"
               resourceDown="algo_switch_on3_png" opacityDown="1" colourDown="0"/>
  <LABEL name="new label" id="7f064fc52edca9aa" memberName="label28" virtualName=""
         explicitFocusOrder="0" pos="1121 250 32 24" textCol="ff000000"
         edTextCol="ff000000" edBkgCol="0" labelText="FM" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" kerning="0" bold="0" italic="0" justification="33"/>
  <GROUPCOMPONENT name="new group" id="4c77a30ef34ca25d" memberName="groupComponent8"
                  virtualName="" explicitFocusOrder="0" pos="1008 208 168 95" outlinecol="ff008000"
                  title=""/>
  <LABEL name="frequency label" id="70b9f51419600f29" memberName="frequencyLabel9"
         virtualName="" explicitFocusOrder="0" pos="1067 216 72 24" tooltip="In additive mode, carrier and modulator output are simply summed rather than modulated"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Algorithm"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <LABEL name="new label" id="31a16fa32fc39ae9" memberName="label29" virtualName=""
         explicitFocusOrder="0" pos="944 304 328 40" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Example Algo Section w/o Diagram"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <LABEL name="new label" id="2470d0303393253b" memberName="label30" virtualName=""
         explicitFocusOrder="0" pos="961 768 319 24" textCol="ff008000"
         edTextCol="ff000000" edBkgCol="0" labelText="Off             On (Bright)          On (Dark)       On (Solid)"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <LABEL name="frequency label" id="9d58547998708b6b" memberName="frequencyLabel10"
         virtualName="" explicitFocusOrder="0" pos="224 376 88 48" tooltip="Attenuate amplitude with note frequency in dB per octave"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Keyscale Attenuation"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <LABEL name="attenuation label" id="63aa860d1d8ae341" memberName="attenuationLabel5"
         virtualName="" explicitFocusOrder="0" pos="328 376 80 48" tooltip="Set or disable velocity senstivity"
         textCol="ff007f00" edTextCol="ff000000" edBkgCol="0" labelText="Velocity Sensitivity"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
  <IMAGEBUTTON name="fm button" id="19b03dffaa7fc94" memberName="fmButton" virtualName=""
               explicitFocusOrder="0" pos="304 472 56 56" tooltip="FM: carrier frequency is modulated by the modulator"
               buttonText="FM" connectedEdges="0" needsCallback="1" radioGroupId="3"
               keepProportions="1" resourceNormal="twoopAm_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="Additive mode button" id="d3cf9bfa8c4d4885" memberName="additiveButton"
               virtualName="" explicitFocusOrder="0" pos="392 464 72 72" tooltip="Additive: output the sum of the modulator and carrier"
               buttonText="Additive Mode" connectedEdges="0" needsCallback="1"
               radioGroupId="3" keepProportions="1" resourceNormal="twoopFm_png"
               opacityNormal="0.5" colourNormal="0" resourceOver="" opacityOver="0.5"
               colourOver="0" resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="bass drum button" id="2c8905c4541593a7" memberName="bassDrumButton"
               virtualName="" explicitFocusOrder="0" pos="576 464 30 30" tooltip="Bass drum"
               buttonText="bass drum" connectedEdges="0" needsCallback="1" radioGroupId="4"
               keepProportions="1" resourceNormal="bassdrum_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="snare drum button" id="bcbb7e2c191a56e8" memberName="snareDrumButton"
               virtualName="" explicitFocusOrder="0" pos="632 464 30 30" tooltip="Snare"
               buttonText="snare" connectedEdges="0" needsCallback="1" radioGroupId="4"
               keepProportions="1" resourceNormal="snare_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="percussion disabled button" id="fcecada70009babc" memberName="disablePercussionButton"
               virtualName="" explicitFocusOrder="0" pos="520 464 30 30" tooltip="Disable percussion"
               buttonText="disabled" connectedEdges="0" needsCallback="1" radioGroupId="4"
               keepProportions="1" resourceNormal="disabled_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="tom tom button" id="7ab8c7e4677552" memberName="tomTomButton"
               virtualName="" explicitFocusOrder="0" pos="520 512 30 30" tooltip="Tom-tom"
               buttonText="tom tom" connectedEdges="0" needsCallback="1" radioGroupId="4"
               keepProportions="1" resourceNormal="tom_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="cymbalButton" id="a4334a83ef3cbbde" memberName="cymbalButton"
               virtualName="" explicitFocusOrder="0" pos="576 512 30 30" tooltip="Cymbal"
               buttonText="snare" connectedEdges="0" needsCallback="1" radioGroupId="4"
               keepProportions="1" resourceNormal="cymbal_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <IMAGEBUTTON name="hi hat button" id="49d70294c1d75708" memberName="hiHatButton"
               virtualName="" explicitFocusOrder="0" pos="632 512 30 30" tooltip="Hi-hat"
               buttonText="hi-hat" connectedEdges="0" needsCallback="1" radioGroupId="4"
               keepProportions="1" resourceNormal="hihat_png" opacityNormal="0.5"
               colourNormal="0" resourceOver="" opacityOver="0.5" colourOver="0"
               resourceDown="" opacityDown="1" colourDown="0"/>
  <LABEL name="db label" id="56a8c68b3fd380e8" memberName="dbLabel7" virtualName=""
         explicitFocusOrder="0" pos="320 520 32 40" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="FM"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="33"/>
  <LABEL name="db label" id="a358d95f525350f5" memberName="dbLabel8" virtualName=""
         explicitFocusOrder="0" pos="392 520 72 40" textCol="ff007f00"
         outlineCol="0" edTextCol="ff000000" edBkgCol="0" labelText="Additive"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" kerning="0" bold="0" italic="0"
         justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: full_sine_png, 203, "../img/full_sine.png"
static const unsigned char resource_PluginGui_full_sine_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,146,73,68,65,84,88,133,237,151,81,14,64,
48,16,68,183,226,94,110,225,18,110,230,146,252,116,35,153,166,97,219,21,131,125,63,66,144,241,76,75,147,136,108,66,192,240,116,0,101,60,61,99,133,253,249,158,32,47,48,162,38,208,64,237,120,39,52,70,146,
224,168,185,250,196,206,102,104,140,68,16,228,232,72,235,59,119,234,10,141,145,8,130,148,51,235,212,120,39,235,117,75,222,230,110,209,24,41,103,86,43,78,163,141,198,72,4,65,250,59,162,116,126,181,105,
140,68,16,196,175,35,74,227,191,238,135,141,40,198,245,208,15,140,24,161,49,178,3,63,99,23,114,126,178,233,88,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::full_sine_png = (const char*) resource_PluginGui_full_sine_png;
const int PluginGui::full_sine_pngSize = 203;

// JUCER_RESOURCE: half_sine_png, 179, "../img/half_sine.png"
static const unsigned char resource_PluginGui_half_sine_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,122,73,68,65,84,88,133,237,148,65,10,128,
32,20,5,191,209,189,186,69,135,236,22,157,172,22,33,193,72,20,102,248,130,55,155,40,50,198,73,77,17,177,133,0,67,111,129,204,120,251,198,130,251,249,27,145,31,20,201,37,88,224,234,249,75,100,138,164,224,
174,121,58,227,198,101,100,138,88,132,156,107,164,246,159,55,90,43,50,69,44,66,202,147,117,170,252,82,237,184,245,184,200,20,41,79,214,78,200,20,177,8,177,8,177,8,177,8,177,8,177,8,217,1,254,115,12,122,
78,109,41,249,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::half_sine_png = (const char*) resource_PluginGui_half_sine_png;
const int PluginGui::half_sine_pngSize = 179;

// JUCER_RESOURCE: abs_sine_png, 181, "../img/abs_sine.png"
static const unsigned char resource_PluginGui_abs_sine_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,124,73,68,65,84,88,133,237,148,81,10,128,
32,16,5,215,232,94,221,162,67,118,201,250,81,130,17,209,182,192,13,222,252,21,62,153,125,150,201,204,78,11,192,50,91,160,176,118,87,28,120,222,7,119,126,152,251,65,35,101,34,78,210,122,255,50,23,166,145,
100,252,107,122,19,183,214,121,115,153,48,141,72,132,220,223,200,232,25,147,143,114,97,26,145,8,169,111,214,205,185,147,55,151,9,211,72,125,179,78,34,76,35,18,33,18,33,18,33,18,33,18,33,18,33,23,197,62,
17,185,230,123,254,103,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::abs_sine_png = (const char*) resource_PluginGui_abs_sine_png;
const int PluginGui::abs_sine_pngSize = 181;

// JUCER_RESOURCE: quarter_sine_png, 181, "../img/quarter_sine.png"
static const unsigned char resource_PluginGui_quarter_sine_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,124,73,68,65,84,88,133,237,150,81,10,
128,32,16,68,215,232,94,221,162,67,118,139,78,150,31,41,209,196,162,174,31,205,199,60,8,177,28,120,77,33,38,51,187,140,128,229,111,129,74,91,228,40,215,40,131,57,154,70,86,247,73,164,133,137,28,113,35,
245,141,118,152,183,136,230,10,52,141,72,4,121,254,17,252,198,189,68,115,0,77,35,18,65,190,251,200,230,172,244,238,207,230,206,123,160,105,36,153,206,35,111,36,130,72,4,145,8,34,17,68,34,136,68,144,12,
123,138,13,78,65,207,81,74,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::quarter_sine_png = (const char*) resource_PluginGui_quarter_sine_png;
const int PluginGui::quarter_sine_pngSize = 181;

// JUCER_RESOURCE: camel_sine_png, 174, "../img/camel_sine.png"
static const unsigned char resource_PluginGui_camel_sine_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,117,73,68,65,84,88,133,237,148,209,9,192,
32,12,68,99,233,94,221,162,67,118,139,78,86,63,218,80,56,41,129,40,245,62,238,129,136,6,225,229,132,20,51,187,140,128,101,182,128,243,45,114,60,43,91,31,38,242,51,107,115,227,93,238,201,115,18,154,68,
36,130,20,243,57,18,253,117,111,61,128,38,17,137,32,18,65,218,201,186,5,47,122,235,200,121,111,52,137,188,115,100,50,52,137,72,4,145,8,34,17,68,34,136,68,16,137,32,21,146,46,14,97,109,79,27,36,0,0,0,0,
73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::camel_sine_png = (const char*) resource_PluginGui_camel_sine_png;
const int PluginGui::camel_sine_pngSize = 174;

// JUCER_RESOURCE: alternating_sine_png, 197, "../img/alternating_sine.png"
static const unsigned char resource_PluginGui_alternating_sine_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,140,73,68,65,84,88,133,99,100,96,
96,248,207,48,8,0,211,64,59,0,6,112,59,100,21,20,15,184,67,232,12,88,48,68,96,161,16,134,131,79,35,48,104,66,100,212,33,232,128,145,1,86,142,16,74,11,52,78,43,131,38,68,70,29,130,14,70,29,130,14,48,75,
86,123,2,58,112,201,103,67,233,169,36,186,224,32,132,26,52,33,130,40,71,8,1,26,151,51,131,38,68,70,29,130,14,70,29,130,14,136,207,53,48,64,163,22,220,160,9,145,81,135,160,3,210,211,8,12,80,185,197,54,
12,66,132,202,96,208,132,8,0,67,200,18,200,95,246,147,104,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::alternating_sine_png = (const char*) resource_PluginGui_alternating_sine_png;
const int PluginGui::alternating_sine_pngSize = 197;

// JUCER_RESOURCE: square_png, 179, "../img/square.png"
static const unsigned char resource_PluginGui_square_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,122,73,68,65,84,88,133,237,150,177,17,128,48,
12,3,29,142,189,216,130,37,216,44,75,66,99,83,40,85,34,238,80,161,111,146,198,190,143,10,59,45,34,238,16,96,251,91,160,216,223,91,39,59,157,92,185,96,34,197,236,203,216,36,19,153,68,44,130,88,4,177,8,
98,17,100,156,172,199,98,167,217,186,43,207,156,228,50,137,180,96,255,35,181,107,86,119,148,90,34,22,65,44,130,88,4,177,8,50,238,154,85,200,223,188,76,34,252,174,249,8,153,68,30,246,154,9,40,60,195,35,
102,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::square_png = (const char*) resource_PluginGui_square_png;
const int PluginGui::square_pngSize = 179;

// JUCER_RESOURCE: logarithmic_saw_png, 206, "../img/logarithmic_saw.png"
static const unsigned char resource_PluginGui_logarithmic_saw_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,34,0,0,0,30,8,6,0,0,0,73,255,204,20,0,0,0,149,73,68,65,84,88,133,237,150,81,
10,128,32,16,68,215,232,94,221,194,75,116,158,46,225,37,235,103,37,24,19,139,218,118,64,31,136,172,32,12,15,87,13,34,178,11,1,147,119,128,204,25,36,233,112,15,226,204,8,130,148,65,156,206,10,177,17,39,
104,130,204,197,74,212,57,65,109,12,177,145,12,154,193,245,143,33,54,178,64,189,65,125,247,142,193,125,200,170,179,26,166,49,18,196,234,63,210,58,91,208,149,52,70,234,93,243,150,134,1,164,3,35,72,237,
94,82,104,140,140,32,200,255,65,162,92,118,78,199,70,42,208,4,177,123,107,30,66,99,228,0,184,114,16,107,149,79,109,141,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::logarithmic_saw_png = (const char*) resource_PluginGui_logarithmic_saw_png;
const int PluginGui::logarithmic_saw_pngSize = 206;

// JUCER_RESOURCE: channeloff_png, 414, "../img/channeloff.png"
static const unsigned char resource_PluginGui_channeloff_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,16,0,0,0,16,8,6,0,0,0,31,243,255,97,0,0,0,6,98,75,71,68,0,0,0,0,0,0,249,67,187,127,
0,0,0,9,112,72,89,115,0,0,11,19,0,0,11,19,1,0,154,156,24,0,0,0,7,116,73,77,69,7,222,8,25,20,6,56,156,246,144,159,0,0,0,29,105,84,88,116,67,111,109,109,101,110,116,0,0,0,0,0,67,114,101,97,116,101,100,32,
119,105,116,104,32,71,73,77,80,100,46,101,7,0,0,1,2,73,68,65,84,56,203,165,211,61,78,66,65,20,5,224,143,193,82,119,224,62,104,40,166,196,194,202,10,66,65,137,149,75,120,121,193,21,216,104,108,164,32,178,
128,183,1,94,66,67,66,172,172,140,27,176,178,192,202,70,139,55,26,2,138,2,39,153,102,230,220,159,185,231,220,154,85,100,14,113,138,46,154,233,118,138,17,10,185,183,101,122,88,9,110,99,129,11,20,104,164,
83,164,187,69,226,252,128,204,64,230,67,166,239,55,100,250,137,51,176,86,185,122,104,249,11,153,86,226,182,161,150,254,188,192,185,220,141,255,160,234,242,26,71,117,209,25,142,229,27,90,95,69,105,46,58,
193,107,72,211,30,218,30,67,116,67,146,106,178,67,130,9,154,193,158,8,201,36,113,135,216,136,105,72,14,235,237,144,160,135,81,248,118,92,182,133,10,21,183,129,162,174,244,46,122,194,157,104,166,244,252,
167,145,24,163,35,247,80,79,186,62,138,14,112,43,122,81,154,111,168,60,198,165,220,85,229,196,245,101,186,199,44,233,60,89,26,88,47,181,221,145,27,127,133,212,246,93,231,79,44,229,73,181,37,137,229,213,
0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::channeloff_png = (const char*) resource_PluginGui_channeloff_png;
const int PluginGui::channeloff_pngSize = 414;

// JUCER_RESOURCE: channelon_png, 326, "../img/channelon.png"
static const unsigned char resource_PluginGui_channelon_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,16,0,0,0,16,8,6,0,0,0,31,243,255,97,0,0,0,6,98,75,71,68,0,0,0,0,0,0,249,67,187,127,
0,0,0,9,112,72,89,115,0,0,11,19,0,0,11,19,1,0,154,156,24,0,0,0,7,116,73,77,69,7,222,8,25,20,6,39,17,254,157,106,0,0,0,29,105,84,88,116,67,111,109,109,101,110,116,0,0,0,0,0,67,114,101,97,116,101,100,32,
119,105,116,104,32,71,73,77,80,100,46,101,7,0,0,0,170,73,68,65,84,56,203,173,147,177,13,194,48,16,69,159,143,148,176,81,20,101,129,84,84,68,217,5,89,176,0,204,16,193,0,30,0,69,202,8,169,88,131,12,64,145,
139,176,82,64,116,230,117,182,252,206,242,249,159,99,201,145,45,80,1,13,144,235,110,15,180,64,192,51,198,199,221,66,62,0,55,190,83,227,185,207,139,77,36,159,128,11,191,217,83,146,209,241,248,20,152,110,
94,35,207,20,148,60,233,24,156,190,249,133,141,157,104,195,172,84,162,221,182,210,72,244,85,22,114,33,17,209,144,88,233,69,19,102,165,21,32,36,20,8,162,217,174,13,114,141,103,156,146,216,49,80,146,1,197,
74,249,140,231,250,151,97,114,169,227,252,6,230,208,38,246,228,75,209,233,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::channelon_png = (const char*) resource_PluginGui_channelon_png;
const int PluginGui::channelon_pngSize = 326;

// JUCER_RESOURCE: toggle_off_sq_png, 118, "../img/toggle_off_sq.png"
static const unsigned char resource_PluginGui_toggle_off_sq_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,12,0,0,0,12,8,6,0,0,0,86,117,92,231,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,
0,48,73,68,65,84,40,21,99,100,172,103,248,207,64,2,96,1,169,253,223,72,156,14,160,225,12,76,196,41,69,168,26,213,128,8,11,220,44,218,135,18,56,166,65,49,72,44,0,0,186,23,4,27,1,178,34,38,0,0,0,0,73,69,
78,68,174,66,96,130,0,0};

const char* PluginGui::toggle_off_sq_png = (const char*) resource_PluginGui_toggle_off_sq_png;
const int PluginGui::toggle_off_sq_pngSize = 118;

// JUCER_RESOURCE: toggle_on_sq_png, 134, "../img/toggle_on_sq.png"
static const unsigned char resource_PluginGui_toggle_on_sq_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,12,0,0,0,12,8,6,0,0,0,86,117,92,231,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,0,
64,73,68,65,84,40,21,99,100,172,103,248,207,64,2,96,1,169,253,223,72,156,14,160,225,12,96,13,32,229,32,14,62,0,51,148,9,159,34,108,114,163,26,176,133,10,186,24,60,30,96,225,140,174,0,157,15,214,64,40,
210,144,53,1,0,212,234,7,159,245,36,0,105,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::toggle_on_sq_png = (const char*) resource_PluginGui_toggle_on_sq_png;
const int PluginGui::toggle_on_sq_pngSize = 134;

// JUCER_RESOURCE: line_border_horiz_png, 108, "../img/line_border_horiz.png"
static const unsigned char resource_PluginGui_line_border_horiz_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,22,0,0,0,6,8,6,0,0,0,199,98,110,160,0,0,0,1,115,82,71,66,0,174,206,28,233,
0,0,0,38,73,68,65,84,40,21,99,100,96,96,248,15,196,84,7,76,84,55,17,106,32,11,67,176,32,77,204,166,153,139,25,129,206,29,90,97,12,0,88,84,2,111,238,165,1,44,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::line_border_horiz_png = (const char*) resource_PluginGui_line_border_horiz_png;
const int PluginGui::line_border_horiz_pngSize = 108;

// JUCER_RESOURCE: line_border_vert_png, 107, "../img/line_border_vert.png"
static const unsigned char resource_PluginGui_line_border_vert_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,6,0,0,0,22,8,6,0,0,0,227,26,237,211,0,0,0,1,115,82,71,66,0,174,206,28,233,0,
0,0,37,73,68,65,84,40,21,99,100,96,96,248,15,196,12,12,193,130,96,138,97,237,123,48,205,4,225,97,146,163,18,24,97,50,236,131,4,0,123,194,3,43,141,43,209,32,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::line_border_vert_png = (const char*) resource_PluginGui_line_border_vert_png;
const int PluginGui::line_border_vert_pngSize = 107;

// JUCER_RESOURCE: algo_switch_off_png, 162, "../img/algo_switch_off.png"
static const unsigned char resource_PluginGui_algo_switch_off_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,52,0,0,0,20,8,6,0,0,0,194,212,30,221,0,0,0,1,115,82,71,66,0,174,206,28,233,0,
0,0,92,73,68,65,84,88,9,237,151,177,13,0,32,12,195,90,196,223,136,203,129,178,248,133,168,74,167,140,113,60,53,115,197,137,70,55,139,229,236,30,68,79,78,140,30,40,80,24,136,45,52,147,13,105,122,161,149,
13,177,133,102,178,33,77,47,180,178,33,182,208,76,54,164,233,133,86,54,196,22,154,201,134,52,189,208,234,127,172,245,233,117,185,11,224,6,4,43,49,160,14,163,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::algo_switch_off_png = (const char*) resource_PluginGui_algo_switch_off_png;
const int PluginGui::algo_switch_off_pngSize = 162;

// JUCER_RESOURCE: algo_switch_on_png, 168, "../img/algo_switch_on.png"
static const unsigned char resource_PluginGui_algo_switch_on_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,52,0,0,0,20,8,6,0,0,0,194,212,30,221,0,0,0,1,115,82,71,66,0,174,206,28,233,0,
0,0,98,73,68,65,84,88,9,237,151,177,13,192,48,12,195,236,52,223,244,200,92,153,115,10,7,233,196,19,4,67,158,52,138,226,228,204,21,21,141,110,94,150,231,205,22,72,223,174,24,45,72,0,97,32,140,33,25,109,
72,82,11,74,217,16,198,144,140,54,36,169,5,165,108,8,99,72,70,27,146,212,130,82,54,132,49,36,163,13,73,106,65,169,255,99,189,159,94,151,59,99,230,7,124,25,120,111,199,0,0,0,0,73,69,78,68,174,66,96,130,
0,0};

const char* PluginGui::algo_switch_on_png = (const char*) resource_PluginGui_algo_switch_on_png;
const int PluginGui::algo_switch_on_pngSize = 168;

// JUCER_RESOURCE: algo_switch_on2_png, 169, "../img/algo_switch_on2.png"
static const unsigned char resource_PluginGui_algo_switch_on2_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,52,0,0,0,20,8,6,0,0,0,194,212,30,221,0,0,0,1,115,82,71,66,0,174,206,28,233,0,
0,0,99,73,68,65,84,88,9,237,151,177,13,128,48,16,3,31,68,145,58,115,48,123,166,201,48,116,33,130,226,86,176,44,127,229,210,231,171,254,168,209,87,25,221,245,177,204,199,3,233,110,117,122,144,64,17,32,
182,208,76,49,164,233,133,86,49,196,22,154,41,134,52,189,208,42,134,216,66,51,197,144,166,23,90,197,16,91,104,166,24,210,244,66,171,255,99,221,159,158,203,189,241,194,4,31,18,119,100,16,0,0,0,0,73,69,
78,68,174,66,96,130,0,0};

const char* PluginGui::algo_switch_on2_png = (const char*) resource_PluginGui_algo_switch_on2_png;
const int PluginGui::algo_switch_on2_pngSize = 169;

// JUCER_RESOURCE: algo_switch_on3_png, 151, "../img/algo_switch_on3.png"
static const unsigned char resource_PluginGui_algo_switch_on3_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,52,0,0,0,20,8,6,0,0,0,194,212,30,221,0,0,0,1,115,82,71,66,0,174,206,28,233,0,
0,0,81,73,68,65,84,88,9,237,146,65,10,0,32,12,195,166,71,159,237,199,21,252,65,47,65,70,118,47,165,201,70,237,117,170,209,205,70,91,222,20,7,253,110,84,67,26,130,9,248,114,48,240,184,78,67,49,50,56,160,
33,24,120,92,167,161,24,25,28,208,16,12,60,174,211,80,140,12,14,180,51,116,1,132,232,1,179,41,122,114,149,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::algo_switch_on3_png = (const char*) resource_PluginGui_algo_switch_on3_png;
const int PluginGui::algo_switch_on3_pngSize = 151;

// JUCER_RESOURCE: twoopAm_png, 1872, "../img/Two-OP AM.png"
static const unsigned char resource_PluginGui_twoopAm_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,183,0,0,0,171,8,6,0,0,0,36,122,13,127,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,7,10,
73,68,65,84,120,1,237,157,191,110,84,71,20,135,231,174,221,208,145,221,164,67,72,116,161,112,30,0,185,72,73,210,217,239,1,72,32,37,101,156,50,145,82,36,77,222,194,86,20,41,166,11,5,242,3,64,65,164,20,
150,44,58,194,138,138,10,103,178,70,1,123,61,123,175,239,222,57,103,207,153,217,143,6,239,253,115,230,204,247,251,52,26,95,175,215,77,216,31,199,80,235,191,102,244,125,216,249,103,175,214,233,49,175,110,
2,163,238,211,156,133,64,185,4,144,187,220,236,232,252,10,2,200,125,5,32,78,151,75,0,185,203,205,142,206,175,32,128,220,87,0,226,116,185,4,54,91,91,223,157,54,173,231,188,157,56,248,116,47,196,127,191,
243,214,22,253,216,18,96,229,182,229,207,232,138,4,144,91,17,46,165,109,9,32,183,45,127,70,87,36,128,220,138,112,41,109,75,0,185,109,249,51,186,34,1,228,86,132,75,105,91,2,200,109,203,159,209,21,9,32,
183,34,92,74,219,18,64,110,91,254,140,174,72,0,185,21,225,82,218,150,0,114,219,242,103,116,69,2,200,173,8,151,210,182,4,144,219,150,63,163,43,18,64,110,69,184,148,182,37,208,180,254,130,176,208,91,94,
155,237,141,122,127,1,121,65,118,241,233,105,57,111,21,94,208,127,77,135,88,185,107,74,147,185,204,17,64,238,57,28,188,168,137,64,251,111,226,40,204,242,214,157,155,10,85,237,75,30,31,157,216,55,65,7,
9,1,86,238,4,9,7,106,33,128,220,181,36,201,60,18,2,200,157,32,225,64,45,4,144,187,150,36,153,71,66,0,185,19,36,28,168,133,0,114,215,146,36,243,72,8,32,119,130,132,3,181,16,64,238,90,146,100,30,9,1,228,
78,144,112,160,22,2,200,93,75,146,204,35,33,128,220,9,18,14,212,66,0,185,107,73,146,121,36,4,144,59,65,194,129,90,8,32,119,45,73,50,143,132,0,114,39,72,56,80,11,1,228,174,37,73,230,145,16,64,238,4,9,7,
106,33,128,220,181,36,201,60,18,2,200,157,32,225,64,45,4,144,187,150,36,153,71,66,0,185,19,36,28,168,133,0,114,215,146,36,243,72,8,32,119,130,132,3,181,16,88,233,231,150,212,2,173,115,30,251,99,95,31,
31,39,244,177,120,157,115,118,122,146,149,219,105,48,180,149,79,160,125,229,62,152,252,144,95,126,86,225,167,55,34,101,40,2,129,101,9,180,203,29,227,55,203,22,227,122,8,120,34,192,182,196,83,26,244,34,
74,0,185,69,113,82,204,19,129,246,109,137,167,46,75,234,197,234,233,132,183,167,52,14,50,99,229,118,16,2,45,232,16,216,12,77,243,173,78,233,143,85,101,158,186,124,44,199,23,16,232,71,96,51,236,188,254,
177,223,165,3,175,218,222,64,238,129,232,184,45,143,0,219,146,60,126,220,237,152,0,114,59,14,135,214,242,8,32,119,30,63,238,118,76,0,185,29,135,67,107,121,4,144,59,143,31,119,59,38,128,220,142,195,161,
181,60,2,200,157,199,143,187,29,19,64,110,199,225,208,90,30,1,228,206,227,199,221,142,9,32,183,227,112,104,45,143,0,114,231,241,227,110,199,4,144,219,113,56,180,150,71,0,185,243,248,113,183,99,2,200,237,
56,28,90,203,35,128,220,121,252,184,219,49,1,228,118,28,14,173,229,17,64,238,60,126,220,237,152,0,114,59,14,135,214,242,8,32,119,30,63,238,118,76,0,185,29,135,67,107,121,4,86,250,185,37,199,71,39,121,
221,114,55,4,150,32,192,202,189,4,44,46,45,139,0,114,151,149,23,221,46,65,64,125,91,18,159,158,54,75,244,195,165,16,16,35,192,202,45,134,146,66,222,8,32,183,183,68,232,71,140,0,114,139,161,164,80,22,129,
131,241,31,225,247,201,231,89,53,46,221,140,220,151,128,240,210,136,64,12,95,133,119,241,121,216,159,252,28,30,223,24,75,116,129,220,18,20,169,33,67,32,134,217,3,142,120,63,188,125,251,119,56,152,220,
11,127,126,153,245,192,3,185,101,98,161,138,44,129,113,136,241,151,240,230,249,179,240,219,103,119,135,150,70,238,161,228,184,79,159,64,140,183,195,233,233,97,24,184,31,71,110,253,136,24,33,151,192,192,
253,56,114,231,130,231,254,213,16,24,176,31,71,238,213,68,195,40,114,4,122,239,199,145,91,14,58,149,86,73,224,226,126,124,127,114,123,209,208,188,239,99,17,149,18,143,173,243,159,234,107,194,187,16,154,
95,195,181,107,123,225,238,203,233,135,248,88,185,63,144,224,255,114,9,156,237,199,99,188,119,249,249,56,43,119,185,145,206,119,190,206,43,247,60,137,217,34,222,188,8,97,244,136,149,251,50,24,94,151,79,
32,134,247,139,54,114,151,31,37,51,56,39,48,13,163,230,65,248,100,107,43,236,188,58,204,250,217,253,121,77,190,130,128,33,129,228,27,202,39,239,155,65,110,195,76,86,50,244,238,180,140,239,171,134,127,
207,240,56,196,230,97,216,125,61,219,103,207,255,67,238,121,30,188,42,134,64,243,87,104,70,15,207,182,31,109,45,179,231,110,35,195,113,175,4,230,246,213,93,77,178,114,119,209,225,156,31,2,45,251,234,174,
6,145,187,139,14,231,188,16,104,221,87,119,53,136,220,93,116,56,103,75,224,255,31,198,116,237,171,187,26,100,207,221,69,135,115,86,4,166,179,159,50,222,15,215,183,190,24,42,246,89,227,172,220,86,241,49,
110,74,96,192,190,58,45,114,126,4,185,207,89,240,149,37,129,38,28,206,158,87,63,90,244,188,122,104,91,200,61,148,28,247,201,18,216,153,126,45,91,112,246,214,41,233,130,212,131,128,23,2,200,237,37,9,250,
16,39,160,190,45,105,182,55,162,120,215,142,11,242,169,182,126,194,97,229,246,147,5,157,8,19,64,110,97,160,148,243,67,64,125,91,114,113,170,183,238,220,188,248,178,154,175,249,91,63,62,163,100,229,246,
153,11,93,9,16,64,110,1,136,148,240,73,0,185,125,230,66,87,2,4,144,91,0,34,37,124,18,64,110,159,185,208,149,0,1,228,22,128,72,9,159,4,144,219,103,46,116,37,64,0,185,5,32,82,194,39,1,228,246,153,11,93,
9,16,64,110,1,136,148,240,73,0,185,125,230,66,87,2,4,144,91,0,34,37,124,18,64,110,159,185,208,149,0,1,228,22,128,72,9,159,4,144,219,103,46,116,37,64,0,185,5,32,82,194,39,1,228,246,153,11,93,9,16,64,110,
1,136,148,240,73,0,185,125,230,66,87,2,4,144,91,0,34,37,124,18,64,110,159,185,208,149,0,1,228,22,128,72,9,159,4,144,219,103,46,116,37,64,96,165,159,91,34,208,175,255,18,195,255,228,156,255,185,21,214,
33,43,119,97,129,209,110,127,2,200,221,159,21,87,22,70,0,185,11,11,140,118,251,19,64,238,254,172,184,178,48,2,200,93,88,96,180,219,159,0,79,75,250,179,234,119,229,238,180,233,119,33,87,105,19,96,229,214,
38,76,125,51,2,200,109,134,158,129,181,9,32,183,54,97,234,155,17,64,110,51,244,12,172,77,0,185,181,9,83,223,140,0,114,155,161,103,96,109,2,200,173,77,152,250,102,4,144,219,12,61,3,107,19,64,110,109,194,
212,55,35,128,220,102,232,25,88,155,0,114,107,19,166,190,25,1,228,54,67,207,192,218,4,144,91,155,48,245,205,8,32,183,25,122,6,214,38,128,220,218,132,169,111,70,0,185,205,208,51,176,54,1,228,214,38,76,
125,51,2,200,109,134,158,129,181,9,32,183,54,97,234,155,17,64,110,51,244,12,172,77,0,185,181,9,83,223,140,0,114,155,161,103,96,109,2,200,173,77,152,250,102,4,86,250,185,37,199,71,39,102,19,101,224,245,
35,192,202,189,126,153,175,205,140,145,123,109,162,94,191,137,254,7,149,161,178,110,197,68,224,25,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::twoopAm_png = (const char*) resource_PluginGui_twoopAm_png;
const int PluginGui::twoopAm_pngSize = 1872;

// JUCER_RESOURCE: twoopFm_png, 1203, "../img/Two-OP FM.png"
static const unsigned char resource_PluginGui_twoopFm_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,219,0,0,0,81,8,6,0,0,0,91,168,211,254,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,4,109,
73,68,65,84,120,1,237,221,177,79,147,65,24,199,241,231,10,11,147,218,202,102,76,220,100,168,127,128,97,208,193,4,221,202,127,225,0,38,144,232,40,140,154,104,162,131,254,23,109,140,137,176,201,96,248,3,
100,208,196,129,132,184,41,13,46,76,224,249,86,36,185,148,247,170,190,125,142,222,221,251,197,193,151,123,233,241,220,231,233,47,111,175,77,139,145,110,211,74,174,95,166,177,46,157,239,107,185,46,143,
117,165,37,208,72,171,92,170,69,32,93,1,194,150,110,239,168,60,49,1,194,150,88,195,40,55,93,1,194,150,110,239,168,60,49,1,194,150,88,195,40,55,93,129,105,111,233,139,125,227,61,23,219,137,222,229,53,177,
63,31,199,86,22,245,32,224,10,112,101,115,53,56,70,32,160,0,97,11,136,203,212,8,184,2,132,205,213,224,24,129,128,2,132,45,32,46,83,35,224,10,16,54,87,131,99,4,2,10,16,182,128,184,76,141,128,43,64,216,
92,13,142,17,8,40,64,216,2,226,50,53,2,174,0,97,115,53,56,70,32,160,0,97,11,136,203,212,8,184,2,132,205,213,224,24,129,128,2,132,45,32,46,83,35,224,10,16,54,87,131,99,4,2,10,16,182,128,184,76,141,128,
43,96,188,31,248,163,244,22,27,51,63,149,239,7,10,185,146,127,142,237,135,227,137,188,53,9,231,146,102,68,54,196,149,45,178,134,80,78,190,2,132,45,223,222,178,178,200,4,252,239,212,14,80,232,181,155,87,
3,204,58,249,41,119,183,247,38,95,132,83,1,206,14,70,68,135,92,217,34,106,6,165,228,45,64,216,242,238,47,171,139,72,128,176,69,212,12,74,201,91,128,176,229,221,95,86,23,145,0,97,139,168,25,148,146,183,
0,97,203,187,191,172,46,34,1,194,22,81,51,40,37,111,1,194,150,119,127,89,93,68,2,132,45,162,102,80,74,222,2,132,45,239,254,178,186,136,4,8,91,68,205,160,148,188,5,8,91,222,253,101,117,17,9,16,182,136,
154,65,41,121,11,16,182,188,251,203,234,34,18,32,108,17,53,131,82,34,18,232,53,223,201,219,214,117,205,138,8,155,166,38,115,229,35,96,229,174,28,217,29,233,182,94,200,230,149,166,198,194,8,155,134,34,
115,228,41,96,165,120,115,181,93,150,195,195,47,210,107,45,201,251,91,99,189,217,154,176,229,121,55,97,85,186,2,77,177,246,165,28,236,124,148,55,179,11,85,167,38,108,85,229,184,93,253,4,172,157,147,227,
227,13,169,184,159,35,108,245,187,203,176,226,113,5,42,238,231,8,219,184,240,220,190,158,2,21,246,115,132,173,158,119,21,86,173,39,240,207,251,57,194,166,135,206,76,117,22,112,247,115,221,214,92,25,197,
88,79,101,150,77,88,251,177,110,115,50,31,183,254,252,71,189,232,39,229,252,55,229,193,126,206,216,59,197,75,5,175,101,102,102,77,22,190,246,79,111,194,149,237,84,130,255,17,208,18,24,236,231,172,93,26,
126,125,206,127,101,235,181,158,168,252,238,103,7,42,211,48,9,2,9,10,156,238,231,238,75,111,118,213,31,54,107,31,38,184,56,74,70,32,62,1,43,166,248,39,60,140,140,175,53,84,148,143,64,95,26,230,129,92,
106,183,165,243,109,195,127,101,203,103,193,172,4,129,243,21,48,114,36,98,156,39,72,182,126,255,126,194,166,221,6,165,63,34,249,223,101,213,236,143,78,74,104,231,234,207,118,110,138,53,43,178,184,255,
105,184,135,132,109,88,132,239,17,168,36,96,62,139,105,172,12,30,46,250,110,62,45,198,60,242,157,84,26,215,121,86,83,169,24,166,65,64,89,96,176,47,91,151,11,237,87,114,123,171,120,248,232,255,154,150,
206,254,83,255,105,133,51,243,83,132,77,129,145,41,34,19,240,236,203,70,85,201,195,200,81,58,156,67,160,92,192,187,47,43,255,241,147,81,194,54,74,135,115,8,184,2,198,20,79,122,52,86,71,237,203,220,31,
31,62,230,117,182,97,17,190,71,224,172,64,191,120,110,99,89,46,182,111,84,13,218,96,74,174,108,103,97,25,65,224,68,160,194,190,108,20,29,97,27,165,195,185,250,10,24,217,40,94,47,91,45,123,189,172,42,10,
97,171,42,199,237,242,22,232,244,239,105,47,144,61,155,182,40,243,33,224,17,32,108,30,24,134,17,208,22,32,108,218,162,204,135,128,71,128,176,121,96,24,70,64,91,128,176,105,139,50,31,2,30,1,194,230,129,
97,24,1,109,1,194,166,45,202,124,8,120,4,8,155,7,134,97,4,180,5,8,155,182,40,243,33,224,17,32,108,30,24,134,17,208,22,32,108,218,162,204,135,128,71,128,176,121,96,24,70,64,91,128,176,105,139,50,31,2,30,
1,194,230,129,97,24,1,109,129,115,125,139,205,238,246,158,118,253,204,87,34,128,115,9,74,4,67,92,217,34,104,2,37,212,67,128,176,213,163,207,172,50,2,129,95,213,163,177,117,99,210,56,193,0,0,0,0,73,69,
78,68,174,66,96,130,0,0};

const char* PluginGui::twoopFm_png = (const char*) resource_PluginGui_twoopFm_png;
const int PluginGui::twoopFm_pngSize = 1203;

// JUCER_RESOURCE: bassdrum_png, 234, "../img/bassdrum.png"
static const unsigned char resource_PluginGui_bassdrum_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,30,0,0,0,30,8,6,0,0,0,59,48,174,162,0,0,0,176,73,68,65,84,72,75,237,151,193,17,128,
32,12,4,67,101,118,97,145,118,97,101,250,138,206,28,19,47,4,4,30,248,211,33,89,238,66,16,146,136,92,50,224,73,243,130,143,160,29,251,119,28,87,220,29,172,64,156,185,245,29,5,146,113,182,226,238,224,70,
138,30,3,140,124,185,226,5,182,186,162,210,153,215,106,111,162,194,213,107,213,122,129,69,200,86,151,149,220,91,34,24,199,173,222,0,117,194,251,2,59,219,111,66,171,181,182,88,211,223,251,184,27,24,149,
48,112,112,53,43,198,254,31,119,7,51,160,78,185,185,226,105,193,149,74,237,26,51,197,205,193,184,39,235,212,180,143,75,143,185,238,115,245,48,112,240,194,16,13,227,55,137,104,102,18,119,3,1,34,108,1,9,
174,221,30,0,0,0,0,73,69,78,68,174,66,96,130,0,0,0};

const char* PluginGui::bassdrum_png = (const char*) resource_PluginGui_bassdrum_png;
const int PluginGui::bassdrum_pngSize = 234;

// JUCER_RESOURCE: snare_png, 261, "../img/snare.png"
static const unsigned char resource_PluginGui_snare_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,30,0,0,0,30,8,6,0,0,0,59,48,174,162,0,0,0,203,73,68,65,84,72,75,237,150,209,13,131,48,
12,5,195,100,221,162,67,118,11,38,43,63,88,149,94,244,240,65,80,172,86,229,51,56,62,46,137,77,150,214,218,187,21,60,203,239,128,31,251,242,173,199,203,120,191,241,237,224,151,49,120,238,227,14,104,198,
185,241,52,176,130,194,76,197,79,154,198,116,111,60,29,28,64,103,24,159,76,77,77,190,222,120,58,152,2,93,121,102,101,36,249,63,198,229,224,48,202,246,152,246,119,115,72,123,227,50,112,152,210,114,82,115,
55,15,239,113,25,56,76,92,171,204,246,88,207,8,54,46,7,103,102,244,253,105,99,154,56,139,251,131,187,203,222,104,235,116,117,45,167,252,250,223,41,219,83,173,138,20,236,202,136,246,110,216,120,198,111,
32,180,101,74,220,248,101,79,193,112,101,190,0,76,15,19,140,227,198,48,33,13,219,0,129,40,94,1,181,64,195,180,0,0,0,0,73,69,78,68,174,66,96,130,0,0,0};

const char* PluginGui::snare_png = (const char*) resource_PluginGui_snare_png;
const int PluginGui::snare_pngSize = 261;

// JUCER_RESOURCE: disabled_png, 210, "../img/disabled.png"
static const unsigned char resource_PluginGui_disabled_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,30,0,0,0,30,8,6,0,0,0,59,48,174,162,0,0,0,152,73,68,65,84,72,75,237,150,81,10,128,48,
12,67,187,75,122,72,47,169,63,78,161,48,146,148,97,55,168,95,194,218,62,147,56,93,51,179,203,18,174,86,224,191,92,47,171,63,167,207,231,246,8,154,15,250,199,86,167,129,187,80,245,1,200,122,252,114,145,
131,222,64,200,122,12,102,149,147,192,62,110,3,240,72,185,168,84,87,156,14,102,51,7,219,159,207,216,15,10,90,188,161,213,94,105,80,57,111,53,2,160,117,23,213,194,96,81,137,145,245,88,49,57,104,222,183,
90,5,138,219,109,225,255,113,240,0,130,218,112,198,104,66,112,189,192,65,227,244,182,52,171,111,130,58,64,1,51,47,73,27,0,0,0,0,73,69,78,68,174,66,96,130,0,0,0};

const char* PluginGui::disabled_png = (const char*) resource_PluginGui_disabled_png;
const int PluginGui::disabled_pngSize = 210;

// JUCER_RESOURCE: tom_png, 231, "../img/tom.png"
static const unsigned char resource_PluginGui_tom_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,30,0,0,0,30,8,6,0,0,0,59,48,174,162,0,0,0,174,73,68,65,84,72,75,237,150,209,13,128,32,12,
68,203,100,110,225,144,110,225,100,250,3,124,212,92,238,8,132,134,136,223,109,31,119,133,214,100,102,143,5,124,105,29,240,5,236,57,219,108,107,87,60,13,236,65,72,153,26,151,141,225,138,213,130,106,28,
5,151,66,141,189,171,157,38,249,88,113,56,216,95,86,181,199,37,15,196,115,197,97,96,127,98,245,57,13,239,113,24,88,29,80,195,21,111,48,114,96,91,237,156,225,3,228,63,179,186,88,163,174,59,53,142,174,197,
48,240,145,201,183,187,21,234,200,68,249,84,241,116,48,57,105,213,223,25,247,125,78,157,5,63,131,12,212,227,63,123,104,36,170,7,4,249,11,130,213,245,56,92,113,39,248,5,18,169,84,1,245,238,241,237,0,0,
0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::tom_png = (const char*) resource_PluginGui_tom_png;
const int PluginGui::tom_pngSize = 231;

// JUCER_RESOURCE: hihat_png, 189, "../img/hihat.png"
static const unsigned char resource_PluginGui_hihat_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,30,0,0,0,30,8,6,0,0,0,59,48,174,162,0,0,0,131,73,68,65,84,72,75,99,100,96,96,248,207,48,
0,128,113,212,98,122,133,58,249,65,109,15,117,226,65,242,156,58,106,49,241,225,70,247,160,94,133,195,109,97,196,187,25,164,146,244,56,166,186,197,184,12,36,205,35,132,85,67,67,6,225,227,1,179,152,176,
91,169,170,130,244,56,166,146,245,131,32,168,7,125,28,83,61,59,17,27,119,3,102,49,204,129,116,47,50,71,45,30,109,8,16,153,59,200,47,50,71,83,53,145,65,12,83,54,4,131,154,68,31,162,43,39,223,199,20,90,
12,0,73,108,52,1,92,98,8,34,0,0,0,0,73,69,78,68,174,66,96,130,0,0,0};

const char* PluginGui::hihat_png = (const char*) resource_PluginGui_hihat_png;
const int PluginGui::hihat_pngSize = 189;

// JUCER_RESOURCE: cymbal_png, 237, "../img/cymbal.png"
static const unsigned char resource_PluginGui_cymbal_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,30,0,0,0,30,8,6,0,0,0,59,48,174,162,0,0,0,178,73,68,65,84,72,75,237,149,209,13,128,48,
8,5,233,100,110,225,144,110,225,100,250,97,209,4,67,30,164,21,108,82,127,140,161,114,61,68,90,136,232,160,132,171,76,112,84,213,227,74,189,84,165,253,186,15,0,222,140,31,97,21,235,132,41,71,237,198,97,
96,9,146,38,168,0,138,41,54,14,7,51,208,107,200,42,192,84,55,14,7,123,129,70,179,187,21,68,254,167,171,127,15,102,83,217,213,117,34,169,205,222,108,156,6,70,255,175,22,111,54,238,14,230,132,222,38,67,
27,81,242,189,103,117,26,88,154,243,179,117,146,25,71,173,126,58,25,19,188,42,109,124,239,251,99,81,233,129,1,192,168,123,157,113,187,177,51,49,90,62,193,168,66,221,226,105,165,62,1,75,77,70,1,212,184,
6,141,0,0,0,0,73,69,78,68,174,66,96,130,0,0,0,0};

const char* PluginGui::cymbal_png = (const char*) resource_PluginGui_cymbal_png;
const int PluginGui::cymbal_pngSize = 237;

// JUCER_RESOURCE: adlib_png, 1605, "../img/adlib.png"
static const unsigned char resource_PluginGui_adlib_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,85,0,0,0,87,8,2,0,0,0,250,95,158,6,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,0,4,103,65,
77,65,0,0,177,143,11,252,97,5,0,0,0,9,112,72,89,115,0,0,14,195,0,0,14,195,1,199,111,168,100,0,0,5,218,73,68,65,84,120,94,237,156,107,79,34,73,20,134,253,15,26,227,7,163,162,130,220,193,75,54,222,88,84,
226,10,49,198,104,212,168,49,70,116,8,1,1,47,136,72,200,122,93,64,205,70,252,201,251,174,117,166,211,67,87,99,119,139,125,153,244,243,145,145,234,122,207,121,79,213,169,238,102,186,108,108,108,108,108,
108,108,108,108,108,100,137,70,163,139,139,139,127,118,148,165,165,165,191,20,19,143,199,167,166,166,104,54,250,51,61,61,93,171,213,26,141,198,63,50,188,190,190,254,171,134,247,247,247,108,54,171,60,4,
6,235,7,161,80,232,250,250,250,246,246,246,111,9,143,143,143,137,68,2,243,251,67,49,179,179,179,61,61,61,52,180,85,184,188,188,124,120,120,32,209,34,52,232,7,8,129,134,42,192,23,105,54,250,19,14,135,171,
213,234,253,253,61,233,22,129,16,104,171,130,92,46,167,170,10,140,212,15,2,129,64,169,84,146,171,130,213,213,85,181,85,208,219,219,75,67,91,133,124,62,15,169,36,90,132,6,253,128,85,1,54,23,74,177,2,224,
2,172,199,52,27,253,9,6,131,149,74,165,77,21,188,188,188,144,191,149,129,42,40,20,10,202,67,96,176,126,224,243,249,138,197,162,92,21,172,173,173,77,78,78,82,126,21,0,11,244,245,245,209,208,86,33,147,201,
200,85,129,90,253,64,91,21,224,91,52,27,253,193,66,136,237,240,238,238,142,116,139,208,86,5,205,102,243,252,252,92,85,21,24,169,31,164,211,233,167,167,39,18,253,43,104,19,214,215,215,21,186,64,72,254,
240,240,48,13,109,9,252,126,255,197,197,5,215,2,202,245,91,178,248,5,82,169,84,27,11,252,127,48,144,225,249,249,153,217,30,139,63,130,56,58,58,74,35,90,11,108,4,56,20,112,155,98,1,105,32,32,254,248,248,
152,217,30,44,47,47,187,221,110,26,209,114,96,234,103,103,103,220,42,0,16,191,185,185,217,82,8,80,222,221,221,77,223,255,13,72,38,147,200,42,41,254,21,174,126,32,36,95,14,85,123,225,202,202,74,52,26,165,
217,232,143,199,227,185,186,186,146,171,2,124,174,97,47,196,128,202,67,96,176,126,224,114,185,208,17,114,59,34,232,223,218,218,82,213,17,205,204,204,12,12,12,208,208,86,193,233,116,226,104,36,93,8,52,
232,7,8,129,134,42,192,223,211,108,12,225,224,224,128,187,16,32,4,248,92,67,21,96,115,81,85,5,6,235,31,27,27,203,229,114,114,29,209,206,206,142,218,42,24,26,26,162,161,173,194,222,222,94,173,86,35,209,
34,52,232,7,218,170,32,22,139,209,108,244,7,11,33,250,57,185,133,144,53,63,228,111,101,160,10,110,110,110,84,85,129,145,250,193,200,200,72,54,155,149,171,130,221,221,221,137,137,9,202,175,2,96,1,12,72,
67,91,133,237,237,109,185,42,80,171,31,176,42,88,88,88,160,20,43,0,46,64,91,77,179,209,31,156,103,208,20,115,143,70,218,170,224,237,237,173,90,173,42,15,129,193,250,129,195,225,200,100,50,220,219,132,
248,112,127,127,255,83,23,176,180,51,160,28,199,109,26,218,42,108,108,108,212,235,117,18,45,66,137,126,75,110,126,45,96,221,66,71,200,173,2,132,0,85,192,165,209,104,192,240,172,249,193,74,62,62,62,78,
195,89,145,193,193,193,116,58,205,173,2,32,141,2,196,99,239,16,182,253,120,60,110,109,253,0,235,144,156,5,208,47,183,84,1,148,247,247,247,211,55,173,14,118,129,72,36,242,227,199,15,110,254,185,250,1,66,
208,210,237,192,5,104,28,105,80,75,128,46,112,126,126,30,78,134,159,229,204,15,152,255,91,246,66,233,225,223,74,250,221,110,55,148,99,217,107,175,92,0,127,115,120,120,40,118,1,242,111,189,195,63,240,122,
189,115,115,115,104,123,160,92,238,118,160,20,169,126,192,214,63,182,4,146,7,62,128,17,240,175,116,61,243,128,206,4,202,113,236,81,165,92,128,91,5,64,122,254,55,163,126,24,190,84,42,105,83,46,128,16,28,
29,29,73,93,96,246,22,200,227,241,124,122,255,95,9,92,253,128,21,130,180,10,12,126,254,205,128,248,114,185,252,117,241,12,86,5,240,17,185,255,39,210,243,191,41,244,251,124,190,14,138,103,32,4,199,199,
199,210,42,48,221,249,63,16,8,84,42,149,206,138,7,92,253,128,85,65,203,225,23,46,152,53,234,249,247,233,233,41,247,14,215,215,145,171,2,233,249,223,48,253,193,96,80,238,229,151,142,128,125,4,45,51,142,
61,148,253,15,96,1,167,211,73,51,48,22,185,247,191,58,5,87,63,64,8,164,37,128,190,131,166,165,15,161,80,72,238,253,191,14,130,16,40,44,1,189,245,163,189,253,214,228,51,160,63,149,74,181,88,0,123,158,193,
37,16,14,135,117,72,62,224,234,7,200,54,165,254,3,189,243,143,101,175,227,123,158,28,112,25,249,254,39,198,251,31,219,30,246,39,154,224,119,98,70,255,99,54,104,245,117,48,63,48,163,255,219,188,225,241,
29,152,206,255,133,66,65,31,243,3,51,250,223,112,253,45,253,15,146,175,107,255,171,155,126,238,17,192,248,254,95,31,253,184,196,201,201,137,116,229,51,190,255,55,92,191,144,124,100,222,128,251,31,58,172,
255,92,231,131,102,179,89,169,84,12,214,143,156,124,235,254,15,241,201,100,82,122,243,3,32,249,166,184,5,36,247,251,191,142,32,167,31,226,133,155,63,200,60,62,161,217,232,207,119,156,127,152,231,25,220,
71,0,229,114,89,184,249,105,176,126,208,217,243,47,196,115,31,129,10,32,249,14,135,131,174,109,6,66,161,16,246,97,204,155,20,124,141,54,250,153,237,65,44,22,195,31,208,229,205,0,187,249,171,57,4,88,65,
200,238,31,112,95,132,18,63,2,134,231,205,165,31,248,124,62,109,247,191,241,21,37,239,191,153,206,246,82,180,61,255,104,163,95,252,139,8,100,222,227,241,208,149,76,11,123,254,85,175,215,113,92,33,125,
34,90,124,46,32,103,120,241,143,255,18,137,68,48,24,164,203,152,25,204,18,39,240,98,177,136,166,13,218,132,64,64,188,242,223,255,129,72,36,98,118,195,183,193,239,247,163,63,65,209,34,135,44,16,224,83,
253,130,225,145,118,203,191,234,199,240,122,189,16,131,64,228,243,121,114,182,12,226,255,255,3,110,183,240,239,223,184,184,92,46,4,130,165,151,11,148,255,62,239,185,217,216,216,216,216,88,133,174,174,
255,0,140,50,58,48,137,109,192,72,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* PluginGui::adlib_png = (const char*) resource_PluginGui_adlib_png;
const int PluginGui::adlib_pngSize = 1605;


//[EndFile] You can add extra defines here...
//[/EndFile]
