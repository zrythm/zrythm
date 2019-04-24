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

#ifndef __JUCER_HEADER_KLANGFALTEREDITOR_KLANGFALTEREDITOR_F5E4498E__
#define __JUCER_HEADER_KLANGFALTEREDITOR_KLANGFALTEREDITOR_F5E4498E__

//[Headers]     -- You can add your own extra header files here --
#include "../JuceHeader.h"
#include "../BinaryData.h"

#include "CustomLookAndFeel.h"
#include "DecibelScale.h"
#include "IRBrowserComponent.h"
#include "IRComponent.h"
#include "LevelMeter.h"
#include "SettingsDialogComponent.h"
#include "../Processor.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class KlangFalterEditor  : public AudioProcessorEditor,
                           public ChangeNotifier::Listener,
                           public ChangeListener,
                           public Timer,
                           public SliderListener,
                           public ButtonListener
{
public:
    //==============================================================================
    KlangFalterEditor (Processor& processor);
    ~KlangFalterEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void updateUI();
    virtual void changeListenerCallback (ChangeBroadcaster* source);
    virtual void changeNotification();
    virtual void timerCallback();
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);



    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    CustomLookAndFeel _customLookAndFeel;
    Processor& _processor;
    juce::Component::SafePointer<juce::DialogWindow> _settingsDialogWindow;
    std::map<std::pair<size_t, size_t>, IRComponent*> _irComponents;
    //[/UserVariables]

    //==============================================================================
    DecibelScale* _decibelScaleDry;
    TabbedComponent* _irTabComponent;
    LevelMeter* _levelMeterDry;
    Label* _dryLevelLabel;
    Label* _wetLevelLabel;
    Slider* _drySlider;
    DecibelScale* _decibelScaleOut;
    Slider* _wetSlider;
    TextButton* _browseButton;
    IRBrowserComponent* _irBrowserComponent;
    TextButton* _settingsButton;
    TextButton* _wetButton;
    TextButton* _dryButton;
    TextButton* _autogainButton;
    TextButton* _reverseButton;
    Label* _hiFreqLabel;
    Label* _hiGainLabel;
    Label* _hiGainHeaderLabel;
    Label* _hiFreqHeaderLabel;
    Slider* _hiGainSlider;
    Slider* _hiFreqSlider;
    Label* _loFreqLabel;
    Label* _loGainLabel;
    Label* _loGainHeaderLabel;
    Label* _loFreqHeaderLabel;
    Slider* _loGainSlider;
    Slider* _loFreqSlider;
    LevelMeter* _levelMeterOut;
    TextButton* _levelMeterOutLabelButton;
    Label* _levelMeterDryLabel;
    TextButton* _lowEqButton;
    Label* _lowCutFreqLabel;
    Label* _lowCutFreqHeaderLabel;
    Slider* _lowCutFreqSlider;
    Label* _highCutFreqLabel;
    Label* _highCutFreqHeaderLabel;
    Slider* _highCutFreqSlider;
    TextButton* _highEqButton;
    Label* _attackShapeLabel;
    Label* _endLabel;
    Slider* _endSlider;
    Slider* _attackShapeSlider;
    Label* _decayShapeLabel;
    Label* _decayShapeHeaderLabel;
    Slider* _decayShapeSlider;
    Label* _attackShapeHeaderLabel;
    Label* _endHeaderLabel;
    Label* _beginLabel;
    Slider* _beginSlider;
    Label* _beginHeaderLabel;
    Label* _widthLabel;
    Label* _widthHeaderLabel;
    Slider* _widthSlider;
    Label* _predelayLabel;
    Label* _predelayHeaderLabel;
    Slider* _predelaySlider;
    Label* _stretchLabel;
    Label* _stretchHeaderLabel;
    Slider* _stretchSlider;
    Label* _attackHeaderLabel;
    Label* _attackLengthLabel;
    Slider* _attackLengthSlider;
    Label* _attackLengthHeaderLabel;
    Label* _decayHeaderLabel;
    Label* _impulseResponseHeaderLabel;
    Label* _stereoHeaderLabel;


    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    KlangFalterEditor (const KlangFalterEditor&);
    const KlangFalterEditor& operator= (const KlangFalterEditor&);
};


#endif   // __JUCER_HEADER_KLANGFALTEREDITOR_KLANGFALTEREDITOR_F5E4498E__
