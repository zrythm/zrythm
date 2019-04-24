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

#ifndef Vco_H
#define Vco_H

#include "Osc.h"
#include "audioutils.h"

class Vco
{
private:
	Osc *osc1;
	Osc *osc2;
	Osc *osc3;

	float oldNoteValue;
	float currentFrequency;

	float osc1Tune;
	float osc2Tune;
	float osc1FineTune;
	float osc2FineTune;

	float osc1Pw;
	float osc2Fm;

    float ringmodulation;
    float oscBitcrusher;

    bool isBitcrusherEnabled;

	AudioUtils audioUtils;

public:
	Vco(float sampleRate)
	{
		oldNoteValue = 0.0f;
		currentFrequency = 440.0f;

		osc1FineTune = 0.0f;
		osc2FineTune = 0.0f;
		osc1Tune = 12.0f;
		osc2Tune = 12.0f;

		osc1Pw = 0.5f;
		osc2Fm = 0.0f;

        this->ringmodulation = 0.0f;
        this->oscBitcrusher = 0.0f;
        this->isBitcrusherEnabled = false;

		osc3 = new Osc(sampleRate, NULL);
		osc1 = new Osc(sampleRate, osc3);
		osc2 = new Osc(sampleRate, osc3);

		osc1->setOscWaveform(Osc::PULSE);
		osc3->setOscWaveform(Osc::PULSE);

		osc1->setOscVolume(0.0f);
		osc2->setOscVolume(1.0f);
		osc3->setOscVolume(0.0f);

        this->osc2->setOscWaveform(Osc::SAW);
        this->setOsc2Phase(0.5f);
	}

	~Vco() 
	{
        delete osc1;
        delete osc2;
        delete osc3;
	}

	void resetVco()
	{
		osc3->resetOsc();
		osc1->resetOsc();
		osc2->resetOsc();
	}

	void setOsc1Volume(float value)
	{
		this->osc1->setOscVolume(value);
	}

	void setOsc2Volume(float value)
	{
		this->osc2->setOscVolume(value);
	}

	void setOsc3Volume(float value)
	{
		this->osc3->setOscVolume(value);
	}

	void setOsc1Waveform(Osc::Waveform waveform)
	{
		osc1->setOscWaveform(waveform);
	}

	void setOsc2Waveform(Osc::Waveform waveform)
	{
		osc2->setOscWaveform(waveform);
	}

	void setOsc1Tune(float value)
	{
		this->osc1Tune = audioUtils.getOscTuneValue(value);
	}

	void setOsc2Tune(float value)
	{
		this->osc2Tune = audioUtils.getOscTuneValue(value);
	}

	void setOsc1FineTune(float value)
	{
		this->osc1FineTune = audioUtils.getOscFineTuneValue(value);
	}

	void setOsc2FineTune(float value)
	{
		this->osc2FineTune = audioUtils.getOscFineTuneValue(value);
	}

	void setOscSync(bool value)
	{
		osc1->setOscSync(value);
		osc2->setOscSync(value);
	}

	void setOsc1Pw(float value)
	{
		this->osc1Pw = value;
	}

	void setOsc1Fm(float value)
	{
		this->osc2Fm = value;
	}

	void setOsc1Phase(float value)
	{
		osc1->setOscPhase(value);
	}

	void setOsc2Phase(float value)
	{
		osc2->setOscPhase(value);
	}

    void setRingmodulation(float value)
    {
        this->ringmodulation = value;
    }

    void setOscBitcrusher(float value)
    {
        if (value != 1.0f)
        {     
            this->isBitcrusherEnabled = true;
        }
        else
        {
            this->isBitcrusherEnabled = false;
        }
        this->oscBitcrusher = (float)audioUtils.getBitDepthDynamic(value);
    }

	inline void process(float *sample, const float note)
	{
		float masterNote = note - 12.0f;
		float osc1Note = note + osc1FineTune + osc1Tune;
		
		float osc2Note = note + osc2FineTune + osc2Tune;

		*sample += this->osc3->process(masterNote);
		float osc1Value = this->osc1->process(osc1Note);
		float osc2Value = this->osc2->process(osc2Note);

        float result = osc1Value + osc2Value;

        *sample += result;
	}
};
#endif