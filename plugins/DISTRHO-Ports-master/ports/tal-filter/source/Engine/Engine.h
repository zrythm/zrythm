/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

	Copyright(c) 2005-2009 Patrick Kunz, TAL
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


#if !defined(__Engine_h)
#define __Engine_h

#include "Params.h"
#include "ParamChangeUtil.h"
#include "FilterHandler.h"
#include "Lfo.h"
#include "Envelope.h"

class Engine 
{
private:
	float bpm;

public:
	FilterHandler *filterHandlerL;
	FilterHandler *filterHandlerR;
	float *param;
	Lfo *lfoL;
	Lfo *lfoR;

	Envelope *envelope;

	float volume, inputDrive, envelopeAmount, lfoAmount;

	ParamChangeUtil* cutoffParamChange;

	Engine(float sampleRate) 
	{
		Params *params= new Params();
		this->param= params->parameters;

		filterHandlerL = NULL;
		filterHandlerR = NULL;
		cutoffParamChange = NULL;
		lfoL = NULL;
		lfoR = NULL;
		envelope = NULL;

		initialize(sampleRate);
		bpm = 120.0f;
		volume = 0.8f;
		inputDrive = 0.8f;
	}

	~Engine()
	{
		delete filterHandlerL;
		delete filterHandlerR;
		delete envelope;

		delete lfoL;
		delete lfoR;
	}

	void setSampleRate(float sampleRate)
	{
        if (sampleRate > 0)
        {
		    initialize(sampleRate);
        }
        else
        {
		    initialize(44100.0f);
        }
	}

	void setBpm(float bpm, int sync, float rateInHz)
	{
        if (bpm <= 0.0f)
        {
            bpm = 120;
        }

		if (this->bpm != bpm)
		{
			if (bpm > 1.0f && bpm < 500.0f)
			{
				this->bpm = bpm;
				setSync(sync, rateInHz);
			}
		}
	}

	void setVolume(float volume)
	{
		this->volume = volume * volume * volume * 8.0f;
	}

	void setLfoAmount(float lfoAmount)
	{
		this->lfoAmount = 2.0f * (lfoAmount - 0.5f);
		this->lfoAmount = this->lfoAmount * fabs(this->lfoAmount);
	}

	void setEnvelopeAmount(float lfoAmount)
	{
		this->envelopeAmount = 2.0f * (lfoAmount - 0.5f);
		this->envelopeAmount = this->envelopeAmount * fabs(this->envelopeAmount);
	}

	// phase [0,1]
	void setLfoPhase(float phase)
	{
		if (phase >= 0 && phase <= 1.0f)
		{
			lfoR->phase = phase * 256.0f;
			lfoL->phase = phase * 256.0f;
		}
	}

	void setInputDrive(float inputDrive)
	{
		this->inputDrive = inputDrive * inputDrive * inputDrive;
	}

	void initialize(float sampleRate)
	{
		filterHandlerL = new FilterHandler(sampleRate);
		filterHandlerR = new FilterHandler(sampleRate);
		cutoffParamChange = new ParamChangeUtil(sampleRate, 1000.0f);
		lfoL = new Lfo(sampleRate);
		lfoR = new Lfo(sampleRate);
		envelope = new Envelope(sampleRate);
	}

	float getLfoInc()
	{
		return lfoL->inc;
	}

	void setLfoRate(float rateInHz)
	{
		lfoL->setRate(rateInHz);
		lfoR->setRate(rateInHz);
	}

	void setSync(int sync, float rateInHz)
	{
		switch (sync)
		{
			case 1: setLfoRate(0.02f + rateInHz * rateInHz * rateInHz * rateInHz * 49.98f); break; // manual
			case 2: rateInHz = bpm/60.0f*4.0f; break;// 1/16
			case 3: rateInHz = bpm/60.0f*2.0f; break;// 1/8
			case 4: rateInHz = bpm/60.0f*1.00f; break;// 1/4
			case 5: rateInHz = bpm/60.0f*0.5f; break;// 1/2
			case 6: rateInHz = bpm/60.0f*0.25f; break;		// 1/1
			case 7: rateInHz = bpm/60.0f*0.125f; break;		// 2/1

			case 8: rateInHz = bpm/60.0f*4.0f*1.5f; break;		// 1/16T
			case 9: rateInHz = bpm/60.0f*2.0f*1.5f; break;		// 1/8T
			case 10: rateInHz = bpm/60.0f*1.00f*1.5f; break;		// 1/4T
			case 11: rateInHz = bpm/60.0f*0.5f*1.5f; break;		// 1/2T
			case 12: rateInHz = bpm/60.0f*0.25f*1.5f; break;		// 1/1T
			case 13: rateInHz = bpm/60.0f*0.125f*1.5f; break;		// 2/1T

			case 14: rateInHz = bpm/60.0f*4.0f*1.33333333f; break;		// 1/16.
			case 15: rateInHz = bpm/60.0f*2.0f*1.33333333f; break;		// 1/8.
			case 16: rateInHz = bpm/60.0f*1.00f*1.33333333f; break;		// 1/4.
			case 17: rateInHz = bpm/60.0f*0.5f*1.33333333f; break;		// 1/2.
			case 18: rateInHz = bpm/60.0f*0.25f*1.33333333f; break;		// 1/1.
			case 19: rateInHz = bpm/60.0f*0.125f*1.33333333f; break;		// 2/1.
		}
		if (sync != 1)
		{
			setLfoRate(rateInHz);
		}
	}

	void process(float *sampleL, float *sampleR) 
	{
		// setLFO phase
		lfoL->phase= lfoR->phase+(128.0f*param[LFOWIDTH]);

		float random = (float)rand()/RAND_MAX * 0.00000002f;
		*sampleL += random;
		*sampleR += random;

		float cutoff = cutoffParamChange->tick(param[CUTOFF]);

		// add envelope
		float envelopeInput = 0.5f * (*sampleL + *sampleR);
		cutoff += envelope->tick(envelopeInput, envelopeAmount, param[ENVELOPESPEED] * param[ENVELOPESPEED]);

		// add lfo
		float cutoffModulationL = lfoL->tick(param[LFOWAVEFORM] - 1) * lfoAmount;
		float cutoffModulationR = lfoR->tick(param[LFOWAVEFORM] - 1) * lfoAmount;

		*sampleL *= 0.3f + inputDrive * 8.0f;
		*sampleR *= 0.3f + inputDrive * 8.0f;

		// process filter
		filterHandlerL->process(sampleL, cutoff, param[RESONANCE], cutoffModulationL, param[FILTERTYPE]);
		filterHandlerR->process(sampleR, cutoff, param[RESONANCE], cutoffModulationR, param[FILTERTYPE]);

		*sampleL *= volume;
		*sampleR *= volume;
		
		//float scaledInputDrive = (1.0f - inputDrive) * (1.0f - inputDrive) + 0.2f;
		*sampleL /= 1.0f + inputDrive * 8.0f;
		*sampleR /= 1.0f + inputDrive * 8.0f;
		//*sampleL += drysampleL * gainValue;
		//*sampleR += drysampleR * gainValue;
	}
};
#endif

