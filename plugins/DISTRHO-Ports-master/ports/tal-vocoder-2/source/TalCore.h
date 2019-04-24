/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef TALCORE_H
#define TALCORE_H

#include "./engine/VocoderEngine.h"
#include "./engine/Params.h"
#include "TalPreset.h"

//==============================================================================
/**
    A simple plugin filter that just applies a gain change to the audio
    passing through it.

*/

class TalCore  : public AudioProcessor,
                 public ChangeBroadcaster
{
public:
    //==============================================================================
    TalCore();
    ~TalCore() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    bool hasEditor() const override                  { return true; }
    AudioProcessorEditor* createEditor() override;
#endif

    // vst
    const String getName() const override;

    int getNumParameters() override;

    float getParameter (int index) override;
    void setParameter (int index, float newValue) override;

    const String getParameterName (int index) override;
    const String getParameterText (int index) override;

    const String getInputChannelName (int channelIndex) const override;
    const String getOutputChannelName (int channelIndex) const override;
    bool isInputChannelStereoPair (int index) const override;
    bool isOutputChannelStereoPair (int index) const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;

    bool silenceInProducesSilenceOut() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    bool doesClip();

    // vst preset handling
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    // preset chunk save load
	void getStateInformation (MemoryBlock& destData) override;
	void getCurrentProgramStateInformation (MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;
	void setCurrentProgramStateInformation (const void* data, int sizeInBytes) override;

    void setStateInformationFromXml(XmlElement* xmlState);
    XmlElement* getCurrentProgramStateInformationAsXml();

    void getXmlPrograms(XmlElement *programList, int programNumber);
    void setXmlPrograms(XmlElement *programList, int programNumber, float version);

    String getStateInformationString () override;
    void setStateInformationString (const String& data) override;

    // midi
    bool getNextEvent(MidiBuffer::Iterator *midiIterator, const int samplePos);
	void processMidiPerSample (MidiBuffer::Iterator *midiIterator, int samplePos);

    void setNewSampleRate();

private:

	VocoderEngine *engine;
	float sampleRate;

	TalPreset **talPresets;
	int curProgram;

	// Midi values
	int midiEventPos;
	bool hasMoreMidiMessages;

    MidiMessage *nextMidiMessage;
    MidiMessage *midiMessage;
	bool hasMidiMessage;

    int numberOfPrograms;

    CriticalSection myCriticalSectionBuffer;
};
#endif
