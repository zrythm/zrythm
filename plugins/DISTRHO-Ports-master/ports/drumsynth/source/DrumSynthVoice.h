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

#ifndef __JUCETICE_DRUMSYNTHVOICE_HEADER__
#define __JUCETICE_DRUMSYNTHVOICE_HEADER__

#include "DrumSynthGlobals.h"

class DrumSynthPlugin;
class DrumSynthComponent;

#define TwoPi   6.2831853f
#define MAX     0
#define ENV     1
#define PNT     2
#define dENV    3
#define NEXTT   4

//==============================================================================
/**
    Our demo synth only has one type of sound, and it's very basic..
*/
class DrumSynthSound : public SynthesiserSound
{
public:

    DrumSynthSound (const BigInteger& midiNotes_)
        : midiNotes (midiNotes_)
    {
    }

    bool appliesToNote (const int midiNoteNumber) override
    {
        return midiNotes [midiNoteNumber];
    }

    bool appliesToChannel (const int midiChannel) override
    {
        return true;
    }

private:

    // this will contain the notes that this sound is attached to.
    BigInteger midiNotes;
};


//==============================================================================
/**
    A simple plugin voice that just applies a gain change to its input.
*/
class DrumSynthVoice : public SynthesiserVoice
{
public:

    //==============================================================================
    DrumSynthVoice (const int numVoice_, DrumSynthPlugin* plugin_)
        : plugin (plugin_),
          numVoice (numVoice_),
          noteNumber (0),
          angleDelta (0.0),
          tailOff (0.0)
    {
        mem_t = 1.0f, mem_o = 1.0f, mem_n = 1.0f, mem_b = 1.0f, mem_tune = 0.0f, mem_time = 1.0f;

        DD = 0;
        DF = 0;
        phi = 0;
    }

    //==============================================================================
    void prepareToPlay (double newSampleRate, int samplesPerBlock)
    {
        DD = new short [samplesPerBlock * 2];
        DF = new float [samplesPerBlock * 2];
        phi = new float [samplesPerBlock * 2];
    }

    void releaseResources ()
    {
        delete[] DD;
        delete[] DF;
        delete[] phi;
    }

    //==============================================================================
    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast <DrumSynthSound*> (sound) != 0;
    }

    void startNote (const int midiNoteNumber, const float velocity,
                    SynthesiserSound* sound, const int currentPitchWheelPosition) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        double cyclesPerSample = MidiMessage::getMidiNoteInHertz (midiNoteNumber) / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * double_Pi;

//********************************************
        TphiStart = 0.f, TT = 0.f, TTT = 0.f;
        BdF = 0.f, BdF2 = 0.f;
        OF1Sync = 0, OF2Sync = 0;
        Oc0 = 0.0f, Oc1 = 0.0f, Oc2 = 0.0f;
        MFin = 0.f, MFout = 0.f;

        noteNumber = midiNoteNumber - START_DRUM_NOTES_OFFSET;

        timestretch = .01f * mem_time * plugin->params [PPAR(noteNumber, PP_MAIN_STRETCH)].getValueMapped ();
        timestretch = jmin (10.f, jmax (0.2f, timestretch));

        DGain = (float) pow (10.0, 0.05 * plugin->params [PPAR(noteNumber, PP_MAIN_GAIN)].getValueMapped ());

        MasterTune = plugin->params [PPAR(noteNumber, PP_MAIN_TUNING)].getValueMapped ();
        MasterTune = (float)pow(1.0594631f, MasterTune + mem_tune);

        MainFilter = 2 * plugin->params [PPAR(noteNumber, PP_MAIN_FILTER)].getIntValueMapped ();

        MFres = 0.0101f * plugin->params [PPAR(noteNumber, PP_MAIN_RESONANCE)].getValueMapped ();
        MFres = (float)pow(MFres, 0.5f);

        HighPass = plugin->params [PPAR(noteNumber, PP_MAIN_HIGHPASS)].getIntValueMapped ();
//        getEnv (7, "0,100 442000,100 443000,0"); /* FilterEnv */

        envpts[7][0][0] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T1TIME)].getIntValueMapped ();
        envpts[7][1][0] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T1GAIN)].getIntValueMapped ();
        envpts[7][0][1] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T2TIME)].getIntValueMapped ();
        envpts[7][1][1] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T2GAIN)].getIntValueMapped ();
        envpts[7][0][2] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T3TIME)].getIntValueMapped ();
        envpts[7][1][2] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T3GAIN)].getIntValueMapped ();
        envpts[7][0][3] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T4TIME)].getIntValueMapped ();
        envpts[7][1][3] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T4GAIN)].getIntValueMapped ();
        envpts[7][0][4] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T5TIME)].getIntValueMapped ();
        envpts[7][1][4] = plugin->params [PPAR(noteNumber, PP_MAIN_ENV_T5GAIN)].getIntValueMapped ();
        envpts[7][0][5] = -1;

        // TONE --------------
        chkOn[0] = plugin->params [PPAR(noteNumber, PP_TONE_ON)].getIntValueMapped (); TON = chkOn[0];
        sliLev[0] = plugin->params [PPAR(noteNumber, PP_TONE_LEVEL)].getIntValueMapped ();
        TL = (float)(sliLev[0] * sliLev[0]) * mem_t;
//        getEnv (1, "0,100 819,81 4395,71 7077,29 10708,0"); /* Tone Envelope */

        envpts[1][0][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T1TIME));
        envpts[1][1][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T1GAIN));
        envpts[1][0][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T2TIME));
        envpts[1][1][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T2GAIN));
        envpts[1][0][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T3TIME));
        envpts[1][1][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T3GAIN));
        envpts[1][0][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T4TIME));
        envpts[1][1][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T4GAIN));
        envpts[1][0][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T5TIME));
        envpts[1][1][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_TONE_ENV_T5GAIN));
        envpts[1][0][5] = -1;

        F1 = MasterTune * TwoPi * plugin->params [PPAR(noteNumber, PP_TONE_F1)].getValueMapped () / getSampleRate();
        if (fabs (F1) < 0.001f) F1 = 0.001f; // to prevent overtone ratio div0
        F2 = MasterTune * TwoPi * plugin->params [PPAR(noteNumber, PP_TONE_F2)].getValueMapped () / getSampleRate();
        Fsync = F2;
        TDroopRate = plugin->params [PPAR(noteNumber, PP_TONE_DROOP)].getValueMapped ();
        if (TDroopRate > 0.f)
        {
            TDroopRate = (float) pow (10.0f, (TDroopRate - 20.0f) / 30.0f);
            TDroopRate = TDroopRate * -4.f / envData[1][MAX];
            TDroop = 1;
            F2 = F1 + ((F2 - F1) / (1.f - (float) exp (TDroopRate * envData[1][MAX])));
            ddF = F1 - F2;
        }
        else
        {
            TDroop = 0;
            ddF = F2 - F1;
        }
        Tphi = plugin->params [PPAR(noteNumber, PP_TONE_PHASE)].getValueMapped () / 57.29578f; // degrees > radians

        // NOISE --------------
        chkOn[1] = plugin->params [PPAR(noteNumber, PP_NOIZ_ON)].getIntValueMapped (); NON = chkOn[1];
        sliLev[1] = plugin->params [PPAR(noteNumber, PP_NOIZ_LEVEL)].getIntValueMapped ();
        NT = plugin->params [PPAR(noteNumber, PP_NOIZ_SLOPE)].getIntValueMapped ();
//        getEnv (2, "0,100 4500,30 9000,0"); /* Noise Envelope */

        envpts[2][0][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T1TIME));
        envpts[2][1][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T1GAIN));
        envpts[2][0][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T2TIME));
        envpts[2][1][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T2GAIN));
        envpts[2][0][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T3TIME));
        envpts[2][1][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T3GAIN));
        envpts[2][0][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T4TIME));
        envpts[2][1][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T4GAIN));
        envpts[2][0][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T5TIME));
        envpts[2][1][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_NOIZ_ENV_T5GAIN));
        envpts[2][0][5] = -1;

        NL = (float)(sliLev[1] * sliLev[1]) * mem_n;
        if (NT < 0)
        {
            a = 1.f + (NT / 105.f);
            b = 0.f;
            c = 0.f;
            d = -NT / 105.f;
            g = (1.f + 0.0005f * NT * NT) * NL;
        }
        else
        {
            a = 1.f;
            b = -NT / 50.f;
            c = (float)fabs((float)NT) / 100.f;
            d = 0.f;
            g = NL;
        }
        x[0] = 0.f, x[1] = 0.f, x[2] = 0.f;
        if (plugin->params [PPAR(noteNumber, PP_NOIZ_FIXEDSEQ)].getIntValueMapped () != 0)
            srand(1); // fixed random sequence

        // OVERTONES --------------
        chkOn[2] = plugin->params [PPAR(noteNumber, PP_OTON_ON)].getIntValueMapped (); OON = chkOn[2];
        sliLev[2] = plugin->params [PPAR(noteNumber, PP_OTON_LEVEL)].getIntValueMapped ();
        OL = (float)(sliLev[2] * sliLev[2]) * mem_o;

//        getEnv(3, "0,100 3750,30 26254,0"); /* Overtones Envelope1 */
        envpts[3][0][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T1TIME));
        envpts[3][1][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T1GAIN));
        envpts[3][0][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T2TIME));
        envpts[3][1][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T2GAIN));
        envpts[3][0][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T3TIME));
        envpts[3][1][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T3GAIN));
        envpts[3][0][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T4TIME));
        envpts[3][1][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T4GAIN));
        envpts[3][0][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T5TIME));
        envpts[3][1][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON1_ENV_T5GAIN));
        envpts[3][0][5] = -1;

//        getEnv(4, "0,100 3000,30 22605,62 27364,0"); /* Overtones Envelope2 */
        envpts[4][0][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T1TIME));
        envpts[4][1][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T1GAIN));
        envpts[4][0][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T2TIME));
        envpts[4][1][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T2GAIN));
        envpts[4][0][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T3TIME));
        envpts[4][1][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T3GAIN));
        envpts[4][0][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T4TIME));
        envpts[4][1][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T4GAIN));
        envpts[4][0][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T5TIME));
        envpts[4][1][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_OTON2_ENV_T5GAIN));
        envpts[4][0][5] = -1;

        OMode = /* Overtones Method 0..2 */ plugin->params [PPAR(noteNumber, PP_OTON_METHOD)].getIntValueMapped ();
        OF1 = MasterTune * TwoPi * plugin->params [PPAR(noteNumber, PP_OTON_F1)].getValueMapped () / getSampleRate();
        OF2 = MasterTune * TwoPi * plugin->params [PPAR(noteNumber, PP_OTON_F2)].getValueMapped () / getSampleRate();
        OW1 = plugin->params [PPAR(noteNumber, PP_OTON_WAVE1)].getIntValueMapped ();
        OW2 = plugin->params [PPAR(noteNumber, PP_OTON_WAVE2)].getIntValueMapped ();
        OBal2 = (float) plugin->params [PPAR(noteNumber, PP_OTON_PARAM)].getIntValueMapped ();
        ODrive = (float) pow (OBal2, 3.0f) / (float) pow (50.0f, 3.0f);
        OBal2 *= 0.01f;
        OBal1 = 1.f - OBal2;
        Ophi1 = Tphi;
        Ophi2 = Tphi;
        if (MainFilter == 0)
            MainFilter = plugin->params [PPAR(noteNumber, PP_OTON_FILTER)].getIntValueMapped ();
        if((plugin->params [PPAR(noteNumber, PP_OTON_TRACK1)].getIntValueMapped () == 1) && (TON==1))
        {
            OF1Sync = 1;
            OF1 = OF1 / F1;
        }
        if((plugin->params [PPAR(noteNumber, PP_OTON_TRACK2)].getIntValueMapped () == 1) && (TON==1))
        {
            OF2Sync = 1;
            OF2 = OF2 / F1;
        }

        OcA = 0.28f + OBal1 * OBal1;  //overtone cymbal mode
        OcQ = OcA * OcA;
        OcF = (1.8f - 0.7f * OcQ) * 0.92f; //multiply by env 2
        OcA *= 1.0f + 4.0f * OBal1;  //level is a compromise!
        Ocf1 = TwoPi / OF1;
        Ocf2 = TwoPi / OF2;
        for (int i = 0; i < 6; i++)
            Oc[i][0] = Oc[i][1] = Ocf1 + (Ocf2 - Ocf1) * 0.2f * (float)i;

        // NOISE BAND 1 --------------
        chkOn[3] = plugin->params [PPAR(noteNumber, PP_NBA1_ON)].getIntValueMapped (); BON = chkOn[3];
        sliLev[3] = plugin->params [PPAR(noteNumber, PP_NBA1_LEVEL)].getIntValueMapped ();
        BL = (float)(sliLev[3] * sliLev[3]) * mem_b;
        BF = MasterTune * TwoPi * plugin->params [PPAR(noteNumber, PP_NBA1_F)].getValueMapped () / getSampleRate();
        BPhi = TwoPi / 8.f;
//        getEnv (5, "0,100 2250,30 4500,0"); /* Noiseband1 Envelope */

        envpts[5][0][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T1TIME));
        envpts[5][1][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T1GAIN));
        envpts[5][0][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T2TIME));
        envpts[5][1][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T2GAIN));
        envpts[5][0][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T3TIME));
        envpts[5][1][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T3GAIN));
        envpts[5][0][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T4TIME));
        envpts[5][1][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T4GAIN));
        envpts[5][0][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T5TIME));
        envpts[5][1][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA1_ENV_T5GAIN));
        envpts[5][0][5] = -1;

        BFStep = plugin->params [PPAR(noteNumber, PP_NBA1_DF)].getIntValueMapped ();
        BQ = (float) BFStep;
        BQ = BQ * BQ / (10000.f - 6600.f * ((float) sqrt(BF) - 0.19f));
        BFStep = 1 + (int)((40.f - (BFStep / 2.5f)) / (BQ + 1.f + (1.f * BF)));

        // NOISE BAND 2 --------------
        chkOn[4] = plugin->params [PPAR(noteNumber, PP_NBA2_ON)].getIntValueMapped (); BON2 = chkOn[4];
        sliLev[4] = plugin->params [PPAR(noteNumber, PP_NBA2_LEVEL)].getIntValueMapped ();
        BL2 = (float)(sliLev[4] * sliLev[4]) * mem_b;
        BF2 = MasterTune * TwoPi * plugin->params [PPAR(noteNumber, PP_NBA2_F)].getValueMapped () / getSampleRate();
        BPhi2 = TwoPi / 8.f;
//        getEnv(6, "0,100 100,30 200,0"); /* Noiseband2 Envelope */

        envpts[6][0][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T1TIME));
        envpts[6][1][0] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T1GAIN));
        envpts[6][0][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T2TIME));
        envpts[6][1][1] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T2GAIN));
        envpts[6][0][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T3TIME));
        envpts[6][1][2] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T3GAIN));
        envpts[6][0][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T4TIME));
        envpts[6][1][3] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T4GAIN));
        envpts[6][0][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T5TIME));
        envpts[6][1][4] = plugin->getParameterMapped (PPAR(noteNumber, PP_NBA2_ENV_T5GAIN));
        envpts[6][0][5] = -1;

        BFStep2 = plugin->params [PPAR(noteNumber, PP_NBA2_DF)].getIntValueMapped ();
        BQ2 = (float) BFStep2;
        BQ2 = BQ2 * BQ2 / (10000.f - 6600.f * ((float) sqrt(BF2) - 0.19f));
        BFStep2 = 1 + (int)((40 - (BFStep2 / 2.5)) / (BQ2 + 1 + (1 * BF2)));

        // DISTORTION --------------
        chkOn[5] = plugin->params [PPAR(noteNumber, PP_NOIZ_ON)].getIntValueMapped (); DiON = chkOn[5];
        DStep = 1 + plugin->params [PPAR(noteNumber, PP_DIST_RATE)].getIntValueMapped ();
        if (DStep == 7) DStep = 20;
        if (DStep == 6) DStep = 10;
        if (DStep == 5) DStep = 8;
        clippoint = 32700;
        DAtten = 1.0f;
        if (DiON == 1)
        {
            DAtten = DGain * (short) loudestEnv();
            if (DAtten > 32700)
                clippoint = 32700;
            else
                clippoint = (short) DAtten;

            DAtten = (float) pow (2.0, 2.0 * plugin->params [PPAR(noteNumber, PP_DIST_BITS)].getIntValueMapped ());
            DGain *= DAtten * (float) pow (10.0, 0.05 * plugin->params [PPAR(noteNumber, PP_DIST_CLIPPING)].getIntValueMapped ());
        }

        // prepare envelopes
        randmax = 1.f / RAND_MAX;
        randmax2 = 2.f * randmax;
        for (int i = 1; i < 8; i++)
        {
            envData[i][NEXTT] = 0;
            envData[i][PNT] = 0;
        }

        Length = longestEnv();
        tpos = 0;
    }

    void stopNote (float, const bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.

            if (tailOff == 0.0) // we only need to begin a tail-off if it's not already doing so - the
                                // stopNote method could be called more than once.
                tailOff = 1.0;
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..

            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    //==============================================================================
    void pitchWheelMoved (const int newValue) override
    {
        // can't be bothered implementing this for the demo!
    }

    void controllerMoved (const int controllerNumber, const int newValue) override
    {
        // not interested in controllers in this case.
    }

    //==============================================================================
    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0 && tpos < Length)
        {
            int t;
            tplus = tpos + (numSamples - 1);

            if(NON==1) //noise
            {
              for(t=tpos; t<=tplus; t++)
              {
                if(t < envData[2][NEXTT]) envData[2][ENV] = envData[2][ENV] + envData[2][dENV];
                else updateEnv(2, t);
                x[2] = x[1];
                x[1] = x[0];
                x[0] = (randmax2 * (float)rand()) - 1.f;
                TT = a * x[0] + b * x[1] + c * x[2] + d * TT;
                DF[t - tpos] = TT * g * envData[2][ENV];
              }
              if(t>=envData[2][MAX]) NON=0;
            }
            else for(int j=0; j<numSamples; j++) DF[j]=0.f;

            if(TON==1) //tone
            {
              TphiStart = Tphi;
              if(TDroop==1)
              {
                for(t=tpos; t<=tplus; t++)
                  phi[t - tpos] = F2 + (ddF * (float)exp(t * TDroopRate));
              }
              else
              {
                for(t=tpos; t<=tplus; t++)
                  phi[t - tpos] = F1 + (t / envData[1][MAX]) * ddF;
              }
              for(t=tpos; t<=tplus; t++)
              {
                int totmp = t - tpos;
                if(t < envData[1][NEXTT])
                  envData[1][ENV] = envData[1][ENV] + envData[1][dENV];
                else updateEnv(1, t);
                Tphi = Tphi + phi[totmp];
                DF[totmp] += TL * envData[1][ENV] * (float)sin(fmod(Tphi,TwoPi));//overflow?
              }
              if(t>=envData[1][MAX]) TON=0;
            }
            else for(int j=0; j<numSamples; j++) phi[j]=F2; //for overtone sync

            if(BON==1) //noise band 1
            {
              for(t=tpos; t<=tplus; t++)
              {
                if(t < envData[5][NEXTT])
                  envData[5][ENV] = envData[5][ENV] + envData[5][dENV];
                else updateEnv(5, t);
                if((t % BFStep) == 0) BdF = randmax * (float)rand() - 0.5f;
                BPhi = BPhi + BF + BQ * BdF;
                botmp = t - tpos;
                DF[botmp] = DF[botmp] + (float)cos(fmod(BPhi,TwoPi)) * envData[5][ENV] * BL;
              }
              if(t>=envData[5][MAX]) BON=0;
            }

            if(BON2==1) //noise band 2
            {
              for(t=tpos; t<=tplus; t++)
              {
                if(t < envData[6][NEXTT])
                  envData[6][ENV] = envData[6][ENV] + envData[6][dENV];
                else updateEnv(6, t);
                if((t % BFStep2) == 0) BdF2 = randmax * (float)rand() - 0.5f;
                BPhi2 = BPhi2 + BF2 + BQ2 * BdF2;
                botmp = t - tpos;
                DF[botmp] = DF[botmp] + (float)cos(fmod(BPhi2,TwoPi)) * envData[6][ENV] * BL2;
              }
              if(t>=envData[6][MAX]) BON2=0;
            }

            for (t=tpos; t<=tplus; t++)
            {
              if(OON==1) //overtones
              {
                if(t<envData[3][NEXTT])
                  envData[3][ENV] = envData[3][ENV] + envData[3][dENV];
                else
                {
                  if(t>=envData[3][MAX]) //wait for OT2
                  {
                    envData[3][ENV] = 0;
                    envData[3][dENV] = 0;
                    envData[3][NEXTT] = 999999;
                  }
                  else updateEnv(3, t);
                }
                //
                if(t<envData[4][NEXTT])
                  envData[4][ENV] = envData[4][ENV] + envData[4][dENV];
                else
                {
                  if(t>=envData[4][MAX]) //wait for OT1
                  {
                    envData[4][ENV] = 0;
                    envData[4][dENV] = 0;
                    envData[4][NEXTT] = 999999;
                  }
                  else updateEnv(4, t);
                }
                //
                TphiStart = TphiStart + phi[t - tpos];
                if(OF1Sync==1) Ophi1 = TphiStart * OF1; else Ophi1 = Ophi1 + OF1;
                if(OF2Sync==1) Ophi2 = TphiStart * OF2; else Ophi2 = Ophi2 + OF2;
                Ot=0.0f;
                switch (OMode)
                {
                  case 0: //add
                    Ot = OBal1 * envData[3][ENV] * getWaveform (Ophi1, OW1);
                    Ot = OL * (Ot + OBal2 * envData[4][ENV] * getWaveform (Ophi2, OW2));
                    break;

                  case 1: //FM
                    Ot = ODrive * envData[4][ENV] * getWaveform (Ophi2, OW2);
                    Ot = OL * envData[3][ENV] * getWaveform (Ophi1 + Ot, OW1);
                    break;

                  case 2: //RM
                    Ot = (1 - ODrive / 8) + (((ODrive / 8) * envData[4][ENV]) * getWaveform (Ophi2, OW2));
                    Ot = OL * envData[3][ENV] * getWaveform (Ophi1, OW1) * Ot;
                    break;

                  case 3: //808 Cymbal
                    for(int j=0; j<6; j++)
                    {
                      Oc[j][0] += 1.0f;

                      if(Oc[j][0]>Oc[j][1])
                      {
                        Oc[j][0] -= Oc[j][1];
                        Ot = OL * envData[3][ENV];
                      }
                    }
                    Ocf1 = envData[4][ENV] * OcF;  //filter freq
                    Oc0 += Ocf1 * Oc1;
                    Oc1 += Ocf1 * (Ot + Oc2 - OcQ * Oc1 - Oc0);  //bpf
                    Oc2 = Ot;
                    Ot = Oc1;
                    break;
                }
              }

              if(MainFilter==1) //filter overtones
              {
                if(t<envData[7][NEXTT])
                  envData[7][ENV] = envData[7][ENV] + envData[7][dENV];
                else updateEnv(7, t);

                MFtmp = envData[7][ENV];
                if(MFtmp >0.2f)
                  MFfb = 1.001f - (float)pow(10.0f, MFtmp - 1);
                else
                  MFfb = 0.999f - 0.7824f * MFtmp;

                MFtmp = Ot + MFres * (1.f + (1.f/MFfb)) * (MFin - MFout);
                MFin = MFfb * (MFin - MFtmp) + MFtmp;
                MFout = MFfb * (MFout - MFin) + MFin;

                DF[t - tpos] = DF[t - tpos] + (MFout - (HighPass * Ot));
              }
              else if(MainFilter==2) //filter all
              {
                if(t<envData[7][NEXTT])
                  envData[7][ENV] = envData[7][ENV] + envData[7][dENV];
                else updateEnv(7, t);

                MFtmp = envData[7][ENV];
                if(MFtmp >0.2f)
                  MFfb = 1.001f - (float)pow(10.0f, MFtmp - 1);
                else
                  MFfb = 0.999f - 0.7824f * MFtmp;

                MFtmp = DF[t - tpos] + Ot + MFres * (1.f + (1.f/MFfb)) * (MFin - MFout);
                MFin = MFfb * (MFin - MFtmp) + MFtmp;
                MFout = MFfb * (MFout - MFin) + MFin;

                DF[t - tpos] = MFout - (HighPass * (DF[t - tpos] + Ot));
              }
              else DF[t - tpos] = DF[t - tpos] + Ot; //no filter
            }

            // bit resolution
            if (DiON == 1)
            {
                for (int j = 0; j < numSamples; j++)
                    DF[j] = DGain * (int)(DF[j] / DAtten);

                // downsampling
                for (int j = 0; j < numSamples; j += DStep)
                {
                    DownAve = 0;
                    DownStart = j;
                    DownEnd = j + DStep - 1;
                    for(int jj = DownStart; jj <= DownEnd; jj++)
                        DownAve = DownAve + DF[jj];
                    DownAve = DownAve / DStep;
                    for(int jj = DownStart; jj <= DownEnd; jj++)
                        DF[jj] = DownAve;
                }
            }
            else
                for (int j = 0; j < numSamples; j++)
                    DF[j] *= DGain;

            // render output
            const float scale = 1.0f / 0xffff;
            float* wave = outputBuffer.getWritePointer (0, startSample);

            if (tailOff > 0)
            {
                for (int j = 0; j < numSamples; j++) //clipping + output
                {
                    float thisGain = level * tailOff;

                    if (DF[j] > clippoint)
                    {
                        wave[j] += (float)(scale * clippoint) * thisGain;
                    }
                    else
                    {
                        if(DF[j] < -clippoint)
                            wave[j] += (float)(scale * -clippoint) * thisGain;
                        else
                            wave[j] += (float)(scale * (short) DF[j]) * thisGain;
                    }

                    currentAngle += angleDelta;

                    tailOff *= 0.999;

                    if (tailOff <= 0.001)
                    {
                        clearCurrentNote();
                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                for (int j = 0; j < numSamples; j++) //clipping + output
                {
                    float thisGain = level;

                    if (DF[j] > clippoint)
                    {
                        wave[j] += (float)(scale * clippoint) * thisGain;
                    }
                    else
                    {
                        if(DF[j] < -clippoint)
                            wave[j] += (float)(scale * -clippoint) * thisGain;
                        else
                            wave[j] += (float)(scale * (short) DF[j]) * thisGain;
                    }

                    currentAngle += angleDelta;
                }
            }

            tpos += numSamples;
        }

        if (tpos >= Length)
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

private:
    DrumSynthPlugin* plugin;

    int numVoice, noteNumber;
    double currentAngle, angleDelta, level, tailOff;

private:

    //==============================================================================
    inline void getEnv (int env, const char* en = "0,0 100,0")
    {
      char s[8];
      int i=0, o=0, ep=0;

      while(en[i]!=0)
      {
        if(en[i] == ',')
        {
          if(sscanf(s, "%f", &envpts[env][0][ep])==0) envpts[env][0][ep] = 0.f;
          o=0;
        }
        else if(en[i] == ' ')
        {
          if(sscanf(s, "%f", &envpts[env][1][ep])==0) envpts[env][1][ep] = 0.f;
          o=0; ep++;
        }
        else { s[o]=en[i]; o++; s[o]=0; }
        i++;
      }
      if(sscanf(s, "%f", &envpts[env][1][ep])==0) envpts[env][1][ep] = 0.f;
      envpts[env][0][ep + 1] = -1;

      envData[env][MAX] = envpts[env][0][ep];
    }

    //==============================================================================
    inline int longestEnv ()
    {
      long eon, p;
      float l = 0.f;

      for (int e = 1; e < 7; e++) // 3
      {
        eon = e - 1;
        if (eon > 2)
            eon = eon - 1;
        p = 0;

        while (envpts[e][0][p + 1] >= 0.f)
            p++;

        envData[e][MAX] = envpts[e][0][p] * timestretch;
        if (chkOn[eon] == 1)
            if (envData[e][MAX] > l)
                l = envData[e][MAX];
      }
      //l *= timestretch;

      return 2400 + (1200 * (int)(l / 1200));
    }

    //==============================================================================
    inline float loudestEnv ()
    {
      float loudest = 0.f;
      int i = 0;

      while (i < 5) // 2
      {
        if (chkOn[i] == 1)
            if (sliLev[i] > loudest)
                loudest = (float) sliLev[i];
        i++;
      }
      return (loudest * loudest);
    }

    //==============================================================================
    inline void updateEnv (int e, long t)
    {
      float endEnv, dT;

      envData[e][NEXTT] = envpts[e][0][(long)(envData[e][PNT] + 1.f)] * timestretch; //get next point

      if(envData[e][NEXTT] < 0)
        envData[e][NEXTT] = 442000 * timestretch; //if end point, hold

      envData[e][ENV] = envpts[e][1][(long)(envData[e][PNT] + 0.f)] * 0.01f; //this level
      endEnv = envpts[e][1][(long)(envData[e][PNT] + 1.f)] * 0.01f;          //next level

      dT = envData[e][NEXTT] - (float)t;
      if(dT < 1.0)
        dT = 1.0;

      envData[e][dENV] = (endEnv - envData[e][ENV]) / dT;
      envData[e][PNT] = envData[e][PNT] + 1.0f;
    }

    //==============================================================================
    inline float getWaveform (float ph, int form)
    {
      float w;

      switch (form)
      {
        case 0:  w = (float) sin (fmod (ph, TwoPi));                                    break; //sine
        case 1:  w = (float) fabs (2.0f * (float) sin (fmod (0.5f * ph, TwoPi))) - 1.f; break; //sine^2
        case 2:  while (ph < TwoPi) ph += TwoPi;
                 w = 0.6366197f * (float) fmod (ph, TwoPi) - 1.f;
                 if (w > 1.f) w = 2.f - w;                                              break; //tri
        case 3:  w = ph - TwoPi * (float)(int)(ph / TwoPi);
                 w = (0.3183098f * w) - 1.f;                                            break; //saw
        default: w = (sin (fmod (ph, TwoPi)) > 0.0) ? 1.f: -1.f;                        break; //square
      }

      return w;
    }

    //==============================================================================
    float envpts[8][3][32];    //envelope/time-level/point
    float envData[8][6];       //envelope running status
    int   chkOn[8], sliLev[8]; //section on/off and level
    float timestretch;         //overall time scaling
    short clippoint;
    short* DD;
    float* DF;
    float* phi;

    float mem_t, mem_o, mem_n, mem_b, mem_tune, mem_time;

    long  Length, tpos, tplus;
    float x[3];
    float MasterTune, randmax, randmax2;
    int   MainFilter, HighPass;

    long  NON, NT, TON, DiON, TDroop, DStep;
    float a, b, c, d, g, TT, TTT, TL, NL, F1, F2, Fsync;
    float TphiStart, Tphi, TDroopRate, ddF, DAtten, DGain;

    long  BON, BON2, BFStep, BFStep2, botmp;
    float BdF, BdF2, BPhi, BPhi2, BF, BF2, BQ, BQ2, BL, BL2;

    long  OON, OF1Sync, OF2Sync, OMode, OW1, OW2;
    float Ophi1, Ophi2, OF1, OF2, OL, Ot, OBal1, OBal2, ODrive;
    float Ocf1, Ocf2, OcF, OcQ, OcA, Oc[6][2];  //overtone cymbal mode
    float Oc0, Oc1, Oc2;

    float MFfb, MFtmp, MFres, MFin, MFout;
    float DownAve;
    long  DownStart, DownEnd;
};



#endif
