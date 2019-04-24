/*
  ==============================================================================

    ChannelButtonLookAndFeel.h
    Created: 10 Oct 2016 9:38:20pm
    Author:  bruce

  ==============================================================================
*/

#ifndef ChannelButtonLookAndFeel_H_INCLUDED
#define ChannelButtonLookAndFeel_H_INCLUDED

#include "JuceHeader.h"

class ChannelButtonLookAndFeel : public LookAndFeel_V3
{

public:
	ChannelButtonLookAndFeel();
	void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
		bool isMouseOverButton, bool isButtonDown);

};




#endif  // ChannelButtonLookAndFeel_H_INCLUDED
