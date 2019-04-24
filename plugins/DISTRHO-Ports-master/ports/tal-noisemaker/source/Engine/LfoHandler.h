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

#ifndef LfoHandler_H
#define LfoHandler_H

#include "LfoHandler.h"
#include "Lfo.h"
#include "math.h"
#include "AudioUtils.h"

class LfoHandler
{
public:
    enum Waveform
    {
        SIN,
        TRIANGLE,
        SAW,
        RECTANGE,
        RANDOMSTEP,
        RANDOM
    };

private:
    Lfo *lfo;
    Waveform waveform;
    bool keyTrigger;
    float currentRateHz;
    float currentPhase;

    AudioUtils audioUtils;

public:
    float value;
    float amount;
    float amountPositive;
    bool isSync;

    LfoHandler(float sampleRate)
    {
        lfo = new Lfo(sampleRate);
        waveform = SIN;
        value = 0.0f;
        amount = 1.0f;
        keyTrigger = false;
        isSync = false;
        currentRateHz = 1.0f;
        currentPhase = 0.0f;
    }

    virtual ~LfoHandler()
    {
        delete lfo;
    }

    void setWaveform(float waveform)
    {
        int waveformInt = (int)(waveform * 5.000001f);
        switch (waveformInt)
        {
        case SIN: this->setWaveform(SIN); break;
        case TRIANGLE: this->setWaveform(TRIANGLE); break;
        case SAW: this->setWaveform(SAW); break;
        case RECTANGE: this->setWaveform(RECTANGE); break;
        case RANDOMSTEP: this->setWaveform(RANDOMSTEP); break;
        case RANDOM: this->setWaveform(RANDOM); break;
        }
    }

    void setWaveform(const Waveform waveform)
    {
        this->waveform = waveform;
    }

    // modulator [-1..1]
    inline void setRateMultiplier(float modulator)
    {
        if (modulator < 0.0f)
        {
            this->lfo->setRate(currentRateHz * (1.0f + modulator * 0.01f));
        }
        else
        {
            this->lfo->setRate(currentRateHz * (1.0f + modulator * 100.0f));
        }
    }

    void setRate(float rateIn, const float bpm)
    {
        if (!isSync)
        {
            currentRateHz = audioUtils.getLogScaledRate(rateIn);
            this->lfo->setRate(currentRateHz);
        }
        else
        {
            audioUtils.getSyncedRateAndGetText(&rateIn, bpm);
            currentRateHz = rateIn;
            this->lfo->setRate(rateIn);
        }
    }

    void setAmount(const float amount)
    {
        this->amount = amount;
        this->amountPositive = fabs(amount);
    }

    void setPhase(float phase)
    {
        this->currentPhase = phase;
    }

    void triggerPhase()
    {
        if (keyTrigger)
        {
            this->lfo->resetPhase(currentPhase);
        }
    }

    // Phase [0..1]
    void setHostPhase(const float phase)
    {
        if (!keyTrigger)
        {
            this->lfo->phase = 255.0f * (phase + currentPhase);
        }
    }

    void setKeyTrigger(bool value)
    {
        this->keyTrigger = value;
    }

    void setSync(bool value, float rateIn, const float bpm)
    {
        this->isSync = value;
        this->setRate(rateIn, bpm);
    }

    inline float getValue()
    {
        return value;
    }

    inline float getAmount()
    {
        return amount;
    }

    virtual float process()
    {
        return value = lfo->tick(this->waveform);
    }

    inline float getLfoInc()
    {
        return lfo->inc;
    }
};
#endif