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

#ifndef __JUCETICE_DRUMSYNTHPLUGIN_HEADER__
#define __JUCETICE_DRUMSYNTHPLUGIN_HEADER__

#include "DrumSynthGlobals.h"

//==============================================================================
/**
    Total number of drum sounds
*/
enum
{
    START_DRUM_NOTES_OFFSET = 36,
    TOTAL_DRUM_OCTAVES = 2,
    TOTAL_DRUM_NOTES = 12 * TOTAL_DRUM_OCTAVES,
    TOTAL_DRUM_VOICES = 32
};

//==============================================================================
/**
    Parameters enum definition
*/
enum
{
    OFFSET_PART_PARAMETERS = 0,

    PP_MAIN_TUNING = 0,
    PP_MAIN_STRETCH,
    PP_MAIN_GAIN,
    PP_MAIN_FILTER,
    PP_MAIN_HIGHPASS,
    PP_MAIN_RESONANCE,
    PP_MAIN_ENV_T1TIME,
    PP_MAIN_ENV_T1GAIN,
    PP_MAIN_ENV_T2TIME,
    PP_MAIN_ENV_T2GAIN,
    PP_MAIN_ENV_T3TIME,
    PP_MAIN_ENV_T3GAIN,
    PP_MAIN_ENV_T4TIME,
    PP_MAIN_ENV_T4GAIN,
    PP_MAIN_ENV_T5TIME,
    PP_MAIN_ENV_T5GAIN,

    PP_TONE_ON,
    PP_TONE_LEVEL,
    PP_TONE_F1,
    PP_TONE_F2,
    PP_TONE_DROOP,
    PP_TONE_PHASE,
    PP_TONE_ENV_T1TIME,
    PP_TONE_ENV_T1GAIN,
    PP_TONE_ENV_T2TIME,
    PP_TONE_ENV_T2GAIN,
    PP_TONE_ENV_T3TIME,
    PP_TONE_ENV_T3GAIN,
    PP_TONE_ENV_T4TIME,
    PP_TONE_ENV_T4GAIN,
    PP_TONE_ENV_T5TIME,
    PP_TONE_ENV_T5GAIN,

    PP_NOIZ_ON,
    PP_NOIZ_LEVEL,
    PP_NOIZ_SLOPE,
    PP_NOIZ_FIXEDSEQ,
    PP_NOIZ_ENV_T1TIME,
    PP_NOIZ_ENV_T1GAIN,
    PP_NOIZ_ENV_T2TIME,
    PP_NOIZ_ENV_T2GAIN,
    PP_NOIZ_ENV_T3TIME,
    PP_NOIZ_ENV_T3GAIN,
    PP_NOIZ_ENV_T4TIME,
    PP_NOIZ_ENV_T4GAIN,
    PP_NOIZ_ENV_T5TIME,
    PP_NOIZ_ENV_T5GAIN,

    PP_OTON_ON,
    PP_OTON_LEVEL,
    PP_OTON_F1,
    PP_OTON_WAVE1,
    PP_OTON_TRACK1,
    PP_OTON_F2,
    PP_OTON_WAVE2,
    PP_OTON_TRACK2,
    PP_OTON_METHOD,
    PP_OTON_PARAM,
    PP_OTON_FILTER,
    PP_OTON1_ENV_T1TIME,
    PP_OTON1_ENV_T1GAIN,
    PP_OTON1_ENV_T2TIME,
    PP_OTON1_ENV_T2GAIN,
    PP_OTON1_ENV_T3TIME,
    PP_OTON1_ENV_T3GAIN,
    PP_OTON1_ENV_T4TIME,
    PP_OTON1_ENV_T4GAIN,
    PP_OTON1_ENV_T5TIME,
    PP_OTON1_ENV_T5GAIN,
    PP_OTON2_ENV_T1TIME,
    PP_OTON2_ENV_T1GAIN,
    PP_OTON2_ENV_T2TIME,
    PP_OTON2_ENV_T2GAIN,
    PP_OTON2_ENV_T3TIME,
    PP_OTON2_ENV_T3GAIN,
    PP_OTON2_ENV_T4TIME,
    PP_OTON2_ENV_T4GAIN,
    PP_OTON2_ENV_T5TIME,
    PP_OTON2_ENV_T5GAIN,

    PP_NBA1_ON,
    PP_NBA1_LEVEL,
    PP_NBA1_F,
    PP_NBA1_DF,
    PP_NBA1_ENV_T1TIME,
    PP_NBA1_ENV_T1GAIN,
    PP_NBA1_ENV_T2TIME,
    PP_NBA1_ENV_T2GAIN,
    PP_NBA1_ENV_T3TIME,
    PP_NBA1_ENV_T3GAIN,
    PP_NBA1_ENV_T4TIME,
    PP_NBA1_ENV_T4GAIN,
    PP_NBA1_ENV_T5TIME,
    PP_NBA1_ENV_T5GAIN,

    PP_NBA2_ON,
    PP_NBA2_LEVEL,
    PP_NBA2_F,
    PP_NBA2_DF,
    PP_NBA2_ENV_T1TIME,
    PP_NBA2_ENV_T1GAIN,
    PP_NBA2_ENV_T2TIME,
    PP_NBA2_ENV_T2GAIN,
    PP_NBA2_ENV_T3TIME,
    PP_NBA2_ENV_T3GAIN,
    PP_NBA2_ENV_T4TIME,
    PP_NBA2_ENV_T4GAIN,
    PP_NBA2_ENV_T5TIME,
    PP_NBA2_ENV_T5GAIN,

    PP_DIST_ON,
    PP_DIST_CLIPPING,
    PP_DIST_BITS,
    PP_DIST_RATE,

    TOTAL_PART_PARAMETERS,

    // total number of parameters ------------------------------
    TOTAL_PARAMETERS = OFFSET_PART_PARAMETERS +
                       TOTAL_PART_PARAMETERS * TOTAL_DRUM_NOTES
};

#define PPAR(n,p)           (OFFSET_PART_PARAMETERS + (TOTAL_PART_PARAMETERS * n) + p)
#define PREG(n,p)           registerParameter (n, &p);

//==============================================================================
class DrumSynthSound;
class DrumSynthVoice;
class DrumSynthComponent;

//==============================================================================
/**
    A simple plugin filter that just applies a gain change to its input.

*/
class DrumSynthPlugin  : public AudioPlugin
{
public:

    //==============================================================================
    DrumSynthPlugin();
    ~DrumSynthPlugin() override;

    bool hasEditor() const override { return true; }

    bool silenceInProducesSilenceOut() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    const String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override     { return JucePlugin_WantsMidiInput; }
    bool producesMidi() const override    { return JucePlugin_ProducesMidiOutput; }

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    DrumSynthComponent* getEditor();

    //==============================================================================
    MidiKeyboardState* getKeyboardState ()               { return &keyboardState; }

    //==============================================================================
    const String& getDrumName (const int drumNumber) const
    {
        return notesNames[drumNumber];
    }

    void setDrumName (const int drumNumber, const String& drumName)
    {
        notesNames.set (drumNumber, drumName);
    }

    //==============================================================================
    const int getCurrentDrum () const
    {
        return currentDrumNumber;
    }

    void setCurrentDrum (const int newDrumNumber)
    {
        currentDrumNumber = jmax (0, jmin ((int) TOTAL_DRUM_NOTES, newDrumNumber));
    }

    //==============================================================================
    void triggerPanic ();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    String getStateInformationString () override;
    void setStateInformationString (const String& data) override;

    //==============================================================================
    void importDS (const int drumNumber, const File& file);
    void exportDS (const int drumNumber, const File& file);

    //==============================================================================
    void setLastBrowsedDirectory (const File& lastTouchedFile);

protected:

    //==============================================================================
    void readEnvelopeFromString (const int drumNumber,
                                 const int parameterOffset,
                                 const String& envelope);

    String writeEnvelopeToString (const int drumNumber,
                                  const int parameterOffset);

    //==============================================================================
    void registerNoteDrumParameters (const int noteNumber);

    //==============================================================================
    friend class DrumSynthVoice;
    friend class DrumSynthComponent;

    Synthesiser synth;
    MidiKeyboardState keyboardState;
    AudioSampleBuffer output;
    int currentDrumNumber;
    File lastBrowsedDirectory;

    //==============================================================================
    // TODO - make parameters array to follow plugin parameters
    StringArray notesNames;
    AudioParameter params [TOTAL_PARAMETERS];
};


#endif
