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

BEGIN_JUCE_NAMESPACE

//==============================================================================
PianoGridKeyboard::PianoGridKeyboard (MidiKeyboardState& state)
  : MidiKeyboardComponent (state, MidiKeyboardComponent::verticalKeyboardFacingRight)
{
}

PianoGridKeyboard::~PianoGridKeyboard()
{
}

//==============================================================================
void PianoGridKeyboard::getKeyPosition (int midiNoteNumber, float keyWidth, int& x, int& w) const
{
    jassert (midiNoteNumber >= 0 && midiNoteNumber < 128);

    static const float blackNoteWidth = 0.7f;

    static const float notePos[] = { 0.0f, 1 - blackNoteWidth * 0.6f,
                                     1.0f, 2 - blackNoteWidth * 0.4f,
                                     2.0f,
                                     3.0f, 4 - blackNoteWidth * 0.7f,
                                     4.0f, 5 - blackNoteWidth * 0.5f,
                                     5.0f, 6 - blackNoteWidth * 0.3f,
                                     6.0f };

    static const float widths[] = { 1.0f, blackNoteWidth,
                                    1.0f, blackNoteWidth,
                                    1.0f,
                                    1.0f, blackNoteWidth,
                                    1.0f, blackNoteWidth,
                                    1.0f, blackNoteWidth,
                                    1.0f };

    const int octave = midiNoteNumber / 12;
    const int note = midiNoteNumber % 12;

    x = roundFloatToInt ((octave * 7.0f * keyWidth) + notePos [note] * keyWidth);
    w = roundFloatToInt (widths [note] * keyWidth);
}

String PianoGridKeyboard::getWhiteNoteText (const int midiNoteNumber)
{
    if (midiNoteNumber % 12 == 0)
        return MidiMessage::getMidiNoteName (midiNoteNumber, true, true, getOctaveForMiddleC ());

    return String();
}

//==============================================================================
void PianoGridKeyboard::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel)
{
    int newNote = getLowestVisibleKey();

    if ((wheel.deltaX != 0 ? wheel.deltaX : wheel.deltaY) < 0)
        newNote = (newNote - 1) / 12;
    else
        newNote = newNote / 12 + 1;

    setLowestVisibleKey (newNote * 12);
}

END_JUCE_NAMESPACE
