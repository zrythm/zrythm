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

#include "../DrumSynthPlugin.h"
#include "../DrumSynthComponent.h"

#include "DrumSynthKeyboard.h"
#include "DrumSynthMain.h"


DrumSynthKeyboard::DrumSynthKeyboard (DrumSynthMain* owner_, MidiKeyboardState &state)
  : MidiKeyboardComponent (state, MidiKeyboardComponent::horizontalKeyboard),
    owner (owner_)
{
}

void DrumSynthKeyboard::drawWhiteNote (int midiNoteNumber,
                                       Graphics& g, int x, int y, int w, int h,
                                       bool isDown, bool isOver,
                                       const Colour& lineColour,
                                       const Colour& textColour)
{
    Colour c (Colours::transparentWhite);

    if (isDown)
        c = findColour (keyDownOverlayColourId);

    if (isOver)
        c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));

    if (currentNote == midiNoteNumber)
        c = c.overlaidWith (Colours::red.withAlpha (0.5f));

    g.setColour (c);
    g.fillRect (x, y, w, h);

    const String text (getWhiteNoteText (midiNoteNumber));

    if (! text.isEmpty())
    {
        g.setColour (textColour);

        Font f (jmin (12.0f, getKeyWidth () * 0.9f));
        f.setHorizontalScale (0.8f);
        g.setFont (f);
        Justification justification (Justification::centredBottom);

        if (getOrientation () == verticalKeyboardFacingLeft)
            justification = Justification::centredLeft;
        else if (getOrientation () == verticalKeyboardFacingRight)
            justification = Justification::centredRight;

        g.drawFittedText (text, x + 2, y + 2, w - 4, h - 4, justification, 1);
    }

    g.setColour (lineColour);

    if (getOrientation () == horizontalKeyboard)
        g.fillRect (x, y, 1, h);
    else if (getOrientation () == verticalKeyboardFacingLeft)
        g.fillRect (x, y, w, 1);
    else if (getOrientation () == verticalKeyboardFacingRight)
        g.fillRect (x, y + h - 1, w, 1);

    if (midiNoteNumber == getRangeEnd ())
    {
        if (getOrientation () == horizontalKeyboard)
            g.fillRect (x + w, y, 1, h);
        else if (getOrientation () == verticalKeyboardFacingLeft)
            g.fillRect (x, y + h, w, 1);
        else if (getOrientation () == verticalKeyboardFacingRight)
            g.fillRect (x, y - 1, w, 1);
    }
}

void DrumSynthKeyboard::drawBlackNote (int midiNoteNumber,
                                       Graphics& g, int x, int y, int w, int h,
                                       bool isDown, bool isOver,
                                       const Colour& noteFillColour)
{
    Colour c (noteFillColour);

    if (isDown)
        c = c.overlaidWith (findColour (keyDownOverlayColourId));

    if (isOver)
        c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));

    if (currentNote == midiNoteNumber)
        c = c.overlaidWith (Colours::red.withAlpha (0.5f));

    g.setColour (c);
    g.fillRect (x, y, w, h);

    if (isDown)
    {
        g.setColour (noteFillColour);
        g.drawRect (x, y, w, h);
    }
    else
    {
        const int xIndent = jmax (1, jmin (w, h) / 8);

        g.setColour (c.brighter());

        if (getOrientation () == horizontalKeyboard)
            g.fillRect (x + xIndent, y, w - xIndent * 2, 7 * h / 8);
        else if (getOrientation () == verticalKeyboardFacingLeft)
            g.fillRect (x + w / 8, y + xIndent, w - w / 8, h - xIndent * 2);
        else if (getOrientation () == verticalKeyboardFacingRight)
            g.fillRect (x, y + xIndent, 7 * w / 8, h - xIndent * 2);
    }
}

void DrumSynthKeyboard::setCurrentNoteNumber (const int midiNoteNumber)
{
    currentNote = jmin ((int) (START_DRUM_NOTES_OFFSET + TOTAL_DRUM_NOTES),
                        jmax ((int) START_DRUM_NOTES_OFFSET, midiNoteNumber));

    repaint ();
}

bool DrumSynthKeyboard::mouseDownOnKey (int midiNoteNumber, const MouseEvent &e)
{
    currentNote = jmin ((int) (START_DRUM_NOTES_OFFSET + TOTAL_DRUM_NOTES),
                        jmax ((int) START_DRUM_NOTES_OFFSET, midiNoteNumber));

    owner->setCurrentDrumNumber (currentNote - START_DRUM_NOTES_OFFSET, true);
    
    repaint ();
    
    return true;
}

void DrumSynthKeyboard::mouseDraggedToKey (int midiNoteNumber, const MouseEvent& e)
{
    mouseDownOnKey (midiNoteNumber, e);
}

