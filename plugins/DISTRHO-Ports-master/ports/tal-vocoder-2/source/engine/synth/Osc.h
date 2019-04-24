/*
==============================================================================
This file is part of Tal-NoiseMaker by Patrick Kunz.

Copyright(c) 2005-2010 Patrick Kunz, TAL
Togu Audio Line, Inc.
http://kunz.corrupt.ch

This file may be licensed under the terms of of the
GNU General Public License Version 2 (the ``GPL'').

Software distributed under the License is distributed
on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
express or implied. See the GPL for the specific language
governing rights and limitations.

You should have received a copy of the GPL along with this
program. If not, go to http://www.gnu.org/licenses/gpl.html
or write to the Free Software Foundation, Inc.,  
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
==============================================================================
*/

#ifndef Osc_H
#define Osc_H

#include "OscSaw.h"
#include "OscPulse.h"
#include "OscTriangle.h"
#include "OscSin.h"
#include "OscNoise.h"

#include "audioutils.h"

class Osc
{
public:
    enum Waveform
    {
        SAW,
        PULSE,
        TRIANGLE,
        SIN,
        NOISE,
    };

private:
    OscSaw *oscSaw;
    OscPulse *oscPulse;
    OscTriangle *oscTriangle;
    OscSin *oscSin;
    OscNoise *oscNoise;
    Osc *masterOsc;

    float oscVolume;
    bool oscSync;

    float oldNoteValue;
    float currentFrequency;

    float pw;
    float fm;
    float fmFrequency;
    float oscPhase;

    Waveform waveform;

    AudioUtils audioUtils;

public:
    Osc(float sampleRate, Osc *masterOsc)
    {
        this->masterOsc = masterOsc;

        this->oscSaw = new OscSaw(sampleRate, 1);
        this->oscPulse = new OscPulse(sampleRate, 1);
        this->oscTriangle = new OscTriangle(sampleRate, 1);
        this->oscSin = new OscSin(sampleRate, 1);
        this->oscNoise = new OscNoise(sampleRate);

        // Init default values
        this->oscVolume = 1.0f;
        this->waveform = SAW;
        this->oscSync = false;
        this->pw = 0.5f;
        this->fm = 0.0f;
        this->fmFrequency = 0.0f;
        this->oscPhase = 0.0f;

        this->oldNoteValue = -1.0f;
        this->currentFrequency = 440.0f;
    }

    ~Osc() 
    {
        delete oscSaw;
        delete oscPulse;
        delete oscTriangle;
        delete oscSin;
        delete oscNoise;
    }

    void resetOsc()
    {
        if (!isMasterOsc())
        {
            float phase = this->oscPhase;
            this->oscPulse->resetOsc(phase);
            this->oscSaw->resetOsc(phase);
            this->oscTriangle->resetOsc(phase);
            this->oscSin->resetOsc(phase);
        }
        else
        {
            this->oscPulse->resetOsc(0.0f);
            this->oscSaw->resetOsc(0.0f);
            this->oscTriangle->resetOsc(0.0f);
            this->oscSin->resetOsc(0.0f);
        }
    }

    void setOscPhase(float value)
    {
        this->oscPhase = value;
        if (!isMasterOsc())
        {
            float masterPhaseOffset = masterOsc->getMasterFrac() + this->oscPhase;

            // Warp phase if required
            if (masterPhaseOffset > 1.0f)
            {
                masterPhaseOffset -= 1.0f;
            }
            this->oscSaw->x = masterPhaseOffset;
            this->oscSin->x = masterPhaseOffset;
            this->oscPulse->x = masterPhaseOffset;
            this->oscTriangle->x = masterPhaseOffset;
        }
    }

    inline bool isMasterOsc()
    {
        return masterOsc == NULL;
    }

    void setOscVolume(float value)
    {
        this->oscVolume = value;
    }

    bool getMasterSync()
    {
        return oscPulse->phaseReset;
    }

    void setOscWaveform(Osc::Waveform waveform)
    {
        this->waveform = waveform;
    }

    void setOscSync(bool value)
    {
        this->oscSync = value;
    }

    void setPw(float value)
    {
        this->pw = value;
    }

    void setFm(float value)
    {
        this->fm = value;
    }

    void setFmFrequency(float value)
    {
        this->fmFrequency = value;
    }

    inline float getMasterFrac()
    {
        return oscPulse->x;
    }

    inline float getMasterFreq()
    {
        return oscPulse->freq;
    }

    inline float getCurrentFrequency()
    {
        return currentFrequency;
    }

    float process(float note)
    {
        // Calculate new frequency if required
        if (note != oldNoteValue)
        {
            currentFrequency = audioUtils.getMidiNoteInHertz(note) * 2.0f;
            oldNoteValue = note;
        }

        // Handle master osc
        bool sync = false;
        float masterFrac = 0.0f;
        float masterFreq = 0.0f;
        if (!isMasterOsc() && oscSync)
        {
            sync = masterOsc->getMasterSync();
            masterFrac = masterOsc->getMasterFrac();
            masterFreq = masterOsc->getMasterFreq();
        }

        // Process oscillators
        float oscValue = 0.0f;
        if (this->oscVolume > 0.0f
            || isMasterOsc() /*Always process master osc*/)
        {
            switch (waveform)
            {
            case SAW:
                oscValue = oscSaw->getNextSample(currentFrequency, fm, fmFrequency, sync, masterFrac, masterFreq);
                break;
            case PULSE:
                oscValue = oscPulse->getNextSample(currentFrequency, pw, fm, fmFrequency, sync, masterFrac, masterFreq);
                break;
            case TRIANGLE:
                oscValue = oscTriangle->getNextSample(currentFrequency, fm, fmFrequency, sync, masterFrac, masterFreq);
                break;
            case SIN:
                oscValue = oscSin->getNextSample(currentFrequency, fm, fmFrequency, false, masterFrac, masterFreq);
                break;
            case NOISE:
                oscValue = oscNoise->getNextSample();
                break;
            }

            oscValue *= this->oscVolume;
        }
        return oscValue;
    }
};
#endif