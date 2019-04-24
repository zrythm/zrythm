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
MidiTransform::MidiTransform ()
: command (MidiTransform::KeepEvents),
channelNumber (1),
noteScale (0.5f),
noteTranspose (0),
velocityScale (0.5f),
velocityTranspose (0)
{
}

MidiTransform::~MidiTransform ()
{
}

//==============================================================================
void MidiTransform::setTransformCommand (TransformCommand newCommand)
{
    command = newCommand;
}

//==============================================================================
void MidiTransform::processEvents (MidiBuffer& midiMessages, const int blockSize)
{
    int timeStamp;
    MidiMessage message (0xf4, 0.0);
    MidiBuffer::Iterator it (midiMessages);
	
    MidiBuffer midiOutput;
	
    switch (command)
    {
		case MidiTransform::KeepEvents:
            break;
		case MidiTransform::DiscardEvents:
        {
            midiMessages.clear ();
            break;
        }
		case MidiTransform::RemapChannel:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                message.setChannel (channelNumber);
                midiOutput.addEvent (message, timeStamp);
            }
            midiMessages = midiOutput;
            break;
        }
		case MidiTransform::ScaleNotes:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                if (message.isNoteOnOrOff ())
                {
                    message.setNoteNumber (roundFloatToInt (message.getNoteNumber () * noteScale));
                    midiOutput.addEvent (message, timeStamp);
                }
            }
            midiMessages = midiOutput;
            break;
        }
		case MidiTransform::InvertNotes:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                if (message.isNoteOnOrOff ())
                {
                    message.setNoteNumber (127 - message.getNoteNumber ());
                    midiOutput.addEvent (message, timeStamp);
                }
            }
            midiMessages = midiOutput;
        }
		case MidiTransform::TransposeNotes:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                if (message.isNoteOnOrOff ())
                {
                    message.setNoteNumber (jmax (0, jmin (127, message.getNoteNumber () - noteTranspose)));
                    midiOutput.addEvent (message, timeStamp);
                }
            }
            midiMessages = midiOutput;
            break;
        }
		case MidiTransform::ScaleVelocity:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                if (message.isNoteOn ())
                {
                    message.setVelocity ((message.getVelocity () / 127.0f) * velocityScale);
                    midiOutput.addEvent (message, timeStamp);
                }
            }
            midiMessages = midiOutput;
            break;
        }
		case MidiTransform::InvertVelocity:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                if (message.isNoteOn ())
                {
                    message.setVelocity ((uint8) (127 - message.getVelocity ()));
                    midiOutput.addEvent (message, timeStamp);
                }
            }
            midiMessages = midiOutput;
            break;
        }
		case MidiTransform::TransposeVelocity:
        {
            while (it.getNextEvent (message, timeStamp))
            {
                if (message.isNoteOn ())
                {
                    message.setVelocity (jmax (0, jmin (127, message.getVelocity () - velocityTranspose)));
                    midiOutput.addEvent (message, timeStamp);
                }
            }
            midiMessages = midiOutput;
            break;
        }
		case MidiTransform::TriggerCC:
        {
            break;
        }
		case MidiTransform::TriggerNote:
        {
            break;
        }
    }
}

END_JUCE_NAMESPACE

