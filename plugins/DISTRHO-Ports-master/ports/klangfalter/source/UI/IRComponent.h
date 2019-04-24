/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  8 Mar 2013 10:37:54am

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_IRCOMPONENT_IRCOMPONENT_7F964DEC__
#define __JUCER_HEADER_IRCOMPONENT_IRCOMPONENT_7F964DEC__

//[Headers]     -- You can add your own extra header files here --
#include "../JuceHeader.h"

#include "WaveformComponent.h"
#include "../IRAgent.h"
#include "../Processor.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class IRComponent  : public Component,
                     public ChangeNotifier::Listener,
                     public ButtonListener,
                     public ComboBoxListener
{
public:
    //==============================================================================
    IRComponent ();
    ~IRComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    void init(IRAgent* irAgent);
    void irChanged();
    virtual void changeNotification();

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    IRAgent* _irAgent;
    //[/UserVariables]

    //==============================================================================
    WaveformComponent* _waveformComponent;
    TextButton* _loadButton;
    TextButton* _clearButton;
    ComboBox* _channelComboBox;
    Label* _channelHeaderLabel;


    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    IRComponent (const IRComponent&);
    const IRComponent& operator= (const IRComponent&);
};


#endif   // __JUCER_HEADER_IRCOMPONENT_IRCOMPONENT_7F964DEC__
