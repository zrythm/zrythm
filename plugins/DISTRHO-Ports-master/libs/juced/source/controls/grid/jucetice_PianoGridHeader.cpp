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

#include "jucetice_PianoGrid.h"

//==============================================================================
PianoGridHeader::PianoGridHeader (MidiGrid* owner_)
  : owner (owner_)
{
    setOpaque (true);
}

PianoGridHeader::~PianoGridHeader()
{
}

void PianoGridHeader::mouseUp (const MouseEvent& e)
{
    owner->notifyListenersOfPlayingPositionChanged (e.x / (float) getWidth());
}

void PianoGridHeader::paint (Graphics& g)
{
    int numBars = owner->getNumBars ();
    int numBeats = owner->getTimeDivision ();
    int barWidth = getWidth () / numBars;
    int beatWidth = barWidth / numBeats;

    Colour backCol = findColour (PianoGrid::headerColourId);
    g.fillAll (backCol);

    g.setColour (backCol.contrasting ());
    for (int i = 0; i < numBars; i++)
    {
        g.setFont (Font (10.0f, Font::plain));
        g.drawText (String (i + 1),
                    barWidth * i + 1, 0, 22, getHeight(),
                    Justification::centredLeft,
                    false);

        if (barWidth >= 80)
        {
            g.setFont (Font (8.0f, Font::plain));
            for (int j = 1; j < numBeats; j++)
            {
                g.drawText (String (j + 1) + "/" + String (numBeats),
                            barWidth * i + beatWidth * j - 2, 0, 18, getHeight(),
                            Justification::bottomLeft,
                            false);
            }
        }
    }
}

END_JUCE_NAMESPACE
