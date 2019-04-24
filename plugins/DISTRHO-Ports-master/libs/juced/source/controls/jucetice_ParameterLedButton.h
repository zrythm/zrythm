/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_PARAMETERLEDBUTTON_HEADER__
#define __JUCETICE_PARAMETERLEDBUTTON_HEADER__

#include "../base/jucetice_AudioParameter.h"


//==============================================================================
/**
     A parameter listener toggle button
*/
class ParameterLedButton  : public TextButton,
                            public AudioParameterListener
{
public:

    //==============================================================================
    ParameterLedButton (const String& buttonText)
        : TextButton (buttonText),
          onValue (1.0f),
          offValue (0.0f),
          activated (false)
    {
        offColour = Colours::black;
        onColour = Colours::yellow;
        
        setColour (TextButton::buttonColourId, offColour);
    }

    //==============================================================================
    virtual ~ParameterLedButton()
    {
    }

    //==============================================================================
    bool isActivated () const
    {
        return activated;
    }

    void setActivated (const bool isActive)
    {
        activated = isActive;

        setColour (TextButton::buttonColourId, activated ? onColour : offColour);
    }
    
    void toggleActivated ()
    {
        setActivated (! activated);
    }

    //==============================================================================
    void setCurrentValues (const float offValue_, const float onValue_)
    {
        onValue = onValue_;
        offValue = offValue_;
    }

    float getActivateValue () const 
    {
        return onValue;
    }

    float getCurrentValue () const 
    {
        return activated ? onValue : offValue;
    }

    void setOnOffColours (const Colour& offColour_, const Colour& onColour_)
    {
        offColour = offColour_;
        onColour = onColour_;

        setColour (TextButton::buttonColourId, activated ? onColour : offColour);
    }

    //==============================================================================
    void parameterChanged (AudioParameter* parameter, const int index)
    {
        setActivated (onValue == parameter->getValueMapped ());
    }
    
protected:

    float onValue, offValue;
    bool activated;
    
    Colour offColour;
    Colour onColour;
};


#endif // __JUCETICE_PARAMETERLEDBUTTON_HEADER__
