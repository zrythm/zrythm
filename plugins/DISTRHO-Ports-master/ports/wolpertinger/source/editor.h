/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  9 Nov 2010 4:40:41pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_EDITOR_EDITOR_30B2A45D__
#define __JUCER_HEADER_EDITOR_EDITOR_30B2A45D__

//[Headers]     -- You can add your own extra header files here --
#include "juce_PluginHeaders.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class editor  : public AudioProcessorEditor,
                public ChangeListener,
                public SliderListener,
                public ComboBoxListener
{
public:
    //==============================================================================
    editor (AudioProcessor *const ownerFilter);
    ~editor() override;

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void changeListenerCallback(ChangeBroadcaster *objectThatHasChanged) override;

    // Binary resources:
    static const char* scratches_png;
    static const int scratches_pngSize;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    GroupComponent* groupComponent2;
    GroupComponent* groupComponent;
    Label* label;
    Label* label2;
    Slider* slgain;
    Slider* slclip;
    Slider* slsaw;
    Label* label3;
    Slider* slrect;
    Label* label4;
    Slider* sltri;
    Label* label5;
    Slider* sltune;
    Label* label6;
    GroupComponent* groupComponent3;
    Label* label7;
    Slider* slcutoff;
    Label* label8;
    Slider* slreso;
    Label* label9;
    Slider* slbandwidth;
    Label* label10;
    Slider* slpasses;
    Label* label11;
    Slider* slvelocity;
    Label* label12;
    Slider* slinertia;
    Label* label13;
    Slider* slcurcutoff;
    Label* label14;
    Label* label15;
    Label* label19;
    Label* label20;
    Label* label16;
    Label* label17;
    Label* label18;
    Slider* slattack5;
    Slider* slattack6;
    Label* label21;
    Label* label22;
    Slider* slattack7;
    Slider* knobAttack;
    Slider* knobDecay;
    Slider* knobSustain;
    Slider* knobRelease;
    Slider* slfilterlimits;
    Label* label23;
    ComboBox* oversmplComboBox;
    Label* label25;
    Image cachedImage_scratches_png;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    editor (const editor&);
    const editor& operator= (const editor&);
};


#endif   // __JUCER_HEADER_EDITOR_EDITOR_30B2A45D__
