/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  13 Nov 2010 7:26:58pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_TABBEDEDITOR_TABBEDEDITOR_9EE00E72__
#define __JUCER_HEADER_TABBEDEDITOR_TABBEDEDITOR_9EE00E72__

//[Headers]     -- You can add your own extra header files here --
#include "juce_PluginHeaders.h"
#include "PresetComboBox.h"
#include "KeyboardButton.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class tabbed_editor  : public AudioProcessorEditor,
                       public ComboBoxListener
{
public:
    //==============================================================================
    tabbed_editor (AudioProcessor *const ownerFilter);
    ~tabbed_editor() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setPolyText(String text) { polytext->setText(text, dontSendNotification); }
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;

    // Binary resources:
    static const char* scratches_png;
    static const int scratches_pngSize;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    MidiKeyboardState midi_keyboard_state;
    KeyboardButton *kbd_button;
    //[/UserVariables]

    //==============================================================================
    MidiKeyboardComponent* midi_keyboard;
    TabbedComponent* tabbedEditor;
    PresetComboBox* comboBox;
    Label* polytext;
    Path internalPath1;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    tabbed_editor (const tabbed_editor&);
    const tabbed_editor& operator= (const tabbed_editor&);
};


#endif   // __JUCER_HEADER_TABBEDEDITOR_TABBEDEDITOR_9EE00E72__
