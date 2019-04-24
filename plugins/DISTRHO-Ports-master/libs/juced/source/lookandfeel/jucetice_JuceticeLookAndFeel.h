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

#ifndef __JUCETICE_JUCETICELOOKANDFEEL_HEADER__
#define __JUCETICE_JUCETICELOOKANDFEEL_HEADER__

//==============================================================================
/**
    A new, slightly plasticky looking look-and-feel.

    To make this the default look for your app, just set it as the default in
    your initialisation code.

    e.g. @code
    void initialise (const String& commandLine)
    {
        static JuceticeLookAndFeel myLook;
        LookAndFeel::setDefaultLookAndFeel (&myLook);
    }
    @endcode
*/
class JuceticeLookAndFeel : public LookAndFeel_V2
{
public:
    //==============================================================================
    /** Creates a JostLookAndFeel look and feel object. */
    JuceticeLookAndFeel();

    /** Destructor. */
    virtual ~JuceticeLookAndFeel();

    //==============================================================================
    Font getPopupMenuFont();

    //==============================================================================
    void drawToggleButton (Graphics& g,
                           ToggleButton& button,
                           bool isMouseOverButton,
                           bool isButtonDown);

    //==============================================================================
    /* TODO - is good but can be better
    void drawGroupComponentOutline (Graphics& g, int width, int height,
                                    const String& text,
                                    const Justification& position,
                                    GroupComponent& group);
    */

    //==============================================================================
    juce_UseDebuggingNewOperator
};


#endif   // __JUCETICE_JUCETICELOOKANDFEEL_HEADER__
