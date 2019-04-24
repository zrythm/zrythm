/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  13 Nov 2010 7:18:26pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_ABOUT_ABOUT_B549DFBD__
#define __JUCER_HEADER_ABOUT_ABOUT_B549DFBD__

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
class about  : public Component
{
public:
    //==============================================================================
    about ();
    ~about() override;

    void paint (Graphics& g) override;
    void resized() override;

    // Binary resources:
    static const char* logo_png;
    static const int logo_pngSize;
    static const char* icon_png;
    static const int icon_pngSize;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    HyperlinkButton* hyperlinkButton;
    Label* label;
    Label* label2;
    Image cachedImage_logo_png;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    about (const about&);
    const about& operator= (const about&);
};


#endif   // __JUCER_HEADER_ABOUT_ABOUT_B549DFBD__
