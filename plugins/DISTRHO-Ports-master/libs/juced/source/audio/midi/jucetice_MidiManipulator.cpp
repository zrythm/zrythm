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
MidiManipulator::MidiManipulator ()
    : filter (0),
      transform (0),
      sampleRate (44100.0001),
      blockSize (512)
{
}

MidiManipulator::~MidiManipulator ()
{
}

//==============================================================================
void MidiManipulator::setMidiFilter (MidiFilter* newMidiFilter)
{
    filter = newMidiFilter;
}

void MidiManipulator::setMidiTransform (MidiTransform* newMidiTransform)
{
    transform = newMidiTransform;
}

//==============================================================================
void MidiManipulator::prepareToPlay (const double sampleRate_, const int blockSize_)
{
    sampleRate = sampleRate_;
    blockSize = blockSize_;
}

void MidiManipulator::releaseResources ()
{
}

//==============================================================================
void MidiManipulator::processEvents (MidiBuffer& midiMessages, const int blockSize)
{
    MidiBuffer midiOutput;

    if (! midiMessages.isEmpty ())
    {
		int timeStamp;
		MidiMessage message (0xf4, 0.0);
	    MidiBuffer::Iterator it (midiMessages);

        if (filter)
        {
	        while (it.getNextEvent (message, timeStamp))
	        {
                if (filter->filterEvent (message))
                    midiOutput.addEvent (message, timeStamp);
	        }
	    }
	    else
	    {
	        midiOutput = midiMessages;
	    }

        midiMessages.clear ();
    }

    if (transform)
    {
        transform->processEvents (midiOutput, blockSize);
    }

    midiMessages = midiOutput;
}

END_JUCE_NAMESPACE