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

#ifndef __JUCETICE_AUDIOSOURCEPROCESSOR_HEADER__
#define __JUCETICE_AUDIOSOURCEPROCESSOR_HEADER__

class AudioSourceProcessor : public AudioProcessor
{
public:

    AudioSourceProcessor (AudioSource* const inputSource,
                         const bool deleteInputWhenDeleted);

    ~AudioSourceProcessor () override;

    void prepareToPlay (double sampleRate, int estimatedSamplesPerBlock) override;
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;
    void releaseResources() override;

    const String getName() const override;
    const String getInputChannelName (const int channelIndex) const override;
    const String getOutputChannelName (const int channelIndex) const override;

    bool isInputChannelStereoPair (int index)   const override             { return false; }
    bool isOutputChannelStereoPair (int index)   const override            { return false; }
    bool acceptsMidi() const override                                      { return false; }
    bool producesMidi() const override                                     { return false; }

    int getNumParameters() override                                        { return 0; }
    const String getParameterName (int parameterIndex) override            { return String(); }
    float getParameter (int parameterIndex) override                       { return 0.0; }
    const String getParameterText (int parameterIndex) override            { return String(); }
    void setParameter (int parameterIndex, float newValue) override        { }

    int getNumPrograms() override                                          { return 0; }
    int getCurrentProgram() override                                       { return 0; }
    void setCurrentProgram (int index) override                            { }
    const String getProgramName (int index) override                       { return String(); }
    void changeProgramName (int index, const String& newName) override     { }

    void getStateInformation (MemoryBlock& destData) override              { }
    void setStateInformation (const void* data,int sizeInBytes) override   { }

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    AudioProcessorEditor* createEditor() override                          { return 0; }
#endif

private:

    AudioSource* const input;
    const bool deleteInputWhenDeleted;
};

#endif

