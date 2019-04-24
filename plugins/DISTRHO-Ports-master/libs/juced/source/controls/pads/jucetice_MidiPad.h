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

    Original code: Reuben a.k.a. Insertpizhere

 ==============================================================================
*/

#ifndef __JUCETICE_MIDIPAD_HEADER__
#define __JUCETICE_MIDIPAD_HEADER__

#include "jucetice_DrawablePad.h"


class MidiPad  : public Component,
                 public ButtonListener,
                 public FileDragAndDropTarget
{
public:

    //==============================================================================
    /** Constructor */
  	MidiPad ();  

    /** Destructor */
    ~MidiPad();

    //==============================================================================
    void setButtonText (const String&);
    void setColour (const Colour&);

    void setTriggeredOnMouseDown (const bool);
    void addButtonListener (ButtonListener *const);
    void setToggleState (bool state);

    //==============================================================================
    /** @internal */
    void filesDropped (const StringArray &files, int x, int y);
    /** @internal */
    bool isInterestedInFileDrag (const StringArray& files);
    /** @internal */
    void buttonClicked (Button*);
    /** @internal */
    void paint (Graphics&);
    /** @internal */
    void resized();

    //==============================================================================
    /** @TODO - make protected ! */
    bool isPlaying;
    DrawablePad* drawableButton;

    //==============================================================================
    juce_UseDebuggingNewOperator

private:

    bool hitTest (int x, int y);

    MidiPad (const MidiPad&);
    const MidiPad& operator= (const MidiPad&);
};

#endif
