/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2004 by Julian Storer.

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

 ------------------------------------------------------------------------------

 If you'd like to release a closed-source product which uses JUCE, commercial
 licenses are also available: visit www.rawmaterialsoftware.com/juce for
 more information.

 ==============================================================================
*/

#ifndef __JUCETICE_DRUMSYNTHKEYBOARD_HEADER__
#define __JUCETICE_DRUMSYNTHKEYBOARD_HEADER__

#include "../StandardHeader.h"

class DrumSynthMain;
class DrumSynthPlugin;
class DrumSynthComponent;

//==============================================================================
class DrumSynthKeyboard : public MidiKeyboardComponent
{
public:

    DrumSynthKeyboard (DrumSynthMain* owner, MidiKeyboardState &state);

    void setCurrentNoteNumber (const int midiNoteNumber);

    bool mouseDownOnKey (int midiNoteNumber, const MouseEvent &e) override;
    void mouseDraggedToKey (int midiNoteNumber, const MouseEvent& e) override;

protected:

    void drawWhiteNote (int midiNoteNumber,
                        Graphics& g, int x, int y, int w, int h,
                        bool isDown, bool isOver,
                        const Colour& lineColour,
                        const Colour& textColour) override;

    void drawBlackNote (int midiNoteNumber,
                        Graphics& g, int x, int y, int w, int h,
                        bool isDown, bool isOver,
                        const Colour& noteFillColour) override;

    DrumSynthMain* owner;
    int currentNote;
};

#endif

