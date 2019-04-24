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

#ifndef __JUCETICE_AUDIOPROCESSINGBUFFER_HEADER__
#define __JUCETICE_AUDIOPROCESSINGBUFFER_HEADER__

//==============================================================================
/**
    Processing buffer helper class, typically used in AudioPlugin

*/
class AudioProcessingBuffer
{
public:

    //==============================================================================
    /** Constructor */
    AudioProcessingBuffer ()
        : inputBuffer (0),
          outputBuffer (0)
    {
    }

    /** Destructor */
    ~AudioProcessingBuffer ()
    {
        deleteAllMidiBuffers ();

        deleteAndZero (inputBuffer);
        deleteAndZero (outputBuffer);
    }

    //==============================================================================
    /** Returns our input buffers */
    AudioSampleBuffer* getInputBuffers () const        { return inputBuffer; }

    /** Returns our output buffers */
    AudioSampleBuffer* getOutputBuffers () const       { return outputBuffer; }

    //==============================================================================
    /**
        Allocate buffers for a specified number of i/o s.

        This takes care of getting the perfect number of channels for input
        and for outputs. The internal allocated buffers are the one we should rely
        on the audio callback.
    */
    void allocateBuffers (const int numInputs,
                          const int numOutputs,
                          const int numMidiInputs,
                          const int numMidiOutputs,
                          const int sizeOfBuffers)
    {
        // create audio buffers
        int doubleBlockSize = sizeOfBuffers;

        if (inputBuffer)
        {
            if (inputBuffer->getNumChannels () != numInputs
                || inputBuffer->getNumSamples () != doubleBlockSize)
            {
                deleteAndZero (inputBuffer);
            }
        }

        if (inputBuffer == 0 && numInputs > 0)
        {
            inputBuffer = new AudioSampleBuffer (numInputs, doubleBlockSize);
            inputBuffer->clear ();
        }

        if (outputBuffer)
        {
            if (outputBuffer->getNumChannels () != numOutputs
                || outputBuffer->getNumSamples () != doubleBlockSize)
            {
                deleteAndZero (outputBuffer);
            }
        }

        if (outputBuffer == 0 && numOutputs > 0)
        {
            outputBuffer = new AudioSampleBuffer (numOutputs, doubleBlockSize);
            outputBuffer->clear ();
        }

        // create midi buffers
        deleteAllMidiBuffers ();

        int maxBuffers = jmax (numMidiInputs, numMidiOutputs);
        if (maxBuffers > 0)
        {
            for (int i = 0; i < maxBuffers; i++)
                midiBuffers.add (new MidiBuffer ());
        }
    }

    //==============================================================================
    /**
        This returns a reference to the internal keyboard state
    */
    MidiBuffer* getMidiBuffer (const int index) const
    {
        return midiBuffers.getUnchecked (index);
    }

    /**
        Clears all internal midi buffers if any
    */
    void clearMidiBuffers ()
    {
        for (int i = midiBuffers.size (); --i >= 0;)
            midiBuffers.getUnchecked (i)->clear ();
    }

    /**
        Clears all available connections, freeing up
    */
    void deleteAllMidiBuffers ()
    {
        for (int i = midiBuffers.size (); --i >= 0;)
            delete midiBuffers.getUnchecked (i);

        midiBuffers.clear ();
    }

protected:

    //==============================================================================
    AudioSampleBuffer* inputBuffer;
    AudioSampleBuffer* outputBuffer;
    Array<MidiBuffer*> midiBuffers;
};

#endif
