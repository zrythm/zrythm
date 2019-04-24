/*
	==============================================================================
	This file is part of Tal-Dub-III by Patrick Kunz.

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
#include "DelayHandler.h"
#include "AudioUtils.h"

class Engine 
{
private:
	float bpm;
	float peakReductionValue[2];

	void initDelayTimes()
	{
		syncTimes= new float[18];
		syncTimes[0]=  0.5f*(60.0f/16.0f)*4.0f; // 1/16
		syncTimes[1]=  0.5f*(60.0f/8.0f)*4.0f; // 1/8
		syncTimes[2]=  0.5f*(60.0f/4.0f)*4.0f; // 1/4
		syncTimes[3]=  0.5f*(60.0f/2.0f)*4.0f; // 1/2
		syncTimes[4]=  0.5f*(60.0f/1.0f)*4.0f; // 1/1
		syncTimes[5]=  0.5f*(60.0f/0.5f)*4.0f; // 2/1
		syncTimes[6]=  0.5f*(60.0f/16.0f)*1.5f*4.0f; // 1/16T
		syncTimes[7]=  0.5f*(60.0f/8.0f)*1.5f*4.0f; // 1/8T
		syncTimes[8]=  0.5f*(60.0f/4.0f)*1.5f*4.0f; // 1/4T
		syncTimes[9]=  0.5f*(60.0f/2.0f)*1.5f*4.0f; // 1/2T
		syncTimes[10]= 0.5f*(60.0f/1.0f)*1.5f*4.0f; // 1/1T
		syncTimes[11]= 0.5f*(60.0f/0.5f)*1.5f*4.0f; // 2/1T
		syncTimes[12]= (60.0f/16.0f)* 1.3333333f; // 1/16.
		syncTimes[13]= (60.0f/8.0f)* 1.3333333f; // 1/8.
		syncTimes[14]= (60.0f/4.0f)* 1.3333333f; // 1/4.
		syncTimes[15]= (60.0f/2.0f)* 1.3333333f; // 1/2.
		syncTimes[16]= (60.0f/1.0f)* 1.3333333f; // 1/1.
		syncTimes[17]= (60.0f/0.5f)* 1.3333333f; // 2/1.
	}

public:
	DelayHandler *delayHandlerL;
	DelayHandler *delayHandlerR;

	float *syncTimes;

	float *param;

	float inputDrive, dry, wet;

	ParamChangeUtil* cutoffParamChange;
	AudioUtils audioUtils;

	Engine(float sampleRate) 
	{
		Params *params= new Params();
		this->param= params->parameters;

		delayHandlerL = NULL;
		delayHandlerR = NULL;

		initialize(sampleRate);
		initDelayTimes();

		bpm = 120.0f;
		dry = 1.0f;
		wet = 1.0f;
		inputDrive = 1.0f;
	}

	~Engine()
	{
		clearMemory();
	}

	void clearMemory()
	{
		if (delayHandlerL)
			delete delayHandlerL;

		if (delayHandlerR)
			delete delayHandlerR;

		if (syncTimes) 
			delete[] syncTimes;
	}

	void setSampleRate(float sampleRate)
	{
		initialize(sampleRate);
	}

	void clearBuffer()
	{
		delayHandlerL->clearBuffer();
		delayHandlerR->clearBuffer();
	}

	void setBpm(float bpm, float delayTime, int sync, bool twiceL, bool twiceR)
	{
		if (this->bpm != bpm)
		{
			if (bpm > 1.0f && bpm < 1000.0f)
			{
				this->bpm = bpm;
				setDelay(delayTime, sync, twiceL, twiceR, false);
			}
		}
	}

	void setDry(float dry)
	{
		this->dry = audioUtils.getLogScaledVolume(dry, 2.0f);
	}

	void setWet(float wet)
	{
		this->wet = audioUtils.getLogScaledVolume(wet, 2.0f);
	}

	void setInputDrive(float inputDrive)
	{
		this->inputDrive = audioUtils.getLogScaledVolume(inputDrive, 2.0f);
	}

	void setDelay(float delayValue, int sync, bool twiceL, bool twiceR, bool liveMode)
	{
		float delayL = delayValue;
		float delayR = delayValue;
		if (sync > 1)
		{
			delayL = syncTimes[sync - 2] / bpm / 2.0f;
			delayR = syncTimes[sync - 2] / bpm / 2.0f;
		}
		if (twiceL)
		{
			delayL /= 2.0f;
		}
		if (twiceR)
		{
			delayR /= 2.0f;
		}
		delayHandlerL->setDelay(delayL, liveMode);
		delayHandlerR->setDelay(delayR, liveMode);
	}

	void setFeedback(float feedback)
	{
		delayHandlerL->setFeedback(feedback);
		delayHandlerR->setFeedback(feedback);
	}

	void setCutoff(float cutoff)
	{
		delayHandlerL->setCutoff(cutoff);
		delayHandlerR->setCutoff(cutoff);
	}

	void setResonance(float resonance)
	{
		delayHandlerL->setResonance(resonance);
		delayHandlerR->setResonance(resonance);
	}

	void setHighCut(float highCut)
	{
		delayHandlerL->setHighCut(highCut);
		delayHandlerR->setHighCut(highCut);
	}

	float getPeakReductionValueL()
	{
		return delayHandlerL->getPeakReductionValue();
	}

	float getPeakReductionValueR()
	{
		return delayHandlerR->getPeakReductionValue();
	}

	void initialize(float sampleRate)
	{
		DelayHandler *delayHandlerTmpL = delayHandlerL;
		DelayHandler *delayHandlerTmpR = delayHandlerR;

		delayHandlerL = new DelayHandler(sampleRate);
		delayHandlerR = new DelayHandler(sampleRate);
		cutoffParamChange = new ParamChangeUtil(sampleRate, 1000.0f);

		if (delayHandlerTmpL != NULL)
			delete delayHandlerTmpL;

		if (delayHandlerTmpR != NULL)
			delete delayHandlerTmpR;
	}

	void process(float *sampleL, float *sampleR) 
	{
		float random = (float)rand()/RAND_MAX * 0.00000002f;
		float inputL = *sampleL + random;
		float inputR = *sampleR + random;

		//float cutoff = cutoffParamChange->tick(param[CUTOFF]);

		*sampleL *= inputDrive;
		*sampleR *= inputDrive;

		// process filter
		delayHandlerL->process(sampleL);
		delayHandlerR->process(sampleR);

		// dry / wet mix
		*sampleL = *sampleL * wet;
		*sampleR = *sampleR * wet;

		*sampleL += inputL * dry;
		*sampleR += inputR * dry;
	}
};
#endif

