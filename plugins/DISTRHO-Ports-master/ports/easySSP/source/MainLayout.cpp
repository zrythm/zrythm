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
//[/Headers]

#include "MainLayout.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
#include "PluginProcessor.h"
//[/MiscUserDefs]

//==============================================================================
MainLayout::MainLayout (AdmvAudioProcessor* plugin)
{
    addAndMakeVisible (mGonioScaleValue = new Slider ("Gonio Scale Value"));
    mGonioScaleValue->setRange (-72, 0, 0);
    mGonioScaleValue->setSliderStyle (Slider::LinearVertical);
    mGonioScaleValue->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    mGonioScaleValue->addListener (this);

    addAndMakeVisible (mSpectroMagnitudeScale = new Slider ("Spectrum Magnitude Scale"));
    mSpectroMagnitudeScale->setRange (-72, 0, 0);
    mSpectroMagnitudeScale->setSliderStyle (Slider::TwoValueVertical);
    mSpectroMagnitudeScale->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    mSpectroMagnitudeScale->addListener (this);

    addAndMakeVisible (mGonioPlaceholder = new Label ("Goniometer",
                                                      TRANS("Goniometer\n")));
    mGonioPlaceholder->setFont (Font (15.00f, Font::plain));
    mGonioPlaceholder->setJustificationType (Justification::centred);
    mGonioPlaceholder->setEditable (false, false, false);
    mGonioPlaceholder->setColour (Label::backgroundColourId, Colours::cadetblue);
    mGonioPlaceholder->setColour (TextEditor::textColourId, Colours::black);
    mGonioPlaceholder->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mSpectroPlaceholder = new Label ("Spectrometer",
                                                        TRANS("Spectrometer\n")));
    mSpectroPlaceholder->setFont (Font (15.00f, Font::plain));
    mSpectroPlaceholder->setJustificationType (Justification::centred);
    mSpectroPlaceholder->setEditable (false, false, false);
    mSpectroPlaceholder->setColour (Label::backgroundColourId, Colours::grey);
    mSpectroPlaceholder->setColour (TextEditor::textColourId, Colours::black);
    mSpectroPlaceholder->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mSpectroFreqScale = new Slider ("Spectrum Frequency Scale"));
    mSpectroFreqScale->setRange (20, 30000, 0);
    mSpectroFreqScale->setSliderStyle (Slider::TwoValueHorizontal);
    mSpectroFreqScale->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    mSpectroFreqScale->addListener (this);

    addAndMakeVisible (mAboutButton = new TextButton ("about button"));
    mAboutButton->setTooltip (TRANS("Help"));
    mAboutButton->setButtonText (TRANS("?"));
    mAboutButton->addListener (this);

    addAndMakeVisible (mOptionsBtn = new TextButton ("options button"));
    mOptionsBtn->setButtonText (TRANS("Options"));
    mOptionsBtn->addListener (this);


    //[UserPreSize]
	mParentProcessor = plugin;

	// This hack unsets label colours assigned by Introjucer, as there is no way to avoid these colors automatic generation
	for (int i = 0; i < getNumChildComponents(); ++i)
	{
		Component* comp = getChildComponent(i);

		Label* label = NULL;
		label = dynamic_cast<Label*>(comp);

		if (label != NULL)
		{
			label->removeColour(TextEditor::textColourId);
			label->removeColour(TextEditor::backgroundColourId);
		}
	}

	mOptionsBtn->setVisible(true);

    //[/UserPreSize]

    setSize (991, 415);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

MainLayout::~MainLayout()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    mGonioScaleValue = nullptr;
    mSpectroMagnitudeScale = nullptr;
    mGonioPlaceholder = nullptr;
    mSpectroPlaceholder = nullptr;
    mSpectroFreqScale = nullptr;
    mAboutButton = nullptr;
    mOptionsBtn = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MainLayout::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff1e1e1e));

    //[UserPaint] Add your own custom painting code here..
	g.fillAll(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::defaultBackground));
    //[/UserPaint]
}

void MainLayout::resized()
{
    mGonioScaleValue->setBounds (351, 9, 32, 342);
    mSpectroMagnitudeScale->setBounds (955, 9, 32, 342);
    mGonioPlaceholder->setBounds (9, 9, 342, 342);
    mSpectroPlaceholder->setBounds (383, 9, 568, 342);
    mSpectroFreqScale->setBounds (383, 355, 568, 24);
    mAboutButton->setBounds (960, 384, 24, 24);
    mOptionsBtn->setBounds (864, 384, 86, 24);
    //[UserResized] Add your own custom resize handling here..
	mSpectroFreqScale->setRange(0, 100); // simple percentage here, all scaling is handling by the client
	mSpectroMagnitudeScale->setRange(0, 100);
    //[/UserResized]
}

void MainLayout::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == mGonioScaleValue)
    {
        //[UserSliderCode_mGonioScaleValue] -- add your slider handling code here..
		mParentProcessor->setManualGonioScaleValue(TOMATL_FROM_DB(sliderThatWasMoved->getValue()));
        //[/UserSliderCode_mGonioScaleValue]
    }
    else if (sliderThatWasMoved == mSpectroMagnitudeScale)
    {
        //[UserSliderCode_mSpectroMagnitudeScale] -- add your slider handling code here..
		mParentProcessor->setSpectroMagnitudeScale(std::pair<double, double>(sliderThatWasMoved->getMinValue(), sliderThatWasMoved->getMaxValue()));
        //[/UserSliderCode_mSpectroMagnitudeScale]
    }
    else if (sliderThatWasMoved == mSpectroFreqScale)
    {
        //[UserSliderCode_mSpectroFreqScale] -- add your slider handling code here..
		mParentProcessor->setSpectroFrequencyScale(std::pair<double, double>(sliderThatWasMoved->getMinValue(), sliderThatWasMoved->getMaxValue()));
        //[/UserSliderCode_mSpectroFreqScale]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void MainLayout::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == mAboutButton)
    {
        //[UserButtonCode_mAboutButton] -- add your button handler code here..
		showAboutDialog();
        //[/UserButtonCode_mAboutButton]
    }
    else if (buttonThatWasClicked == mOptionsBtn)
    {
        //[UserButtonCode_mOptionsBtn] -- add your button handler code here..
		showPreferencesDialog();
        //[/UserButtonCode_mOptionsBtn]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MainLayout" componentName=""
                 parentClasses="public Component" constructorParams="AdmvAudioProcessor* plugin"
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="1" initialWidth="991" initialHeight="415">
  <BACKGROUND backgroundColour="ff1e1e1e"/>
  <SLIDER name="Gonio Scale Value" id="759c99b88517019b" memberName="mGonioScaleValue"
          virtualName="" explicitFocusOrder="0" pos="351 9 32 342" min="-72"
          max="0" int="0" style="LinearVertical" textBoxPos="NoTextBox"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Spectrum Magnitude Scale" id="c469fe133993978c" memberName="mSpectroMagnitudeScale"
          virtualName="" explicitFocusOrder="0" pos="955 9 32 342" min="-72"
          max="0" int="0" style="TwoValueVertical" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="Goniometer" id="cc50c59b667e6fe0" memberName="mGonioPlaceholder"
         virtualName="" explicitFocusOrder="0" pos="9 9 342 342" bkgCol="ff5f9ea0"
         edTextCol="ff000000" edBkgCol="0" labelText="Goniometer&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="Spectrometer" id="ec29c7cd27f78cb9" memberName="mSpectroPlaceholder"
         virtualName="" explicitFocusOrder="0" pos="383 9 568 342" bkgCol="ff808080"
         edTextCol="ff000000" edBkgCol="0" labelText="Spectrometer&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="36"/>
  <SLIDER name="Spectrum Frequency Scale" id="fea54c0326f58da2" memberName="mSpectroFreqScale"
          virtualName="" explicitFocusOrder="0" pos="383 355 568 24" min="20"
          max="30000" int="0" style="TwoValueHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TEXTBUTTON name="about button" id="268c0f6cf9c85fec" memberName="mAboutButton"
              virtualName="" explicitFocusOrder="0" pos="960 384 24 24" tooltip="Help"
              buttonText="?" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="options button" id="a11b265cc1198ab6" memberName="mOptionsBtn"
              virtualName="" explicitFocusOrder="0" pos="864 384 86 24" buttonText="Options"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
