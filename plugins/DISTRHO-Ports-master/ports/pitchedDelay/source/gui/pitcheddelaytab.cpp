/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  13 Feb 2013 7:22:10pm

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

#include "pitcheddelaytab.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PitchedDelayTab::PitchedDelayTab (PitchedDelayAudioProcessor* processor, int delayindex)
    : filter(processor),
      delayIndex(delayindex),
      sDelay (0),
      label (0),
      cbSync (0),
      sPitch (0),
      tbPostPitch (0),
      sFeedback (0),
      label3 (0),
      sFreq (0),
      label4 (0),
      tbSemitones (0),
      sQfactor (0),
      label5 (0),
      cbFilter (0),
      label6 (0),
      sGain (0),
      label7 (0),
      tbEnable (0),
      sVolume (0),
      label8 (0),
      cbPitch (0),
      tbMono (0),
      tbStereo (0),
      tbPingpong (0),
      lPan (0),
      sPan (0),
      sPreDelay (0),
      label2 (0),
      sPreVolume (0)
{
	addAndMakeVisible (sDelay = new Slider (L"DelaySlider"));
	sDelay->setTooltip (L"Delay time");
	sDelay->setRange (0, 10, 0.25);
	sDelay->setSliderStyle (Slider::LinearBar);
	sDelay->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sDelay->addListener (this);

	addAndMakeVisible (label = new Label (L"new label",
									   L"Delay"));
	label->setFont (Font (15.0000f, Font::plain));
	label->setJustificationType (Justification::centredLeft);
	label->setEditable (false, false, false);
	label->setColour (TextEditor::textColourId, Colours::black);
	label->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (cbSync = new ComboBox (L"SyncComboBox"));
	cbSync->setTooltip (L"Quantisation");
	cbSync->setEditableText (false);
	cbSync->setJustificationType (Justification::centredLeft);
	cbSync->setTextWhenNothingSelected (String());
	cbSync->setTextWhenNoChoicesAvailable (L"(no choices)");
	cbSync->addItem (L"seconds", 1);
	cbSync->addItem (L"1/2", 2);
	cbSync->addItem (L"1/2T", 3);
	cbSync->addItem (L"1/4", 4);
	cbSync->addItem (L"1/4T", 5);
	cbSync->addItem (L"1/8", 6);
	cbSync->addItem (L"1/8T", 7);
	cbSync->addItem (L"1/16", 8);
	cbSync->addItem (L"1/16T", 9);
	cbSync->addItem (L"1/32", 10);
	cbSync->addListener (this);

	addAndMakeVisible (sPitch = new Slider (L"PitchSlider"));
	sPitch->setTooltip (L"Pitch-Shift");
	sPitch->setRange (-12, 12, 0.01);
	sPitch->setSliderStyle (Slider::LinearBar);
	sPitch->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sPitch->addListener (this);

	addAndMakeVisible (tbPostPitch = new ToggleButton (L"PostPitchButton"));
	tbPostPitch->setTooltip (L"When checked, pitch is applied only before feedback loop. Otherwise the feedback will be pitched to. Pitching inside the feedback results in a minimum delay time.");
	tbPostPitch->setButtonText (L"Pre-Feedback");
	tbPostPitch->addListener (this);

	addAndMakeVisible (sFeedback = new Slider (L"FeedbackSlider"));
	sFeedback->setTooltip (L"Delay feedback");
	sFeedback->setRange (0, 100, 0.1);
	sFeedback->setSliderStyle (Slider::LinearBar);
	sFeedback->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sFeedback->addListener (this);

	addAndMakeVisible (label3 = new Label (L"new label",
										L"Feedback [%]"));
	label3->setFont (Font (15.0000f, Font::plain));
	label3->setJustificationType (Justification::centredLeft);
	label3->setEditable (false, false, false);
	label3->setColour (TextEditor::textColourId, Colours::black);
	label3->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (sFreq = new Slider (L"FrequencySlider"));
	sFreq->setTooltip (L"Filter in feedback loop frequency");
	sFreq->setRange (20, 20000, 0.1);
	sFreq->setSliderStyle (Slider::LinearBar);
	sFreq->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sFreq->addListener (this);
	sFreq->setSkewFactor (0.2);

	addAndMakeVisible (label4 = new Label (L"new label",
										L"Frequency [Hz]"));
	label4->setFont (Font (15.0000f, Font::plain));
	label4->setJustificationType (Justification::centredLeft);
	label4->setEditable (false, false, false);
	label4->setColour (TextEditor::textColourId, Colours::black);
	label4->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (tbSemitones = new ToggleButton (L"SemitonesButton"));
	tbSemitones->setTooltip (L"Pitch only in semitones");
	tbSemitones->setButtonText (L"Semitones");
	tbSemitones->addListener (this);

	addAndMakeVisible (sQfactor = new Slider (L"QSlider"));
	sQfactor->setTooltip (L"Filter in feedback loop Q-factor");
	sQfactor->setRange (0.3, 10, 0.01);
	sQfactor->setSliderStyle (Slider::LinearBar);
	sQfactor->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sQfactor->addListener (this);
	sQfactor->setSkewFactor (0.3);

	addAndMakeVisible (label5 = new Label (L"new label",
										L"Q factor"));
	label5->setFont (Font (15.0000f, Font::plain));
	label5->setJustificationType (Justification::centredLeft);
	label5->setEditable (false, false, false);
	label5->setColour (TextEditor::textColourId, Colours::black);
	label5->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (cbFilter = new ComboBox (L"FilterTypeComboBox"));
	cbFilter->setTooltip (L"Filter Type. Maximum filter gain is limited to 0 dB to avoid feedback oscillation.");
	cbFilter->setEditableText (false);
	cbFilter->setJustificationType (Justification::centredLeft);
	cbFilter->setTextWhenNothingSelected (String());
	cbFilter->setTextWhenNoChoicesAvailable (L"(no choices)");
	cbFilter->addItem (L"Off", 1);
	cbFilter->addItem (L"Lowpass", 2);
	cbFilter->addItem (L"Highpass", 3);
	cbFilter->addItem (L"Lowshelf", 4);
	cbFilter->addItem (L"Highshelf", 5);
	cbFilter->addItem (L"Bell", 6);
	cbFilter->addItem (L"Bandpass", 7);
	cbFilter->addItem (L"Notch", 8);
	cbFilter->addListener (this);

	addAndMakeVisible (label6 = new Label (L"new label",
										L"Filter Type"));
	label6->setFont (Font (15.0000f, Font::plain));
	label6->setJustificationType (Justification::centredRight);
	label6->setEditable (false, false, false);
	label6->setColour (TextEditor::textColourId, Colours::black);
	label6->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (sGain = new Slider (L"GainSlider"));
	sGain->setTooltip (L"Filter in feedback loop gain (where available)");
	sGain->setRange (-24, 24, 0.1);
	sGain->setSliderStyle (Slider::LinearBar);
	sGain->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sGain->addListener (this);

	addAndMakeVisible (label7 = new Label (L"new label",
										L"Gain [dB]"));
	label7->setFont (Font (15.0000f, Font::plain));
	label7->setJustificationType (Justification::centredLeft);
	label7->setEditable (false, false, false);
	label7->setColour (TextEditor::textColourId, Colours::black);
	label7->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (tbEnable = new ToggleButton (L"EnabledButton"));
	tbEnable->setButtonText (L"Enabled");
	tbEnable->addListener (this);

	addAndMakeVisible (sVolume = new Slider (L"VolumeSlider"));
	sVolume->setTooltip (L"Delay tab volume");
	sVolume->setRange (-60, 0, 0.1);
	sVolume->setSliderStyle (Slider::LinearHorizontal);
	sVolume->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sVolume->addListener (this);

	addAndMakeVisible (label8 = new Label (L"new label",
										L"Volume"));
	label8->setFont (Font (15.0000f, Font::plain));
	label8->setJustificationType (Justification::centredLeft);
	label8->setEditable (false, false, false);
	label8->setColour (TextEditor::textColourId, Colours::black);
	label8->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (cbPitch = new ComboBox (L"new combo box"));
	cbPitch->setTooltip (L"Pitch shfiting algorithm. Off Disables pitching and saves resources.");
	cbPitch->setEditableText (false);
	cbPitch->setJustificationType (Justification::centredLeft);
	cbPitch->setTextWhenNothingSelected (String());
	cbPitch->setTextWhenNoChoicesAvailable (L"(no choices)");
	cbPitch->addListener (this);

	addAndMakeVisible (tbMono = new ToggleButton (L"MonoTogglebutton"));
	tbMono->setTooltip (L"Mono (mid) delay");
	tbMono->setButtonText (L"Mono");
	tbMono->setRadioGroupId (5000);
	tbMono->addListener (this);
	tbMono->setToggleState (true, dontSendNotification);

	addAndMakeVisible (tbStereo = new ToggleButton (L"StereoTogglebutton"));
	tbStereo->setTooltip (L"Stereo delay");
	tbStereo->setButtonText (L"Stereo");
	tbStereo->setRadioGroupId (5000);
	tbStereo->addListener (this);

	addAndMakeVisible (tbPingpong = new ToggleButton (L"PingpongTogglebutton"));
	tbPingpong->setTooltip (L"Stereo pingpong delay");
	tbPingpong->setButtonText (L"Pingpong");
	tbPingpong->setRadioGroupId (5000);
	tbPingpong->addListener (this);

	addAndMakeVisible (lPan = new Label (L"Pan/Balance Label",
									  L"Pan\n"));
	lPan->setFont (Font (15.0000f, Font::plain));
	lPan->setJustificationType (Justification::centredRight);
	lPan->setEditable (false, false, false);
	lPan->setColour (TextEditor::textColourId, Colours::black);
	lPan->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (sPan = new Slider (L"pan slider"));
	sPan->setRange (-100, 100, 1);
	sPan->setSliderStyle (Slider::LinearHorizontal);
	sPan->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
	sPan->addListener (this);

	addAndMakeVisible (sPreDelay = new Slider (L"DelaySlider"));
	sPreDelay->setTooltip (L"Delay time");
	sPreDelay->setRange (0, 10, 0.25);
	sPreDelay->setSliderStyle (Slider::LinearBar);
	sPreDelay->setTextBoxStyle (Slider::TextBoxRight, false, 50, 20);
	sPreDelay->addListener (this);

	addAndMakeVisible (label2 = new Label (L"new label",
										L"Predelay"));
	label2->setFont (Font (15.0000f, Font::plain));
	label2->setJustificationType (Justification::centredLeft);
	label2->setEditable (false, false, false);
	label2->setColour (TextEditor::textColourId, Colours::black);
	label2->setColour (TextEditor::backgroundColourId, Colour (0x0));

	addAndMakeVisible (sPreVolume = new Slider (L"new slider"));
	sPreVolume->setRange (-60, 0, 1);
	sPreVolume->setSliderStyle (Slider::LinearHorizontal);
	sPreVolume->setTextBoxStyle (Slider::TextBoxLeft, false, 30, 20);
	sPreVolume->addListener (this);


	//[UserPreSize]
	for (int i=0; i<DelayTabDsp::kNumParameters; ++i)
	 currentValues[i] = -1e8;

	cbFilter->setSelectedId(1, sendNotification);
	cbSync->setSelectedId(1, sendNotification);

	setDelayRange();
	setPitchRange();

	{
		cbPitch->clear();
		cbPitch->addItem("Off", 1);

		StringArray pitchers(filter->getDelay(0)->getPitchNames());

		for (int i=0; i<pitchers.size(); ++i)
		 cbPitch->addItem(pitchers[i], i+2);
	}

	//[/UserPreSize]

	setSize (500, 285);


	//[Constructor] You can add your own custom stuff here..
	startTimer(50);
	//[/Constructor]
}

PitchedDelayTab::~PitchedDelayTab()
{
	//[Destructor_pre]. You can add your own custom destruction code here..
	//[/Destructor_pre]

	deleteAndZero (sDelay);
	deleteAndZero (label);
	deleteAndZero (cbSync);
	deleteAndZero (sPitch);
	deleteAndZero (tbPostPitch);
	deleteAndZero (sFeedback);
	deleteAndZero (label3);
	deleteAndZero (sFreq);
	deleteAndZero (label4);
	deleteAndZero (tbSemitones);
	deleteAndZero (sQfactor);
	deleteAndZero (label5);
	deleteAndZero (cbFilter);
	deleteAndZero (label6);
	deleteAndZero (sGain);
	deleteAndZero (label7);
	deleteAndZero (tbEnable);
	deleteAndZero (sVolume);
	deleteAndZero (label8);
	deleteAndZero (cbPitch);
	deleteAndZero (tbMono);
	deleteAndZero (tbStereo);
	deleteAndZero (tbPingpong);
	deleteAndZero (lPan);
	deleteAndZero (sPan);
	deleteAndZero (sPreDelay);
	deleteAndZero (label2);
	deleteAndZero (sPreVolume);


	//[Destructor]. You can add your own custom destruction code here..
	//[/Destructor]
}

//==============================================================================
void PitchedDelayTab::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setGradientFill (ColourGradient (Colours::silver,
                                       0.0f, 0.0f,
                                       Colour (0xff707070),
                                       0.0f, (float) (getHeight()),
                                       false));
    g.fillRect (0, 0, getWidth() - 0, getHeight() - 0);

    g.setColour (Colour (0x80000000));
    g.drawRect (2, 25, getWidth() - 4, 105, 1);

    g.setColour (Colour (0x80000000));
    g.drawRect (2, 140, getWidth() - 4, 80, 1);

    g.setColour (Colour (0x40ffffff));
    g.fillPath (internalPath1);
    g.setColour (Colours::black);
    g.strokePath (internalPath1, PathStrokeType (1.0000f));

    g.setColour (Colours::black);
    g.setFont (Font (15.0000f, Font::plain));
    g.drawText (L"Volume",
                getWidth() - 80, 29, 70, 20,
                Justification::centred, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PitchedDelayTab::resized()
{
    sDelay->setBounds (110, 54, getWidth() - 230, 20);
    label->setBounds (5, 54, 100, 20);
    cbSync->setBounds (getWidth() - 110, 54, 100, 20);
    sPitch->setBounds (110, 79, getWidth() - 230, 20);
    tbPostPitch->setBounds (getWidth() - 110, 104, 100, 20);
    sFeedback->setBounds (110, 104, getWidth() - 230, 20);
    label3->setBounds (5, 104, 100, 20);
    sFreq->setBounds (110, 144, getWidth() - 250, 20);
    label4->setBounds (5, 144, 100, 20);
    tbSemitones->setBounds (getWidth() - 110, 79, 100, 20);
    sQfactor->setBounds (110, 169, getWidth() - 250, 20);
    label5->setBounds (5, 169, 100, 20);
    cbFilter->setBounds (getWidth() - 130, 169, 120, 20);
    label6->setBounds (getWidth() - 130, 144, 120, 20);
    sGain->setBounds (110, 194, getWidth() - 250, 20);
    label7->setBounds (5, 194, 100, 20);
    tbEnable->setBounds (10, 2, 100, 20);
    sVolume->setBounds (getWidth() - 390, 264, 380, 20);
    label8->setBounds (getWidth() - 60, 244, 50, 20);
    cbPitch->setBounds (5, 79, 100, 20);
    tbMono->setBounds (10, 224, 100, 20);
    tbStereo->setBounds (10, 244, 100, 20);
    tbPingpong->setBounds (10, 264, 100, 20);
    lPan->setBounds (110, 224, 80, 20);
    sPan->setBounds (210, 224, 150, 20);
    sPreDelay->setBounds (110, 29, getWidth() - 230, 20);
    label2->setBounds (5, 29, 100, 20);
    sPreVolume->setBounds (getWidth() - 110, 29, 100, 20);
    internalPath1.clear();
    internalPath1.startNewSubPath (4.0f, 77.0f);
    internalPath1.lineTo ((float) (getWidth() - 4), 77.0f);
    internalPath1.lineTo ((float) (getWidth() - 4), 126.0f);
    internalPath1.lineTo ((float) (getWidth() - 111), 126.0f);
    internalPath1.lineTo ((float) (getWidth() - 111), 101.0f);
    internalPath1.lineTo (4.0f, 101.0f);
    internalPath1.closeSubPath();

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PitchedDelayTab::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == sDelay)
    {
        //[UserSliderCode_sDelay] -- add your slider handling code here..
			const double delaySeconds = getSliderDelaySeconds();
			sendActionMessage("Tab" + String(delayIndex) + ":Delay:" + String(delaySeconds));
        //[/UserSliderCode_sDelay]
    }
    else if (sliderThatWasMoved == sPitch)
    {
        //[UserSliderCode_sPitch] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Pitch:" + String(sPitch->getValue()));
        //[/UserSliderCode_sPitch]
    }
    else if (sliderThatWasMoved == sFeedback)
    {
        //[UserSliderCode_sFeedback] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Feedback:" + String(sFeedback->getValue()));
        //[/UserSliderCode_sFeedback]
    }
    else if (sliderThatWasMoved == sFreq)
    {
        //[UserSliderCode_sFreq] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":EqFreq:" + String(sFreq->getValue()));
        //[/UserSliderCode_sFreq]
    }
    else if (sliderThatWasMoved == sQfactor)
    {
        //[UserSliderCode_sQfactor] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":EqQ:" + String(sQfactor->getValue()));
        //[/UserSliderCode_sQfactor]
    }
    else if (sliderThatWasMoved == sGain)
    {
        //[UserSliderCode_sGain] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":EqGain:" + String(sGain->getValue()));
        //[/UserSliderCode_sGain]
    }
    else if (sliderThatWasMoved == sVolume)
    {
        //[UserSliderCode_sVolume] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Volume:" + String(sVolume->getValue()));
        //[/UserSliderCode_sVolume]
    }
    else if (sliderThatWasMoved == sPan)
    {
        //[UserSliderCode_sPan] -- add your slider handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Pan:" + String(sPan->getValue()));
        //[/UserSliderCode_sPan]
    }
    else if (sliderThatWasMoved == sPreDelay)
    {
        //[UserSliderCode_sPreDelay] -- add your slider handling code here..
		sendActionMessage("Tab" + String(delayIndex) + ":Predelay:" + String(unQuantizeDelay(sPreDelay->getValue())));
        //[/UserSliderCode_sPreDelay]
    }
    else if (sliderThatWasMoved == sPreVolume)
    {
        //[UserSliderCode_sPreVolume] -- add your slider handling code here..
		sendActionMessage("Tab" + String(delayIndex) + ":PredelayVol:" + String(sPreVolume->getValue()));
        //[/UserSliderCode_sPreVolume]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void PitchedDelayTab::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == cbSync)
    {
        //[UserComboBoxCode_cbSync] -- add your combo box handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Sync:" + String(cbSync->getSelectedId() - 1 ) );
			setDelayRange();
        //[/UserComboBoxCode_cbSync]
    }
    else if (comboBoxThatHasChanged == cbFilter)
    {
        //[UserComboBoxCode_cbFilter] -- add your combo box handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":EqType:" + String(cbFilter->getSelectedId() - 1) );
        //[/UserComboBoxCode_cbFilter]
    }
    else if (comboBoxThatHasChanged == cbPitch)
    {
        //[UserComboBoxCode_cbPitch] -- add your combo box handling code here..
			sendActionMessage("Tab" + String(delayIndex) + ":PitchType:" + String(cbPitch->getSelectedId() - 1) );
        //[/UserComboBoxCode_cbPitch]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void PitchedDelayTab::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == tbPostPitch)
    {
        //[UserButtonCode_tbPostPitch] -- add your button handler code here..
			setDelayRange();
			sendActionMessage("Tab" + String(delayIndex) + ":PrePitch:" + String(tbPostPitch->getToggleState() ? 1 : 0) );
        //[/UserButtonCode_tbPostPitch]
    }
    else if (buttonThatWasClicked == tbSemitones)
    {
        //[UserButtonCode_tbSemitones] -- add your button handler code here..
			setPitchRange();
        //[/UserButtonCode_tbSemitones]
    }
    else if (buttonThatWasClicked == tbEnable)
    {
        //[UserButtonCode_tbEnable] -- add your button handler code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Enabled:" + String(tbEnable->getToggleState() ? 1 : 0) );
        //[/UserButtonCode_tbEnable]
    }
    else if (buttonThatWasClicked == tbMono)
    {
        //[UserButtonCode_tbMono] -- add your button handler code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Mode:0");
        //[/UserButtonCode_tbMono]
    }
    else if (buttonThatWasClicked == tbStereo)
    {
        //[UserButtonCode_tbStereo] -- add your button handler code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Mode:1");
        //[/UserButtonCode_tbStereo]
    }
    else if (buttonThatWasClicked == tbPingpong)
    {
        //[UserButtonCode_tbPingpong] -- add your button handler code here..
			sendActionMessage("Tab" + String(delayIndex) + ":Mode:2");
        //[/UserButtonCode_tbPingpong]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void PitchedDelayTab::setDelayRange(bool sendMessage)
{
	const double delayInSeconds = getDelaySeconds();

	DelayTabDsp* delayDsp = filter->getDelay(delayIndex);

	AudioPlayHead::CurrentPositionInfo info;
	if (AudioPlayHead* playHead = filter->getPlayHead())
		playHead->getCurrentPosition(info);
	else
		info.resetToDefault();

	const double timePerQuarter = 60. / (info.bpm > 0 ? info.bpm : 120);

	const Range<double> delayRange = delayDsp->getCurrentDelayRange();
	const double minDelay = delayRange.getStart();
	const double maxDelay = jmin((double) MAXDELAYSECONDS, delayRange.getEnd());

	double minimum;
	double maximum;
	double interval;

	switch (cbSync->getSelectedId())
	{
	case 1:
		minimum = floor(minDelay*1000 + 0.5) * 0.001;
		maximum = floor(maxDelay*1000) * 0.001;
		interval = 0.001;
		break;
	case 2: // 1/2
		minimum = ceil(0.5 * minDelay/timePerQuarter);
		maximum = floor(0.5 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 3: // 1/2T
		minimum = ceil(0.75 * minDelay/timePerQuarter);
		maximum = floor(0.75 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 4: // 1/4
		minimum = ceil(minDelay/timePerQuarter);
		maximum = floor(maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 5: // 1/4T
		minimum = ceil(1.5 * minDelay/timePerQuarter);
		maximum = floor(1.5 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 6: // 1/8
		minimum = ceil(2 * minDelay/timePerQuarter);
		maximum = floor(2 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 7: // 1/8T
		minimum = ceil(3 * minDelay/timePerQuarter);
		maximum = floor(3 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 8: // 1/16
		minimum = ceil(4 * minDelay/timePerQuarter);
		maximum = floor(4 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 9: // 1/16T
		minimum = ceil(6 * minDelay/timePerQuarter);
		maximum = floor(6 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	case 10: // 1/32
		minimum = ceil(8 * minDelay/timePerQuarter);
		maximum = floor(8 * maxDelay/timePerQuarter);
		interval = 1;
		break;
	default:
		jassertfalse;
		return;
	}

	sDelay->setRange(minimum, maximum, interval);
	sPreDelay->setRange(0, maximum, interval);
	setDelaySeconds(delayInSeconds, sendMessage);
	setPreDelaySeconds(getPreDelaySeconds(), sendMessage);
}

void PitchedDelayTab::setPitchRange()
{
	sPitch->setRange(-PITCHRANGESEMITONES, PITCHRANGESEMITONES, tbSemitones->getToggleState() ? 1 : 0.01);
}

double PitchedDelayTab::getDelaySeconds()
{
	DelayTabDsp* delayDsp = filter->getDelay(delayIndex);
	jassert(delayDsp != 0);

	const double val = delayDsp->getParam(DelayTabDsp::kDelay);

	return val;
}

void PitchedDelayTab::setDelaySeconds(double seconds, bool sendMessage)
{
	DelayTabDsp* delayDsp = filter->getDelay(delayIndex);

	const Range<double> delayRange = delayDsp->getCurrentDelayRange();
	seconds = delayRange.clipValue(seconds);

	const double val = quantizeDelay(seconds);

	sDelay->setValue(val, sendMessage ? sendNotification : dontSendNotification);
}

double PitchedDelayTab::getSliderDelaySeconds()
{
	const double val = sDelay->getValue();

	return unQuantizeDelay(val);
}

void PitchedDelayTab::setPreDelaySeconds(double seconds, bool sendMessage)
{
	DelayTabDsp* delayDsp = filter->getDelay(delayIndex);

	const Range<double> delayRange = delayDsp->getCurrentDelayRange();
	seconds = jmin(delayRange.getEnd(), seconds);;

	const double val = quantizeDelay(seconds);

	DBG("PreDelay: " + String(val, 1));

	sPreDelay->setValue(val, sendMessage ? sendNotification : dontSendNotification);
}

double PitchedDelayTab::getPreDelaySeconds()
{
	DelayTabDsp* delayDsp = filter->getDelay(delayIndex);
	jassert(delayDsp != 0);

	const double val = delayDsp->getParam(DelayTabDsp::kPreDelay);

	return val;
}

void PitchedDelayTab::updateBPM()
{
	AudioPlayHead::CurrentPositionInfo info;
	if (AudioPlayHead* playHead = filter->getPlayHead())
		playHead->getCurrentPosition(info);
	else
		info.resetToDefault();

	if (info.bpm != currentBPM)
	{
		currentBPM = info.bpm;
		setDelayRange(false);
		setDelaySeconds(getDelaySeconds(), true);
		setPreDelaySeconds(getPreDelaySeconds(), true);
	}
}

double PitchedDelayTab::quantizeDelay(double seconds)
{
	AudioPlayHead::CurrentPositionInfo info;
	if (AudioPlayHead* playHead = filter->getPlayHead())
		playHead->getCurrentPosition(info);
	else
		info.resetToDefault();

	const double timePerQuarter = 60. / (info.bpm > 0 ? info.bpm : 120);

	double val = 0;

	const int sync = cbSync->getSelectedId();

	switch (sync)
	{
	case 1: // seconds
		val = floor(seconds*1000 + 0.5) * 0.001;
		break;
	case 2: // 1/2
		val = floor(0.5 + 0.5 * seconds / timePerQuarter );
		break;
	case 3: // 1/2T
		val = floor(0.5 + 0.75 * seconds / timePerQuarter );
		break;
	case 4: // 1/4
		val = floor(0.5 + seconds / timePerQuarter);
		break;
	case 5: // 1/4T
		val = floor(0.5 + 1.5 * seconds / timePerQuarter );
		break;
	case 6: // 1/8
		val = floor(0.5 + 2 * seconds / timePerQuarter );
		break;
	case 7: // 8T
		val = floor(0.5 + 3. * seconds / timePerQuarter );
		break;
	case 8: // 16
		val = floor(0.5 + 4 * seconds / timePerQuarter );
		break;
	case 9: // 16T
		val = floor(0.5 + 6. * seconds / timePerQuarter );
		break;
	case 10: // 32
		val = floor(0.5 + 8 * seconds / timePerQuarter );
		break;
	default:
		jassertfalse;
	}

	return val;
}

double PitchedDelayTab::unQuantizeDelay(double sliderValue)
{
	AudioPlayHead::CurrentPositionInfo info;
	if (AudioPlayHead* playHead = filter->getPlayHead())
		playHead->getCurrentPosition(info);
	else
		info.resetToDefault();

	const double timePerQuarter = 60. / (info.bpm > 0 ? info.bpm : 120);

	switch (cbSync->getSelectedId())
	{
	case 1: // seconds
		return sliderValue;
	case 2: // 1/2
		return 2*timePerQuarter * sliderValue;
		//val = (0.5 * seconds / timePerQuarter );
	case 3: // 1/2T
		return 4/3. * timePerQuarter * sliderValue;
		//val = (2/3 * seconds / timePerQuarter );
	case 4: // 1/4
		return timePerQuarter * sliderValue;
		//val = (seconds / timePerQuarter);
	case 5: // 1/4T
		return 2/3. * timePerQuarter * sliderValue;
		//val = (4/3* seconds / timePerQuarter );
	case 6: // 1/8
		return 0.5 * timePerQuarter * sliderValue;
		//val = (2 * seconds / timePerQuarter );
	case 7: // 1/8T
		return 1/3. * timePerQuarter * sliderValue;
		//val = (seconds / timePerQuarter );
	case 8: // 16
		return 0.25 * timePerQuarter * sliderValue;
		//val = (4 * seconds / timePerQuarter );
	case 9: // 16T
		return 1/6. * timePerQuarter * sliderValue;
		//val = (seconds / timePerQuarter );
	case 10: // 32
		return 0.125 * timePerQuarter * sliderValue;
		//val = (4 * seconds / timePerQuarter );
	default:
		jassertfalse;
		return 0;
	}
}

void PitchedDelayTab::timerCallback()
{
	if (! isShowing())
		return;

	DelayTabDsp* delayDsp = filter->getDelay(delayIndex);

	for (int i=0; i<DelayTabDsp::kNumParameters; ++i)
	{
		const double oldVal = currentValues[i] ;
		const double value = delayDsp->getParam(i);
		if (fabs(oldVal - value) > 1e-8)
			setParam(i, value);
	}

	{
		const String curPan(lPan->getText());
		const String shouldPan(tbMono->getToggleState() ? "Pan" : tbStereo->getToggleState() ? "Out Balance" : "In Balance");

		if (curPan != shouldPan)
			lPan->setText(shouldPan, dontSendNotification);
	}

	sPreVolume->setEnabled(delayDsp->getParam(DelayTabDsp::kPreDelay) > 0);

	updateBPM();
}

void PitchedDelayTab::setParam(int index, double value)
{
	switch (index)
	{
		case DelayTabDsp::kPitch:
			sPitch->setValue(value, dontSendNotification);
			break;
		case DelayTabDsp::kSync:
			cbSync->setSelectedId((int) value+1, sendNotification);
			break;
		case DelayTabDsp::kPreDelay:
			sPreDelay->setValue(quantizeDelay(value), dontSendNotification);
			break;
		case DelayTabDsp::kPreDelayVol:
			sPreVolume->setValue(value, dontSendNotification);
			break;
		case DelayTabDsp::kDelay:
			setDelaySeconds(value, false);
			setDelayRange();
			break;
		case DelayTabDsp::kPitchType:
			cbPitch->setSelectedId((int) value + 1, sendNotification);
			sPitch->setEnabled(value > 0.5);
			break;
		case DelayTabDsp::kPrePitch:
			tbPostPitch->setToggleState(value > 0.5 , dontSendNotification);
			break;
		case DelayTabDsp::kFeedback:
			sFeedback->setValue(value, dontSendNotification);
			break;

		case DelayTabDsp::kFilterType:
			cbFilter->setSelectedId((int) value + 1, sendNotification);
			sFreq->setEnabled(value != 0);
			sQfactor->setEnabled(value != 0);
			sGain->setEnabled(value >=3 && value <= 5);
			break;
		case DelayTabDsp::kFilterFreq:
			sFreq->setValue(value, dontSendNotification);
			break;
		case DelayTabDsp::kFilterQ:
			sQfactor->setValue(value, dontSendNotification);
			break;
		case DelayTabDsp::kFilterGain:
			sGain->setValue(value, dontSendNotification);
			break;


		case DelayTabDsp::kMode:
			if (value < 0.5)
				tbMono->setToggleState(true, dontSendNotification);
			else if (value < 1.5)
				tbStereo->setToggleState(true, dontSendNotification);
			else
				tbPingpong->setToggleState(true, dontSendNotification);
			break;

		case DelayTabDsp::kVolume:
			sVolume->setValue(value, dontSendNotification);
			break;
		case DelayTabDsp::kPan:
			sPan->setValue(value, dontSendNotification);
			break;

		case DelayTabDsp::kEnabled:
			tbEnable->setToggleState(value > 0.5, dontSendNotification);
			break;
		default:
			jassertfalse;
			return;
	}
	currentValues[index] = value;
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

	This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PitchedDelayTab" componentName=""
                 parentClasses="public Component, public ActionBroadcaster, public Timer"
                 constructorParams="PitchedDelayAudioProcessor* processor, int delayindex"
                 variableInitialisers="filter(processor),&#10;delayIndex(delayindex)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="1" initialWidth="500" initialHeight="285">
  <BACKGROUND backgroundColour="ffffff">
    <RECT pos="0 0 0M 0M" fill="linear: 0 0, 0 0R, 0=ffc0c0c0, 1=ff707070"
          hasStroke="0"/>
    <RECT pos="2 25 4M 105" fill="solid: ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: 80000000"/>
    <RECT pos="2 140 4M 80" fill="solid: ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: 80000000"/>
    <PATH pos="0 0 100 100" fill="solid: 40ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: ff000000" nonZeroWinding="1">s 4 77 l 4R 77 l 4R 126 l 111R 126 l 111R 101 l 4 101 x</PATH>
    <TEXT pos="80R 29 70 20" fill="solid: ff000000" hasStroke="0" text="Volume"
          fontname="Default font" fontsize="15" bold="0" italic="0" justification="36"/>
  </BACKGROUND>
  <SLIDER name="DelaySlider" id="e11598fe9ccb5d4" memberName="sDelay" virtualName=""
          explicitFocusOrder="0" pos="110 54 230M 20" tooltip="Delay time"
          min="0" max="10" int="0.25" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="17c1100985312b05" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="5 54 100 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Delay" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="SyncComboBox" id="e67496d5c14bd1b4" memberName="cbSync"
            virtualName="" explicitFocusOrder="0" pos="110R 54 100 20" tooltip="Quantisation"
            editable="0" layout="33" items="seconds&#10;1/2&#10;1/2T&#10;1/4&#10;1/4T&#10;1/8&#10;1/8T&#10;1/16&#10;1/16T&#10;1/32"
            textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <SLIDER name="PitchSlider" id="4f81ab45b18aa5d8" memberName="sPitch"
          virtualName="" explicitFocusOrder="0" pos="110 79 230M 20" tooltip="Pitch-Shift"
          min="-12" max="12" int="0.01" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="PostPitchButton" id="69444009b8d90dd7" memberName="tbPostPitch"
                virtualName="" explicitFocusOrder="0" pos="110R 104 100 20" tooltip="When checked, pitch is applied only before feedback loop. Otherwise the feedback will be pitched to. Pitching inside the feedback results in a minimum delay time."
                buttonText="Pre-Feedback" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <SLIDER name="FeedbackSlider" id="8a1d019dd2ee110e" memberName="sFeedback"
          virtualName="" explicitFocusOrder="0" pos="110 104 230M 20" tooltip="Delay feedback"
          min="0" max="100" int="0.1" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="bb4ca75073b47191" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="5 104 100 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Feedback [%]" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <SLIDER name="FrequencySlider" id="6df9989dcc0448f2" memberName="sFreq"
          virtualName="" explicitFocusOrder="0" pos="110 144 250M 20" tooltip="Filter in feedback loop frequency"
          min="20" max="20000" int="0.1" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="0.2"/>
  <LABEL name="new label" id="42a436310bac269c" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="5 144 100 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Frequency [Hz]" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <TOGGLEBUTTON name="SemitonesButton" id="72b923a08fbb4de8" memberName="tbSemitones"
                virtualName="" explicitFocusOrder="0" pos="110R 79 100 20" tooltip="Pitch only in semitones"
                buttonText="Semitones" connectedEdges="0" needsCallback="1" radioGroupId="0"
                state="0"/>
  <SLIDER name="QSlider" id="2b1bb8d3200587d7" memberName="sQfactor" virtualName=""
          explicitFocusOrder="0" pos="110 169 250M 20" tooltip="Filter in feedback loop Q-factor"
          min="0.3" max="10" int="0.01" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="0.3"/>
  <LABEL name="new label" id="560dc0f41a0d4db6" memberName="label5" virtualName=""
         explicitFocusOrder="0" pos="5 169 100 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Q factor" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="FilterTypeComboBox" id="dc2f5f9efc6074ec" memberName="cbFilter"
            virtualName="" explicitFocusOrder="0" pos="130R 169 120 20" tooltip="Filter Type. Maximum filter gain is limited to 0 dB to avoid feedback oscillation."
            editable="0" layout="33" items="Off&#10;Lowpass&#10;Highpass&#10;Lowshelf&#10;Highshelf&#10;Bell&#10;Bandpass&#10;Notch"
            textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="c7401e46e71bdc1d" memberName="label6" virtualName=""
         explicitFocusOrder="0" pos="130R 144 120 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Filter Type" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="GainSlider" id="50aec1bc77928637" memberName="sGain" virtualName=""
          explicitFocusOrder="0" pos="110 194 250M 20" tooltip="Filter in feedback loop gain (where available)"
          min="-24" max="24" int="0.1" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="4ec97e73ed4eee4f" memberName="label7" virtualName=""
         explicitFocusOrder="0" pos="5 194 100 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Gain [dB]" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TOGGLEBUTTON name="EnabledButton" id="9210e2d468ca8aa7" memberName="tbEnable"
                virtualName="" explicitFocusOrder="0" pos="10 2 100 20" buttonText="Enabled"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="VolumeSlider" id="49c1442c53da8c6" memberName="sVolume"
          virtualName="" explicitFocusOrder="0" pos="390R 264 380 20" tooltip="Delay tab volume"
          min="-60" max="0" int="0.1" style="LinearHorizontal" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="96d82c82b9fd34ab" memberName="label8" virtualName=""
         explicitFocusOrder="0" pos="60R 244 50 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Volume" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="new combo box" id="a722c7ed3566cad" memberName="cbPitch"
            virtualName="" explicitFocusOrder="0" pos="5 79 100 20" tooltip="Pitch shfiting algorithm. Off Disables pitching and saves resources."
            editable="0" layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="MonoTogglebutton" id="e91f6a5333e004fa" memberName="tbMono"
                virtualName="" explicitFocusOrder="0" pos="10 224 100 20" tooltip="Mono (mid) delay"
                buttonText="Mono" connectedEdges="0" needsCallback="1" radioGroupId="5000"
                state="1"/>
  <TOGGLEBUTTON name="StereoTogglebutton" id="698c74213e7fdc29" memberName="tbStereo"
                virtualName="" explicitFocusOrder="0" pos="10 244 100 20" tooltip="Stereo delay"
                buttonText="Stereo" connectedEdges="0" needsCallback="1" radioGroupId="5000"
                state="0"/>
  <TOGGLEBUTTON name="PingpongTogglebutton" id="bd9a88eca85f4957" memberName="tbPingpong"
                virtualName="" explicitFocusOrder="0" pos="10 264 100 20" tooltip="Stereo pingpong delay"
                buttonText="Pingpong" connectedEdges="0" needsCallback="1" radioGroupId="5000"
                state="0"/>
  <LABEL name="Pan/Balance Label" id="71b4933fb7d2eb20" memberName="lPan"
         virtualName="" explicitFocusOrder="0" pos="110 224 80 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Pan&#10;" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="34"/>
  <SLIDER name="pan slider" id="749823034ff76ead" memberName="sPan" virtualName=""
          explicitFocusOrder="0" pos="210 224 150 20" min="-100" max="100"
          int="1" style="LinearHorizontal" textBoxPos="TextBoxRight" textBoxEditable="1"
          textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="DelaySlider" id="a62b819222936a8e" memberName="sPreDelay"
          virtualName="" explicitFocusOrder="0" pos="110 29 230M 20" tooltip="Delay time"
          min="0" max="10" int="0.25" style="LinearBar" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="50" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="effc0bbc5c907681" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="5 29 100 20" edTextCol="ff000000"
         edBkgCol="0" labelText="Predelay" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <SLIDER name="new slider" id="a23eb1536c07d839" memberName="sPreVolume"
          virtualName="" explicitFocusOrder="0" pos="110R 29 100 20" min="-60"
          max="0" int="1" style="LinearHorizontal" textBoxPos="TextBoxLeft"
          textBoxEditable="1" textBoxWidth="30" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
