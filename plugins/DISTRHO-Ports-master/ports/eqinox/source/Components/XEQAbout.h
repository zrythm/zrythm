/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  18 Jan 2010 2:04:58 pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_XEQABOUT_XEQABOUT_799806C1__
#define __JUCER_HEADER_XEQABOUT_XEQABOUT_799806C1__

//[Headers]     -- You can add your own extra header files here --
#include "../StandardHeader.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class XEQAbout  : public Component
{
public:
    //==============================================================================
    XEQAbout ();
    ~XEQAbout() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;

    // Binary resources:
    static const char* equalizer_png;
    static const int equalizer_pngSize;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    HyperlinkButton* anticoreLink;
    Image cachedImage_equalizer_png;

    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    XEQAbout (const XEQAbout&);
    const XEQAbout& operator= (const XEQAbout&);
};


#endif   // __JUCER_HEADER_XEQABOUT_XEQABOUT_799806C1__
