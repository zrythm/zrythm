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

#ifndef __JUCETICE_PARAMETERCOMBOBOX_HEADER__
#define __JUCETICE_PARAMETERCOMBOBOX_HEADER__

#include "../base/jucetice_AudioParameter.h"


//==============================================================================
/**
     A parameter listener combo box button
*/
class ParameterComboBox  : public ComboBox,
                           public AudioParameterListener
{
public:

    //==============================================================================
    ParameterComboBox (const String& componentName)
        : ComboBox (componentName)
    {
    }

    //==============================================================================
    virtual ~ParameterComboBox()
    {
    }

    //==============================================================================
    void parameterChanged (AudioParameter* parameter, const int index)
    {
#if 1
        int newItemSelected = parameter->getIntValueMapped ();

//        DBG (String (parameter->getName ()) + " " + String (newItemSelected));

        jassert (newItemSelected > 0); // if you fall back here, then you have defined
                                       // a parameter which starts from 0 and not from
                                       // 1 and this is impossible in a ComboBox

        setSelectedId (newItemSelected, sendNotification);
#else
/*
        int newItemSelected = roundFloatToInt (parameter->getValue () * getNumItems () + 0.5f);

        jassert (newItemSelected > 0) // if you fall back here, then you have defined
                                      // a parameter which starts from 0 and not from
                                      // 1 and this is impossible in a ComboBox

        setSelectedId (newItemSelected, sendNotification);
*/
#endif
    }

};


#endif // __JUCETICE_PARAMETERTOGGLEBUTTON_HEADER__
