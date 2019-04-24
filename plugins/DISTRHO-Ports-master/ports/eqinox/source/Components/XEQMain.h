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

#ifndef __JUCER_HEADER_XEQMAIN_XEQMAIN_5791E0C2__
#define __JUCER_HEADER_XEQMAIN_XEQMAIN_5791E0C2__

//[Headers]     -- You can add your own extra header files here --
#include "../StandardHeader.h"
#include "../XEQPlugin.h"
#include "../Filters/jucetice_EQ.h"
#include "XEQGraph.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class XEQMain  : public Component,
                 public SliderListener
{
public:
    //==============================================================================
    XEQMain (XEQPlugin* plugin_);
    ~XEQMain() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void updateControls ();
    void updateScope ();
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    Equalizer* manager;
    XEQPlugin* plugin;
    //[/UserVariables]

    //==============================================================================
    EQGraph* eqgraph;
    ImageSlider* eq1Gain;
    ParameterSlider* eq1Bw;
    ParameterSlider* eq1Freq;
    ImageSlider* eq2Gain;
    ParameterSlider* eq2Bw;
    ParameterSlider* eq2Freq;
    ImageSlider* eq3Gain;
    ParameterSlider* eq3Bw;
    ParameterSlider* eq3Freq;
    ImageSlider* eq4Gain;
    ParameterSlider* eq4Bw;
    ParameterSlider* eq4Freq;
    ImageSlider* eq5Gain;
    ParameterSlider* eq5Bw;
    ParameterSlider* eq5Freq;
    ImageSlider* eq6Gain;
    ParameterSlider* eq6Bw;
    ParameterSlider* eq6Freq;
    Label* label;
    ImageSlider* gainSlider;
    ImageSlider* drywetSlider;
    Label* label2;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    XEQMain (const XEQMain&);
    const XEQMain& operator= (const XEQMain&);
};


#endif   // __JUCER_HEADER_XEQMAIN_XEQMAIN_5791E0C2__
