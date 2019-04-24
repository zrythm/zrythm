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

#ifndef __JUCER_HEADER_DRUMSYNTHMAIN_DRUMSYNTHMAIN_FA1801A0__
#define __JUCER_HEADER_DRUMSYNTHMAIN_DRUMSYNTHMAIN_FA1801A0__

//[Headers]     -- You can add your own extra header files here --
#include "../StandardHeader.h"
#include "DrumSynthEnvelope.h"
#include "DrumSynthKeyboard.h"

class DrumSynthPlugin;
class DrumSynthComponent;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class DrumSynthMain  : public Component,
                       public TextEditorListener,
                       public ButtonListener,
                       public SliderListener
{
public:
    //==============================================================================
    DrumSynthMain (DrumSynthPlugin* plugin_, DrumSynthComponent* editor_);
    ~DrumSynthMain() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setCurrentDrumNumber (const int newDrumNumber,
                               const bool updateAllControls);
    void updateControls ();

    void textEditorTextChanged (TextEditor &editor) override;
    void textEditorReturnKeyPressed (TextEditor &editor) override;
    void textEditorEscapeKeyPressed (TextEditor &editor) override;
    void textEditorFocusLost (TextEditor &editor) override;
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    DrumSynthPlugin* plugin;
    DrumSynthComponent* editor;
    //[/UserVariables]

    //==============================================================================
    TextButton* importButton;
    TextButton* exportButton;
    Slider* currentDrum;
    DrumSynthKeyboard* midiKeyboard;
    Label* labelDrum;
    Label* labelDrumName;
    TextEditor* drumName;
    Label* versionLabel;
    ParameterSlider* tuningSlider;
    ParameterSlider* stretchSlider;
    ParameterSlider* gainSlider;
    ParameterSlider* resonanceSlider;
    ParameterLedButton* filterButton;
    ParameterLedButton* hipassButton;
    TabbedComponent* envelopeTabs;
    TextButton* panicButton;
    ParameterSlider* toneLevelSlider;
    ParameterSlider* toneFreq1Slider;
    ParameterSlider* toneFreq2Slider;
    ParameterSlider* toneDroopSlider;
    ParameterSlider* tonePhaseSlider;
    ParameterLedButton* toneOnButton;
    ParameterLedButton* overOnButton;
    ParameterSlider* overLevelSlider;
    ParameterSlider* overParamSlider;
    ParameterLedButton* overFilterButton;
    ParameterLedButton* overMethod1Button;
    ParameterLedButton* overMethod2Button;
    ParameterLedButton* overMethod3Button;
    ParameterSlider* overFreq1Slider;
    ParameterSlider* overFreq2Slider;
    ParameterLedButton* overTrack1Button;
    ParameterLedButton* overTrack2Button;
    ParameterSlider* overWave1Slider;
    ParameterSlider* overWave2Slider;
    ParameterSlider* band1LevelSlider;
    ParameterSlider* band1FreqSlider;
    ParameterSlider* band1DeltaSlider;
    ParameterLedButton* band1OnButton;
    ParameterLedButton* band2OnButton;
    ParameterSlider* band2LevelSlider;
    ParameterSlider* band2FreqSlider;
    ParameterSlider* band2DeltaSlider;
    ParameterLedButton* noiseOnButton;
    ParameterSlider* noiseLevelSlider;
    ParameterSlider* noiseSlopeSlider;
    TextButton* importBankButton;
    TextButton* exportBankButton;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    DrumSynthMain (const DrumSynthMain&);
    const DrumSynthMain& operator= (const DrumSynthMain&);
};


#endif   // __JUCER_HEADER_DRUMSYNTHMAIN_DRUMSYNTHMAIN_FA1801A0__
