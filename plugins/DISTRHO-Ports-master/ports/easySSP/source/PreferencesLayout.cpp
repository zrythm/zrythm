/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "TomatlLookAndFeel.h"
#include <limits>
//[/Headers]

#include "PreferencesLayout.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PreferencesLayout::PreferencesLayout (AdmvAudioProcessor* plugin)
    : mParentProcessor(plugin)
{
    addAndMakeVisible (mSpectroGroup = new GroupComponent ("new group",
                                                           TRANS("Spectrometer")));

    addAndMakeVisible (mGenericGroup = new GroupComponent ("new group",
                                                           TRANS("Generic")));

    addAndMakeVisible (mGonioGroup = new GroupComponent ("gonio group",
                                                         TRANS("Goniometer")));

    addAndMakeVisible (mGoniometerScaleModeBox = new ComboBox ("goniometer scale mode"));
    mGoniometerScaleModeBox->setEditableText (false);
    mGoniometerScaleModeBox->setJustificationType (Justification::centredLeft);
    mGoniometerScaleModeBox->setTextWhenNothingSelected (String());
    mGoniometerScaleModeBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    mGoniometerScaleModeBox->addItem (TRANS("Auto"), 1);
    mGoniometerScaleModeBox->addItem (TRANS("Manual"), 2);
    mGoniometerScaleModeBox->addListener (this);

    addAndMakeVisible (mSpectroReleaseSlider = new Slider ("spectro release slider"));
    mSpectroReleaseSlider->setRange (0, 1000, 0);
    mSpectroReleaseSlider->setSliderStyle (Slider::LinearHorizontal);
    mSpectroReleaseSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    mSpectroReleaseSlider->addListener (this);

    addAndMakeVisible (mSpectroReleaseLabel = new Label ("spectro release label",
                                                         TRANS("label text")));
    mSpectroReleaseLabel->setFont (Font (15.00f, Font::plain));
    mSpectroReleaseLabel->setJustificationType (Justification::centredLeft);
    mSpectroReleaseLabel->setEditable (false, false, false);
    mSpectroReleaseLabel->setColour (TextEditor::textColourId, Colours::black);
    mSpectroReleaseLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mSpectrumFillModeBox = new ComboBox ("spectrum fill mode box"));
    mSpectrumFillModeBox->setEditableText (false);
    mSpectrumFillModeBox->setJustificationType (Justification::centredLeft);
    mSpectrumFillModeBox->setTextWhenNothingSelected (String());
    mSpectrumFillModeBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    mSpectrumFillModeBox->addItem (TRANS("Semi-transparent"), 1);
    mSpectrumFillModeBox->addItem (TRANS("None"), 2);
    mSpectrumFillModeBox->addListener (this);

    addAndMakeVisible (mOutputModeBox = new ComboBox ("output mode box"));
    mOutputModeBox->setEditableText (false);
    mOutputModeBox->setJustificationType (Justification::centredLeft);
    mOutputModeBox->setTextWhenNothingSelected (String());
    mOutputModeBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    mOutputModeBox->addItem (TRANS("No change"), 1);
    mOutputModeBox->addItem (TRANS("Mute all"), 2);
    mOutputModeBox->addListener (this);

    addAndMakeVisible (mGonioScaleReleaseSlider = new Slider ("goniometer scale release"));
    mGonioScaleReleaseSlider->setRange (0, 1000, 0);
    mGonioScaleReleaseSlider->setSliderStyle (Slider::LinearHorizontal);
    mGonioScaleReleaseSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    mGonioScaleReleaseSlider->addListener (this);

    addAndMakeVisible (mGonioScaleReleaseLabel = new Label ("goniometer scale release label",
                                                            TRANS("label text")));
    mGonioScaleReleaseLabel->setFont (Font (15.00f, Font::plain));
    mGonioScaleReleaseLabel->setJustificationType (Justification::centredLeft);
    mGonioScaleReleaseLabel->setEditable (false, false, false);
    mGonioScaleReleaseLabel->setColour (TextEditor::textColourId, Colours::black);
    mGonioScaleReleaseLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mGonioOptionLabel1 = new Label ("new label",
                                                       TRANS("Scale mode:")));
    mGonioOptionLabel1->setFont (Font (15.00f, Font::plain));
    mGonioOptionLabel1->setJustificationType (Justification::centredLeft);
    mGonioOptionLabel1->setEditable (false, false, false);
    mGonioOptionLabel1->setColour (TextEditor::textColourId, Colours::black);
    mGonioOptionLabel1->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mGonioOptionLabel2 = new Label ("new label",
                                                       TRANS("Auto-scale release (ms):")));
    mGonioOptionLabel2->setFont (Font (15.00f, Font::plain));
    mGonioOptionLabel2->setJustificationType (Justification::centredLeft);
    mGonioOptionLabel2->setEditable (false, false, false);
    mGonioOptionLabel2->setColour (TextEditor::textColourId, Colours::black);
    mGonioOptionLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mSpectroOptionLabel1 = new Label ("new label",
                                                         TRANS("Spectrum fill:")));
    mSpectroOptionLabel1->setFont (Font (15.00f, Font::plain));
    mSpectroOptionLabel1->setJustificationType (Justification::centredLeft);
    mSpectroOptionLabel1->setEditable (false, false, false);
    mSpectroOptionLabel1->setColour (TextEditor::textColourId, Colours::black);
    mSpectroOptionLabel1->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mSpectroOptionLabel2 = new Label ("new label",
                                                         TRANS("Spectrum release speed (ms):")));
    mSpectroOptionLabel2->setFont (Font (15.00f, Font::plain));
    mSpectroOptionLabel2->setJustificationType (Justification::centredLeft);
    mSpectroOptionLabel2->setEditable (false, false, false);
    mSpectroOptionLabel2->setColour (TextEditor::textColourId, Colours::black);
    mSpectroOptionLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mGenericOptionLabel1 = new Label ("new label",
                                                         TRANS("Audio output:")));
    mGenericOptionLabel1->setFont (Font (15.00f, Font::plain));
    mGenericOptionLabel1->setJustificationType (Justification::centredLeft);
    mGenericOptionLabel1->setEditable (false, false, false);
    mGenericOptionLabel1->setColour (TextEditor::textColourId, Colours::black);
    mGenericOptionLabel1->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]
	mSpectroReleaseBounds.mLow = 50;
	mSpectroReleaseBounds.mHigh = 10000;
	mGonioScaleReleaseBounds.mLow = 50;
	mGonioScaleReleaseBounds.mHigh = 10000;

	mGoniometerScaleModeBox->setSelectedItemIndex(mParentProcessor->getState().mManualGoniometerScale ? 1 : 0);

	mSpectrumFillModeBox->setSelectedItemIndex(mParentProcessor->getState().mSpectrumFillMode == AdmvPluginState::spectrumFillNone ? 1 : 0);

	mOutputModeBox->setSelectedItemIndex(mParentProcessor->getState().mOutputMode == AdmvPluginState::outputMute ? 1 : 0);

	double spectroReleaseSpeed = mParentProcessor->getState().mSpectrometerReleaseSpeed;

	if (spectroReleaseSpeed != std::numeric_limits<double>::infinity())
	{
		mSpectroReleaseSlider->setValue(mSpectroReleaseScale.scale(mSpectroReleaseSlider->getMaximum(), mSpectroReleaseBounds, spectroReleaseSpeed));
	}
	else
	{
		mSpectroReleaseSlider->setValue(mSpectroReleaseSlider->getMaximum());
	}

	setSpectroReleaseLabelText(spectroReleaseSpeed);

	double gonioReleaseSpeed = mParentProcessor->getState().mGoniometerScaleAttackRelease.second;

	mGonioScaleReleaseSlider->setValue(mGonioScaleReleaseScale.scale(mGonioScaleReleaseSlider->getMaximum(), mGonioScaleReleaseBounds, gonioReleaseSpeed));

	mGonioScaleReleaseLabel->setText(juce::String((int)gonioReleaseSpeed), juce::dontSendNotification);

    //[/UserPreSize]

    setSize (528, 256);


    //[Constructor] You can add your own custom stuff here..

    //[/Constructor]
}

PreferencesLayout::~PreferencesLayout()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    mSpectroGroup = nullptr;
    mGenericGroup = nullptr;
    mGonioGroup = nullptr;
    mGoniometerScaleModeBox = nullptr;
    mSpectroReleaseSlider = nullptr;
    mSpectroReleaseLabel = nullptr;
    mSpectrumFillModeBox = nullptr;
    mOutputModeBox = nullptr;
    mGonioScaleReleaseSlider = nullptr;
    mGonioScaleReleaseLabel = nullptr;
    mGonioOptionLabel1 = nullptr;
    mGonioOptionLabel2 = nullptr;
    mSpectroOptionLabel1 = nullptr;
    mSpectroOptionLabel2 = nullptr;
    mGenericOptionLabel1 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PreferencesLayout::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::white);

    //[UserPaint] Add your own custom painting code here..
	g.fillAll(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::defaultBackground));
    //[/UserPaint]
}

void PreferencesLayout::resized()
{
    mSpectroGroup->setBounds (0, 96, 528, 96);
    mGenericGroup->setBounds (0, 192, 528, 64);
    mGonioGroup->setBounds (0, 0, 528, 96);
    mGoniometerScaleModeBox->setBounds (264, 24, 150, 24);
    mSpectroReleaseSlider->setBounds (264, 152, 150, 24);
    mSpectroReleaseLabel->setBounds (416, 152, 88, 24);
    mSpectrumFillModeBox->setBounds (264, 120, 150, 24);
    mOutputModeBox->setBounds (264, 216, 150, 24);
    mGonioScaleReleaseSlider->setBounds (264, 56, 150, 24);
    mGonioScaleReleaseLabel->setBounds (416, 56, 72, 24);
    mGonioOptionLabel1->setBounds (16, 24, 168, 24);
    mGonioOptionLabel2->setBounds (16, 56, 239, 24);
    mSpectroOptionLabel1->setBounds (16, 120, 150, 24);
    mSpectroOptionLabel2->setBounds (16, 152, 207, 24);
    mGenericOptionLabel1->setBounds (16, 216, 150, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PreferencesLayout::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == mGoniometerScaleModeBox)
    {
        //[UserComboBoxCode_mGoniometerScaleModeBox] -- add your combo box handling code here..
		mParentProcessor->setManualGonioScaleEnabled(comboBoxThatHasChanged->getSelectedItemIndex() == 1);
        //[/UserComboBoxCode_mGoniometerScaleModeBox]
    }
    else if (comboBoxThatHasChanged == mSpectrumFillModeBox)
    {
        //[UserComboBoxCode_mSpectrumFillModeBox] -- add your combo box handling code here..
		mParentProcessor->setSpectroFillMode(comboBoxThatHasChanged->getSelectedItemIndex() == 0 ? AdmvPluginState::spectrumFillWithTransparency : AdmvPluginState::spectrumFillNone);
        //[/UserComboBoxCode_mSpectrumFillModeBox]
    }
    else if (comboBoxThatHasChanged == mOutputModeBox)
    {
        //[UserComboBoxCode_mOutputModeBox] -- add your combo box handling code here..
		mParentProcessor->setOutputMode(comboBoxThatHasChanged->getSelectedItemIndex() == 0 ? AdmvPluginState::outputNoChange : AdmvPluginState::outputMute);
        //[/UserComboBoxCode_mOutputModeBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void PreferencesLayout::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == mSpectroReleaseSlider)
    {
        //[UserSliderCode_mSpectroReleaseSlider] -- add your slider handling code here..
		auto releaseSpeed = mSpectroReleaseScale.unscale(sliderThatWasMoved->getMaximum(), mSpectroReleaseBounds, sliderThatWasMoved->getValue());
		mParentProcessor->setSpectroReleaseSpeed(releaseSpeed == mSpectroReleaseBounds.mHigh ? std::numeric_limits<double>::infinity() : releaseSpeed);
		setSpectroReleaseLabelText(releaseSpeed);
        //[/UserSliderCode_mSpectroReleaseSlider]
    }
    else if (sliderThatWasMoved == mGonioScaleReleaseSlider)
    {
        //[UserSliderCode_mGonioScaleReleaseSlider] -- add your slider handling code here..
		auto releaseSpeed = mGonioScaleReleaseScale.unscale(sliderThatWasMoved->getMaximum(), mGonioScaleReleaseBounds, sliderThatWasMoved->getValue());
		mParentProcessor->setManualGonioScaleRelease(releaseSpeed);
		mGonioScaleReleaseLabel->setText(juce::String((int)releaseSpeed), juce::dontSendNotification);
        //[/UserSliderCode_mGonioScaleReleaseSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PreferencesLayout" componentName=""
                 parentClasses="public Component" constructorParams="AdmvAudioProcessor* plugin"
                 variableInitialisers="mParentProcessor(plugin)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="528" initialHeight="256">
  <BACKGROUND backgroundColour="ffffffff"/>
  <GROUPCOMPONENT name="new group" id="f24f8e0b8ec35751" memberName="mSpectroGroup"
                  virtualName="" explicitFocusOrder="0" pos="0 96 528 96" title="Spectrometer"/>
  <GROUPCOMPONENT name="new group" id="3a93324f56884b80" memberName="mGenericGroup"
                  virtualName="" explicitFocusOrder="0" pos="0 192 528 64" title="Generic"/>
  <GROUPCOMPONENT name="gonio group" id="29e358bd05278b4" memberName="mGonioGroup"
                  virtualName="" explicitFocusOrder="0" pos="0 0 528 96" title="Goniometer"/>
  <COMBOBOX name="goniometer scale mode" id="a1dd39452c7f167b" memberName="mGoniometerScaleModeBox"
            virtualName="" explicitFocusOrder="0" pos="264 24 150 24" editable="0"
            layout="33" items="Auto&#10;Manual" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <SLIDER name="spectro release slider" id="80c6c9a19158f9fd" memberName="mSpectroReleaseSlider"
          virtualName="" explicitFocusOrder="0" pos="264 152 150 24" min="0"
          max="1000" int="0" style="LinearHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="spectro release label" id="cd214626a42882bc" memberName="mSpectroReleaseLabel"
         virtualName="" explicitFocusOrder="0" pos="416 152 88 24" edTextCol="ff000000"
         edBkgCol="0" labelText="label text" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="spectrum fill mode box" id="c3a176fc7e06cc2f" memberName="mSpectrumFillModeBox"
            virtualName="" explicitFocusOrder="0" pos="264 120 150 24" editable="0"
            layout="33" items="Semi-transparent&#10;None" textWhenNonSelected=""
            textWhenNoItems="(no choices)"/>
  <COMBOBOX name="output mode box" id="582acc21501a5672" memberName="mOutputModeBox"
            virtualName="" explicitFocusOrder="0" pos="264 216 150 24" editable="0"
            layout="33" items="No change&#10;Mute all" textWhenNonSelected=""
            textWhenNoItems="(no choices)"/>
  <SLIDER name="goniometer scale release" id="32283029e35fd491" memberName="mGonioScaleReleaseSlider"
          virtualName="" explicitFocusOrder="0" pos="264 56 150 24" min="0"
          max="1000" int="0" style="LinearHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="goniometer scale release label" id="1da2f804f295b7f4" memberName="mGonioScaleReleaseLabel"
         virtualName="" explicitFocusOrder="0" pos="416 56 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="label text" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5e1d40a82f1005ba" memberName="mGonioOptionLabel1"
         virtualName="" explicitFocusOrder="0" pos="16 24 168 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Scale mode:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5198253911601231" memberName="mGonioOptionLabel2"
         virtualName="" explicitFocusOrder="0" pos="16 56 239 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Auto-scale release (ms):" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="d04b6c868f590aa8" memberName="mSpectroOptionLabel1"
         virtualName="" explicitFocusOrder="0" pos="16 120 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Spectrum fill:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="d45450f0081a7e37" memberName="mSpectroOptionLabel2"
         virtualName="" explicitFocusOrder="0" pos="16 152 207 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Spectrum release speed (ms):" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="2619bcfb069a93ef" memberName="mGenericOptionLabel1"
         virtualName="" explicitFocusOrder="0" pos="16 216 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Audio output:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
