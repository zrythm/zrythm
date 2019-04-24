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
MidiFilter::MidiFilter ()
    : useChannelFilter (false),
      useNoteFilter (false),
      useVelocityFilter (false),
      usePitchWeelFilter (false),
      noteMin (0),
      noteMax (127),
      velocityMin (0),
      velocityMax (127),
      pitchMin (0),
      pitchMax (16383)
{
    channels.clear ();
}

MidiFilter::~MidiFilter ()
{
}

//==============================================================================
void MidiFilter::setUseChannelFilter (const bool useFilter)
{
    useChannelFilter = useFilter;
}

void MidiFilter::clearAllChannels ()
{
    channels.clear ();
}

void MidiFilter::setChannel (const int channelNumber)
{
    channels.setBit (channelNumber - 1);
}

void MidiFilter::unsetChannel (const int channelNumber)
{
    channels.clearBit (channelNumber - 1);
}

//==============================================================================
void MidiFilter::setUseNoteFilter (const bool useFilter)
{
    useNoteFilter = useFilter;
}

void MidiFilter::setNoteMin (const int newNoteMin)
{
    noteMin = newNoteMin;
}

void MidiFilter::setNoteMax (const int newNoteMax)
{
    noteMax = newNoteMax;
}

//==============================================================================
void MidiFilter::setUseVelocityFilter (const bool useFilter)
{
    useVelocityFilter = useFilter;
}

void MidiFilter::setVelocityMin (const int newVelocityMin)
{
    velocityMin = newVelocityMin;
}

void MidiFilter::setVelocityMax (const int newVelocityMax)
{
    velocityMax = newVelocityMax;
}

//==============================================================================
void MidiFilter::setUsePitchWeelFilter (const bool useFilter)
{
    usePitchWeelFilter = useFilter;
}

//==============================================================================
bool MidiFilter::filterEvent (const MidiMessage& message)
{
    if (useChannelFilter)
    {
        const int channelNumber = message.getChannel () - 1;
        if (! channels [channelNumber])
        {
            DBG ("MidiFilter::filterEvent > Bad channel " + String (channelNumber));
            return false;
        }
    }

    if (useNoteFilter
        && message.isNoteOnOrOff ())
    {
        const int noteNumber = message.getNoteNumber ();
        if (noteNumber < noteMin || noteNumber > noteMax)
        {
            DBG ("MidiFilter::filterEvent > Bad note " + String (noteNumber));
            return false;
        }
    }

    if (useVelocityFilter
        && message.isNoteOn ())
    {
        const int velocityValue = message.getVelocity ();
        if (velocityValue < velocityMin || velocityValue > velocityMax)
        {
            DBG ("MidiFilter::filterEvent > Bad velocity " + String (velocityValue));
            return false;
        }
    }

    if (usePitchWeelFilter
        && message.isPitchWheel ())
    {
        const int pitchValue = message.getPitchWheelValue ();
        if (pitchValue < pitchMin || pitchValue > pitchMax)
        {
            DBG ("MidiFilter::filterEvent > Bad pitch " + String (pitchValue));
            return false;
        }
    }
    
    return true;    
}

END_JUCE_NAMESPACE