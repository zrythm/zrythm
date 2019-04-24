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

#include "DrumSynthPlugin.h"
#include "DrumSynthComponent.h"
#include "DrumSynthVoice.h"

#include "IniParser/iniparser.h"
#include "Resources/DrumSynthResources.h"


//==============================================================================
DrumSynthPlugin::DrumSynthPlugin()
    : output (1,1)
{
    // initially reset it to zero
    currentDrumNumber = 0;

    // register parameters
    setNumParameters (TOTAL_PARAMETERS);

    // add a wave sound, which will get applied to some of the notes..
    BigInteger drumNotes;
    for (int noteNumber = 0; noteNumber < TOTAL_DRUM_NOTES; noteNumber++)
    {
        drumNotes.setBit (START_DRUM_NOTES_OFFSET + noteNumber);

        notesNames.add ("Unset");

        registerNoteDrumParameters (noteNumber);
    }

    synth.addSound (new DrumSynthSound (drumNotes));

    // give our synth a few voices that can play the wave sound..
    for (int i = TOTAL_DRUM_VOICES; --i >= 0;)
        synth.addVoice (new DrumSynthVoice (i, this));

    // initialize bank
    setStateInformation (Resources::banktr909, Resources::banktr909_size);
}

DrumSynthPlugin::~DrumSynthPlugin()
{
}

//==============================================================================
void DrumSynthPlugin::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    // update synthesizer info
    synth.setCurrentPlaybackSampleRate (newSampleRate);

    for (int i = TOTAL_DRUM_VOICES; --i >= 0;)
        ((DrumSynthVoice*) synth.getVoice (i))->prepareToPlay (newSampleRate, samplesPerBlock);

    // reset midi keyboard state
    keyboardState.reset ();

    // create output buffer
    output.setSize (1, samplesPerBlock * 2, false, false, false);
    output.clear ();
}

void DrumSynthPlugin::releaseResources()
{
    for (int i = TOTAL_DRUM_VOICES; --i >= 0;)
        ((DrumSynthVoice*) synth.getVoice (i))->releaseResources ();
}

void DrumSynthPlugin::processBlock (AudioSampleBuffer& buffer,
                                    MidiBuffer& midiMessages)
{
    int blockSamples = buffer.getNumSamples();
    output.clear ();

    // process incoming midi (automation of parameters)
    midiAutomatorManager.handleMidiMessageBuffer (midiMessages);

    // process incoming midi (keypress on gui midi note keyboard)
    keyboardState.processNextMidiBuffer (midiMessages,
                                         0, blockSamples,
                                         true);

    // render drum synth voices
    synth.renderNextBlock (output, midiMessages, 0, blockSamples);

    // produce output buffers
    buffer.clear ();
    buffer.addFrom (0, 0, output, 0, 0, blockSamples, 2.0f);
    buffer.addFrom (1, 0, output, 0, 0, blockSamples, 2.0f);
}

//==============================================================================
void DrumSynthPlugin::registerNoteDrumParameters (const int noteNumber)
{
    int n;

    // main --------------------------------------------------------------------
    n = PPAR(noteNumber, PP_MAIN_TUNING);
    PREG(n, params[n].part (n).name ("Tuning").unit ("st").range (-24.0f, 24.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_STRETCH);
    PREG(n, params[n].part (n).name ("Stretch").unit ("%").range (10.0f, 200.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_MAIN_GAIN);
    PREG(n, params[n].part (n).name ("Gain").unit ("%").range (-60.0f, 10.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_FILTER);
    PREG(n, params[n].part (n).name ("Filter").unit ("%").range (0, 1));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_HIGHPASS);
    PREG(n, params[n].part (n).name ("HP").unit ("%").range (0, 1));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_RESONANCE);
    PREG(n, params[n].part (n).name ("Resonance").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // -------------------------------------------------------------------------
    n = PPAR(noteNumber, PP_MAIN_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("F-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("F-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("F-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("F-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("F-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("F-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("F-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("F-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("F-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_MAIN_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("F-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // tone --------------------------------------------------------------------
    n = PPAR(noteNumber, PP_TONE_ON);
    PREG(n, params[n].part (n).name ("Tone On").unit ("?").range (0, 1));
    setParameterMapped (n, 1);

    n = PPAR(noteNumber, PP_TONE_LEVEL);
    PREG(n, params[n].part (n).name ("Tone Amt").unit ("%").range (0.0f, 200.0f));
    setParameterMapped (n, 128.0f);

    n = PPAR(noteNumber, PP_TONE_F1);
    PREG(n, params[n].part (n).name ("Freq1").unit ("hz").range (20.0f, 11025.0f));
    setParameterMapped (n, 200.0f);

    n = PPAR(noteNumber, PP_TONE_F2);
    PREG(n, params[n].part (n).name ("Freq2").unit ("hz").range (20.0f, 11025.0f));
    setParameterMapped (n, 50.0f);

    n = PPAR(noteNumber, PP_TONE_DROOP);
    PREG(n, params[n].part (n).name ("Droop").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_TONE_PHASE);
    PREG(n, params[n].part (n).name ("Phase").unit (CharPointer_UTF8("º")).range (0.0f, 90.0f));
    setParameterMapped (n, 0.0f);

    // -------------------------------------------------------------------------
    n = PPAR(noteNumber, PP_TONE_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("T-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("T-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("T-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("T-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("T-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("T-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("T-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("T-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("T-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_TONE_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("T-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // noise ------------------------------------------------------------------
    n = PPAR(noteNumber, PP_NOIZ_ON);
    PREG(n, params[n].part (n).name ("Noise On").unit ("?").range (0, 1));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_NOIZ_LEVEL);
    PREG(n, params[n].part (n).name ("Noise Amt").unit ("%").range (0, 200.0f));
    setParameterMapped (n, 128.0f);

    n = PPAR(noteNumber, PP_NOIZ_SLOPE);
    PREG(n, params[n].part (n).name ("Slope Amt").unit ("%").range (-100, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NOIZ_FIXEDSEQ);
    PREG(n, params[n].part (n).name ("Fixed Seq").unit ("?").range (0, 1));
    setParameterMapped (n, 1);

    // -------------------------------------------------------------------------
    n = PPAR(noteNumber, PP_NOIZ_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("N-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("N-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("N-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("N-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("N-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_NOIZ_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // overtones --------------------------------------------------------------
    n = PPAR(noteNumber, PP_OTON_ON);
    PREG(n, params[n].part (n).name ("Over On").unit ("?").range (0, 1));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_OTON_LEVEL);
    PREG(n, params[n].part (n).name ("Over Amt").unit ("%").range (0, 200.0f));
    setParameterMapped (n, 128.0f);

    n = PPAR(noteNumber, PP_OTON_F1);
    PREG(n, params[n].part (n).name ("Over F1").unit ("%").range (20, 22050.0f));
    setParameterMapped (n, 4600);

    n = PPAR(noteNumber, PP_OTON_WAVE1);
    PREG(n, params[n].part (n).name ("Over Wave1").unit ("n").range (0, 4));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_OTON_TRACK1);
    PREG(n, params[n].part (n).name ("Over Track1").unit ("?").range (0, 1));
    setParameterMapped (n, 1);

    n = PPAR(noteNumber, PP_OTON_F2);
    PREG(n, params[n].part (n).name ("Over F2").unit ("hz").range (20, 22050.0f));
    setParameterMapped (n, 1600);

    n = PPAR(noteNumber, PP_OTON_WAVE2);
    PREG(n, params[n].part (n).name ("Over Wave2").unit ("n").range (0, 4));
    setParameterMapped (n, 1);

    n = PPAR(noteNumber, PP_OTON_TRACK2);
    PREG(n, params[n].part (n).name ("Over Track2").unit ("?").range (0, 1));
    setParameterMapped (n, 1);

    n = PPAR(noteNumber, PP_OTON_METHOD);
    PREG(n, params[n].part (n).name ("Over Method").unit ("n").range (0, 2));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_OTON_PARAM);
    PREG(n, params[n].part (n).name ("Over Param").unit ("%").range (0, 100));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_OTON_FILTER);
    PREG(n, params[n].part (n).name ("Over Filter").unit ("%").range (0, 1));
    setParameterMapped (n, 0);

    // -------------------------------------------------------------------------
    n = PPAR(noteNumber, PP_OTON1_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("O-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("O-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("O-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("O-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("O-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_OTON1_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("O-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("O-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("O-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("O-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("O-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_OTON2_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("O-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // noise band 1 -----------------------------------------------------------
    n = PPAR(noteNumber, PP_NBA1_ON);
    PREG(n, params[n].part (n).name ("NzBnd1 On").unit ("?").range (0, 1));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_NBA1_LEVEL);
    PREG(n, params[n].part (n).name ("NzBnd1 Amt").unit ("%").range (0.0f, 200.0f));
    setParameterMapped (n, 128);

    n = PPAR(noteNumber, PP_NBA1_F);
    PREG(n, params[n].part (n).name ("NzBnd1 F").unit ("hz").range (30, 22050.0f));
    setParameterMapped (n, 3000);

    n = PPAR(noteNumber, PP_NBA1_DF);
    PREG(n, params[n].part (n).name ("NzBnd1 DF").unit ("%").range (0, 50));
    setParameterMapped (n, 0);

    // -------------------------------------------------------------------------
    n = PPAR(noteNumber, PP_NBA1_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("N-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("N-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("N-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("N-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("N-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_NBA1_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // noise band 2 -----------------------------------------------------------
    n = PPAR(noteNumber, PP_NBA2_ON);
    PREG(n, params[n].part (n).name ("NzBnd2 On").unit ("?").range (0, 1));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_NBA2_LEVEL);
    PREG(n, params[n].part (n).name ("NzBnd2 Amt").unit ("%").range (0.0f, 200.0f));
    setParameterMapped (n, 128);

    n = PPAR(noteNumber, PP_NBA2_F);
    PREG(n, params[n].part (n).name ("NzBnd2 F").unit ("hz").range (30, 22050.0f));
    setParameterMapped (n, 6000);

    n = PPAR(noteNumber, PP_NBA2_DF);
    PREG(n, params[n].part (n).name ("NzBnd2 DF").unit ("%").range (0, 50));
    setParameterMapped (n, 0);

    // -------------------------------------------------------------------------
    n = PPAR(noteNumber, PP_NBA2_ENV_T1TIME);
    PREG(n, params[n].part (n).name ("N-Env-T1").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T1GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G1").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T2TIME);
    PREG(n, params[n].part (n).name ("N-Env-T2").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 44100.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T2GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G2").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 100.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T3TIME);
    PREG(n, params[n].part (n).name ("N-Env-T3").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 50000.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T3GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G3").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T4TIME);
    PREG(n, params[n].part (n).name ("N-Env-T4").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T4GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G4").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T5TIME);
    PREG(n, params[n].part (n).name ("N-Env-T5").unit ("%").range (0.0f, 44100.0f * 5));
    setParameterMapped (n, 51000.0f);

    n = PPAR(noteNumber, PP_NBA2_ENV_T5GAIN);
    PREG(n, params[n].part (n).name ("N-Env-G5").unit ("%").range (0.0f, 100.0f));
    setParameterMapped (n, 0.0f);

    // dist ------------------------------------------------------------------
    n = PPAR(noteNumber, PP_DIST_ON);
    PREG(n, params[n].part (n).name ("Dist On").unit ("?").range (0, 1));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_DIST_CLIPPING);
    PREG(n, params[n].part (n).name ("Clipping").unit ("%").range (0, 16));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_DIST_BITS);
    PREG(n, params[n].part (n).name ("Bits").unit ("bits").range (0, 16));
    setParameterMapped (n, 0);

    n = PPAR(noteNumber, PP_DIST_RATE);
    PREG(n, params[n].part (n).name ("Rate").unit ("hz").range (0, 10));
    setParameterMapped (n, 0);
}

//==============================================================================
void DrumSynthPlugin::getStateInformation (MemoryBlock& destData)
{
#ifndef JUCE_DEBUG
    try
    {
#endif

        MemoryBlock tempBlock;
        XmlElement xml ("preset");
        // xml.setAttribute ("version", JucePlugin_VersionCode);

        for (int i = 0; i < getNumParameters (); i++)
        {
            XmlElement* e = new XmlElement ("p");
            e->setAttribute ("key", i);
            e->setAttribute ("value", (double) getParameter (i));
            xml.addChildElement (e);
        }

        for (int i = 0; i < TOTAL_DRUM_NOTES; i++)
        {
            XmlElement* e = new XmlElement ("n");
            e->setAttribute ("key", i);
            e->setAttribute ("name", getDrumName (i));
            xml.addChildElement (e);
        }

        String xmlString = xml.createDocument (String());
        destData.append ((const char*) xmlString.toUTF8(), xmlString.length());

#ifndef JUCE_DEBUG
    }
    catch (...)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     "Error !",
                                     "Something bad occurred while saving presets data !");
    }
#endif
}

void DrumSynthPlugin::setStateInformation (const void* data, int sizeInBytes)
{
    suspendProcessing (true);

#ifndef JUCE_DEBUG
    try
    {
#endif

        if (data && sizeInBytes > 0)
        {
            XmlDocument xmlDoc ((char*) data);

            XmlElement* xml = xmlDoc.getDocumentElement();
            if (xml == 0 || ! xml->hasTagName ("preset"))
            {
                String errString = xmlDoc.getLastParseError();
                printf ("Error restoring preset: %s \n", (const char*) errString.toUTF8());
            }
            else
            {
                // TODO - take care of versioning
                // int version = xml->getIntAttribute ("version", -1);

                forEachXmlChildElement (*xml, e)
                {
                    if (e->hasTagName ("p"))
                    {
                        int key = e->getIntAttribute ("key", -1);
                        double value = e->getDoubleAttribute ("value", 0.0f);

                        if (key >= 0 && key < getNumParameters ())
                            setParameter (key, (float) value);
                    }
                    else if (e->hasTagName ("n"))
                    {
                        int key = e->getIntAttribute ("key", -1);
                        String name = e->getStringAttribute ("name", "Unset");

                        if (key >= 0 && key < TOTAL_DRUM_NOTES)
                            setDrumName (key, name);
                    }
                }

                delete xml;
            }

            sendChangeMessage ();
        }

#ifndef JUCE_DEBUG
    }
    catch (...)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                    "Error !",
                                    "Something bad occurred while restoring presets data !");
    }
#endif

    suspendProcessing (false);
}

//==============================================================================
String DrumSynthPlugin::getStateInformationString ()
{
    XmlElement xml ("preset");
    // xml.setAttribute ("version", JucePlugin_VersionCode);

    for (int i = 0; i < getNumParameters (); i++)
    {
        XmlElement* e = new XmlElement ("p");
        e->setAttribute ("key", i);
        e->setAttribute ("value", (double) getParameter (i));
        xml.addChildElement (e);
    }

    for (int i = 0; i < TOTAL_DRUM_NOTES; i++)
    {
        XmlElement* e = new XmlElement ("n");
        e->setAttribute ("key", i);
        e->setAttribute ("name", getDrumName (i));
        xml.addChildElement (e);
    }

    return xml.createDocument (String());
}

void DrumSynthPlugin::setStateInformationString (const String& data)
{
    suspendProcessing (true);

    XmlElement* const xml = XmlDocument::parse(data);
    if (xml != 0)
    {
        // TODO - take care of versioning
        // int version = xml->getIntAttribute ("version", -1);

        forEachXmlChildElement (*xml, e)
        {
            if (e->hasTagName ("p"))
            {
                int key = e->getIntAttribute ("key", -1);
                double value = e->getDoubleAttribute ("value", 0.0f);

                if (key >= 0 && key < getNumParameters ())
                    setParameter (key, (float) value);
            }
            else if (e->hasTagName ("n"))
            {
                int key = e->getIntAttribute ("key", -1);
                String name = e->getStringAttribute ("name", "Unset");

                if (key >= 0 && key < TOTAL_DRUM_NOTES)
                    setDrumName (key, name);
            }
        }

        delete xml;

        sendChangeMessage ();
    }

    suspendProcessing (false);
}

//==============================================================================
void DrumSynthPlugin::triggerPanic ()
{
    for (int i = 1; i <= 16; i++)
        synth.allNotesOff (i, false);
}

//==============================================================================
void DrumSynthPlugin::setLastBrowsedDirectory (const File& lastTouchedFile)
{
    lastBrowsedDirectory = lastTouchedFile.existsAsFile() ? lastTouchedFile.getParentDirectory ()
                                                          : lastTouchedFile;
}

//==============================================================================
void DrumSynthPlugin::importDS (const int drumNumber, const File& file)
{
    dictionary* ini;

    ini = iniparser_load ((const char*) file.getFullPathName().toUTF8());
	if (ini == 0)
		return;

//    String version = iniparser_getstring (ini, "general:version", "DrumSynth v1.0");
//    String comment = iniparser_getstring (ini, "general:comment", "None");

    notesNames.set (drumNumber, file.getFileNameWithoutExtension());

    setParameterMapped (PPAR (drumNumber, PP_MAIN_TUNING),    iniparser_getdouble (ini, "general:tuning", 0.0));
    setParameterMapped (PPAR (drumNumber, PP_MAIN_STRETCH),   iniparser_getdouble (ini, "general:stretch", 100.0));
    setParameterMapped (PPAR (drumNumber, PP_MAIN_GAIN),      iniparser_getdouble (ini, "general:level", 0.0));
    setParameterMapped (PPAR (drumNumber, PP_MAIN_FILTER),    iniparser_getint    (ini, "general:filter", 0));
    setParameterMapped (PPAR (drumNumber, PP_MAIN_HIGHPASS),  iniparser_getint    (ini, "general:highpass", 0));
    setParameterMapped (PPAR (drumNumber, PP_MAIN_RESONANCE), iniparser_getint    (ini, "general:resonance", 0));
    readEnvelopeFromString (drumNumber, PP_MAIN_ENV_T1TIME, iniparser_getstring (ini, "general:filterenv", "0,100 442000,100 442000,0"));

    setParameterMapped (PPAR (drumNumber, PP_TONE_ON),        iniparser_getint    (ini, "tone:on", 1));
    setParameterMapped (PPAR (drumNumber, PP_TONE_LEVEL),     iniparser_getint    (ini, "tone:level", 128));
    setParameterMapped (PPAR (drumNumber, PP_TONE_F1),        iniparser_getdouble (ini, "tone:f1", 200.0));
    setParameterMapped (PPAR (drumNumber, PP_TONE_F2),        iniparser_getdouble (ini, "tone:f2", 50.0));
    setParameterMapped (PPAR (drumNumber, PP_TONE_DROOP),     iniparser_getdouble (ini, "tone:droop", 50.0));
    setParameterMapped (PPAR (drumNumber, PP_TONE_PHASE),     iniparser_getdouble (ini, "tone:phase", 0.0));
    readEnvelopeFromString (drumNumber, PP_TONE_ENV_T1TIME, iniparser_getstring (ini, "tone:envelope", "0,100 5250,30 10500,0"));

    setParameterMapped (PPAR (drumNumber, PP_NOIZ_ON),        iniparser_getint    (ini, "noise:on", 0));
    setParameterMapped (PPAR (drumNumber, PP_NOIZ_LEVEL),     iniparser_getint    (ini, "noise:level", 128));
    setParameterMapped (PPAR (drumNumber, PP_NOIZ_SLOPE),     iniparser_getint    (ini, "noise:slope", 0));
    setParameterMapped (PPAR (drumNumber, PP_NOIZ_FIXEDSEQ),  iniparser_getint    (ini, "noise:fixedseq", 0));
    readEnvelopeFromString (drumNumber, PP_NOIZ_ENV_T1TIME, iniparser_getstring (ini, "noise:envelope", "0,100 894,45 2756,18 5289,8 20113,0"));

    setParameterMapped (PPAR (drumNumber, PP_OTON_ON),        iniparser_getint    (ini, "overtones:on", 0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_LEVEL),     iniparser_getint    (ini, "overtones:level", 128));
    setParameterMapped (PPAR (drumNumber, PP_OTON_F1),        iniparser_getdouble (ini, "overtones:f1", 100.0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_WAVE1),     iniparser_getint    (ini, "overtones:wave1", 0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_TRACK1),    iniparser_getint    (ini, "overtones:track1", 0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_F2),        iniparser_getdouble (ini, "overtones:f2", 60.0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_WAVE2),     iniparser_getint    (ini, "overtones:wave2", 1));
    setParameterMapped (PPAR (drumNumber, PP_OTON_TRACK2),    iniparser_getint    (ini, "overtones:track2", 0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_METHOD),    iniparser_getint    (ini, "overtones:method", 0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_PARAM),     iniparser_getint    (ini, "overtones:param", 0));
    setParameterMapped (PPAR (drumNumber, PP_OTON_FILTER),    iniparser_getint    (ini, "overtones:filter", 0));
    readEnvelopeFromString (drumNumber, PP_OTON1_ENV_T1TIME,iniparser_getstring (ini, "overtones:envelope1", "0,100 1341,37 5215,10 20784,0"));
    readEnvelopeFromString (drumNumber, PP_OTON2_ENV_T1TIME,iniparser_getstring (ini, "overtones:envelope2", "0,100 1341,37 5215,10 20784,0"));

    setParameterMapped (PPAR (drumNumber, PP_NBA1_ON),        iniparser_getint    (ini, "noiseband:on", 0));
    setParameterMapped (PPAR (drumNumber, PP_NBA1_LEVEL),     iniparser_getint    (ini, "noiseband:level", 128));
    setParameterMapped (PPAR (drumNumber, PP_NBA1_F),         iniparser_getdouble (ini, "noiseband:f", 100.0));
    setParameterMapped (PPAR (drumNumber, PP_NBA1_DF),        iniparser_getint    (ini, "noiseband:df", 0));
    readEnvelopeFromString (drumNumber, PP_NBA1_ENV_T1TIME, iniparser_getstring (ini, "noiseband:envelope", "0,100 1639,38 4619,20 10802,10 21082,0"));

    setParameterMapped (PPAR (drumNumber, PP_NBA2_ON),        iniparser_getint    (ini, "noiseband2:on", 0));
    setParameterMapped (PPAR (drumNumber, PP_NBA2_LEVEL),     iniparser_getint    (ini, "noiseband2:level", 128));
    setParameterMapped (PPAR (drumNumber, PP_NBA2_F),         iniparser_getdouble (ini, "noiseband2:f", 100.0));
    setParameterMapped (PPAR (drumNumber, PP_NBA2_DF),        iniparser_getint    (ini, "noiseband2:df", 0));
    readEnvelopeFromString (drumNumber, PP_NBA2_ENV_T1TIME, iniparser_getstring (ini, "noiseband2:envelope", "0,100 1639,38 4619,20 10802,10 21082,0"));

    setParameterMapped (PPAR (drumNumber, PP_DIST_ON),        iniparser_getint    (ini, "dist:on", 0));
    setParameterMapped (PPAR (drumNumber, PP_DIST_CLIPPING),  iniparser_getint    (ini, "dist:clipping", 128));
    setParameterMapped (PPAR (drumNumber, PP_DIST_BITS),      iniparser_getint    (ini, "dist:bits", 0));
    setParameterMapped (PPAR (drumNumber, PP_DIST_RATE),      iniparser_getint    (ini, "dist:rate", 0));

	iniparser_freedict (ini);

	// trigger update for the editor !
	sendChangeMessage ();
}

void DrumSynthPlugin::exportDS (const int drumNumber, const File& file)
{
    String content;

    content <<
        "[General]"     << "\n" <<
        "Version="      << "DrumSynth v2.0" << "\n" <<
        "Comment="      << notesNames[drumNumber] << "\n" <<
        "Tuning="       << getParameterMapped (PPAR (drumNumber, PP_MAIN_TUNING)) << "\n" <<
        "Stretch="      << getParameterMapped (PPAR (drumNumber, PP_MAIN_STRETCH)) << "\n" <<
        "Gain="         << (int) getParameterMapped (PPAR (drumNumber, PP_MAIN_GAIN)) << "\n" <<
        "Filter="       << (int) getParameterMapped (PPAR (drumNumber, PP_MAIN_FILTER)) << "\n" <<
        "Highpass="     << (int) getParameterMapped (PPAR (drumNumber, PP_MAIN_HIGHPASS)) << "\n" <<
        "Resonance="    << (int) getParameterMapped (PPAR (drumNumber, PP_MAIN_RESONANCE)) << "\n" <<
        "FilterEnv="    << writeEnvelopeToString (drumNumber, PP_MAIN_ENV_T1TIME) << "\n\n" <<

        "[Tone]"        << "\n" <<
        "On="           << (int) getParameterMapped (PPAR (drumNumber, PP_TONE_ON)) << "\n" <<
        "Level="        << (int) getParameterMapped (PPAR (drumNumber, PP_TONE_LEVEL)) << "\n" <<
        "F1="           << getParameterMapped (PPAR (drumNumber, PP_TONE_F1)) << "\n" <<
        "F2="           << getParameterMapped (PPAR (drumNumber, PP_TONE_F2)) << "\n" <<
        "Droop="        << getParameterMapped (PPAR (drumNumber, PP_TONE_DROOP)) << "\n" <<
        "Phase="        << getParameterMapped (PPAR (drumNumber, PP_TONE_PHASE)) << "\n" <<
        "Envelope="     << writeEnvelopeToString (drumNumber, PP_TONE_ENV_T1TIME) << "\n\n" <<

        "[Noise]"       << "\n" <<
        "On="           << (int) getParameterMapped (PPAR (drumNumber, PP_NOIZ_ON)) << "\n" <<
        "Level="        << (int) getParameterMapped (PPAR (drumNumber, PP_NOIZ_LEVEL)) << "\n" <<
        "Slope="        << (int) getParameterMapped (PPAR (drumNumber, PP_NOIZ_SLOPE)) << "\n" <<
        "FixedSeq="     << (int) getParameterMapped (PPAR (drumNumber, PP_NOIZ_FIXEDSEQ)) << "\n" <<
        "Envelope="     << writeEnvelopeToString (drumNumber, PP_NOIZ_ENV_T1TIME) << "\n\n" <<

        "[Overtones]"   << "\n" <<
        "On="           << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_ON)) << "\n" <<
        "Level="        << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_LEVEL)) << "\n" <<
        "F1="           << getParameterMapped (PPAR (drumNumber, PP_OTON_F1)) << "\n" <<
        "Wave1="        << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_WAVE1)) << "\n" <<
        "Track1="       << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_TRACK1)) << "\n" <<
        "F2="           << getParameterMapped (PPAR (drumNumber, PP_OTON_F2)) << "\n" <<
        "Wave2="        << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_WAVE2)) << "\n" <<
        "Track2="       << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_TRACK2)) << "\n" <<
        "Method="       << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_METHOD)) << "\n" <<
        "Param="        << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_PARAM)) << "\n" <<
        "Filter="       << (int) getParameterMapped (PPAR (drumNumber, PP_OTON_FILTER)) << "\n" <<
        "Envelope1="    << writeEnvelopeToString (drumNumber, PP_OTON1_ENV_T1TIME) << "\n" <<
        "Envelope2="    << writeEnvelopeToString (drumNumber, PP_OTON2_ENV_T1TIME) << "\n\n" <<

        "[Noiseband]"   << "\n" <<
        "On="           << (int) getParameterMapped (PPAR (drumNumber, PP_NBA1_ON)) << "\n" <<
        "Level="        << (int) getParameterMapped (PPAR (drumNumber, PP_NBA1_LEVEL)) << "\n" <<
        "F="            << getParameterMapped (PPAR (drumNumber, PP_NBA1_F)) << "\n" <<
        "dF="           << (int) getParameterMapped (PPAR (drumNumber, PP_NBA1_DF)) << "\n" <<
        "Envelope="     << writeEnvelopeToString (drumNumber, PP_NBA1_ENV_T1TIME) << "\n\n" <<

        "[Noiseband2]"  << "\n" <<
        "On="           << (int) getParameterMapped (PPAR (drumNumber, PP_NBA2_ON)) << "\n" <<
        "Level="        << (int) getParameterMapped (PPAR (drumNumber, PP_NBA2_LEVEL)) << "\n" <<
        "F="            << getParameterMapped (PPAR (drumNumber, PP_NBA2_F)) << "\n" <<
        "dF="           << (int) getParameterMapped (PPAR (drumNumber, PP_NBA2_DF)) << "\n" <<
        "Envelope="     << writeEnvelopeToString (drumNumber, PP_NBA2_ENV_T1TIME) << "\n\n" <<

        "[Dist]"        << "\n" <<
        "On="           << (int) getParameterMapped (PPAR (drumNumber, PP_DIST_ON)) << "\n" <<
        "Clipping="     << (int) getParameterMapped (PPAR (drumNumber, PP_DIST_CLIPPING)) << "\n" <<
        "Bits="         << (int) getParameterMapped (PPAR (drumNumber, PP_DIST_BITS)) << "\n" <<
        "Rate="         << (int) getParameterMapped (PPAR (drumNumber, PP_DIST_RATE)) << "\n";

//    DBG (content);

    file.replaceWithText (content);
}

//==============================================================================
void DrumSynthPlugin::readEnvelopeFromString (const int drumNumber, const int parameterOffset, const String& envelope)
{
    int currentPos = 0,
        currentEnvelopePoint = 0,
        currentParameter = parameterOffset;

    while (currentPos < envelope.length())
    {
        int nextPos = envelope.indexOf (currentPos, ",");
        String timeValue = envelope.substring (currentPos, nextPos);

        currentPos = nextPos + 1;

        nextPos = envelope.indexOf (currentPos, " ");
        String gainValue = envelope.substring (currentPos, nextPos < 0 ? envelope.length() : nextPos);

        setParameterMapped (PPAR (drumNumber, currentParameter++), timeValue.getFloatValue ());
        setParameterMapped (PPAR (drumNumber, currentParameter++), gainValue.getFloatValue ());

        currentPos = nextPos + 1;

        if (++currentEnvelopePoint == 5 || nextPos < 0)
            break;
    }
}

String DrumSynthPlugin::writeEnvelopeToString (const int drumNumber, const int parameterOffset)
{
    int currentEnvelopePoint = 0,
        currentParameter = parameterOffset;

    String envelope;

    while (currentEnvelopePoint++ < 5)
    {
        envelope << (int) getParameterMapped (PPAR (drumNumber, currentParameter++)) << ",";
        envelope << (int) getParameterMapped (PPAR (drumNumber, currentParameter++)) << " ";
    }

    return envelope.trim ();
}

//==============================================================================
DrumSynthComponent* DrumSynthPlugin::getEditor()
{
    return (DrumSynthComponent*) getActiveEditor();
}

AudioProcessorEditor* DrumSynthPlugin::createEditor()
{
    return new DrumSynthComponent (this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter ()
{
    return new DrumSynthPlugin();
}


AudioProcessor* JUCE_CALLTYPE createPluginFilter (const String& createArgs)
{
    return new DrumSynthPlugin();
}

