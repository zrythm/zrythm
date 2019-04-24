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

#ifndef __JUCETICE_PARAMETERSLIDER_HEADER__
#define __JUCETICE_PARAMETERSLIDER_HEADER__

#include "../base/jucetice_AudioParameter.h"


//==============================================================================
/**
     A parameter listener slider
*/
class ParameterSlider  : public Slider,
                         public AudioParameterListener
{
public:

    //==============================================================================
    ParameterSlider (const String& sliderName)
        : Slider (sliderName),
          parameter (0)
    {
    }

    //==============================================================================
    virtual ~ParameterSlider()
    {
    }

    //==============================================================================
    void parameterChanged (AudioParameter* newParameter, const int index)
    {
        setValue (newParameter->getValueMapped (), dontSendNotification);
    }

    void attachedToParameter (AudioParameter* newParameter, const int index)
    {
        parameter = newParameter;
    }

    void detachedFromParameter (AudioParameter* newParameter, const int index)
    {
        if (newParameter == parameter)
            parameter = 0;
    }

    //==============================================================================
    void mouseDown (const MouseEvent& e)
    {
        if (parameter && e.mods.isRightButtonDown ())
        {
            parameter->handleMidiPopupMenu (e);

            return;
        }
        
        Slider::mouseDown (e);
    }

protected:

    AudioParameter* parameter;
};


#endif // __JUCETICE_PARAMETERSLIDER_HEADER__
