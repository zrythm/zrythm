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

   @author  justin
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

AudioSourceProcessor::AudioSourceProcessor (AudioSource* const inputSource,
                                            const bool deleteInputWhenDeleted_)
    : input(inputSource),
      deleteInputWhenDeleted(deleteInputWhenDeleted_)
{
}

AudioSourceProcessor::~AudioSourceProcessor()
{
   if (deleteInputWhenDeleted)
      delete input;   
}

const String AudioSourceProcessor::getName() const
{
    return "AudioSource wrapper";
}

const String AudioSourceProcessor::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String AudioSourceProcessor::getOutputChannelName (const int channelIndex) const   
{
    return String (channelIndex + 1);
}

void AudioSourceProcessor::prepareToPlay (double sampleRate, int estimatedSamplesPerBlock)
{
    input->prepareToPlay (estimatedSamplesPerBlock, sampleRate);
}

void AudioSourceProcessor::releaseResources()
{
    input->releaseResources();
}

void AudioSourceProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = buffer.getNumSamples();

    input->getNextAudioBlock (info);
} 

END_JUCE_NAMESPACE