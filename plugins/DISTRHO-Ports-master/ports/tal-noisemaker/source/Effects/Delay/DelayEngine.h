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

#ifndef __DelayEngine_h_
#define __DelayEngine_h_

#include "Delay.h"
#include "AudioUtils.h"

class DelayEngine
{
private:
	DelayFx* delayL;
	DelayFx* delayR;
    
    float delayTime;
    float wet;

    float bpm;

    bool sync;

    bool factorL;
    bool factorR;

	AudioUtils audioUtils;

public:
	DelayEngine(float sampleRate) 
	{
		this->delayL = new DelayFx(sampleRate);
		this->delayR = new DelayFx(sampleRate);

        this->delayTime = 0.5f;
        this->wet = 0.0f;

        this->bpm = 120.0f;

        this->sync = false;
        this->factorL = false;
        this->factorR = false;
	}

	~DelayEngine()
	{
		delete this->delayL;
		delete this->delayR;
	}

	void setBpm(float bpm)
	{
		if (this->bpm != bpm)
		{
			if (bpm > 1.0f && bpm < 1000.0f)
			{
				this->bpm = bpm;
				this->setDelay(this->delayTime);
			}
		}
	}

	void setDelay(float delayTimeIn)
	{
        this->delayTime = delayTimeIn;

        float delayTimeL = this->delayTime;
        float delayTimeR = this->delayTime;

        if (!this->sync)
        {
            delayTimeL = 0.01f + 0.99f * this->audioUtils.getLogScaledValue(this->delayTime);
            delayTimeR = delayTimeL;
        }
        else
        {
            float syncedValue = this->delayTime;
            this->audioUtils.getDelaySyncTimeAndText(&syncedValue);
            delayTimeL = syncedValue / this->bpm;
            delayTimeR = delayTimeL;
        }

        if (this->factorL)
        {
            delayTimeL *= 0.5f;
        }

        if (this->factorR)
        {
            delayTimeR *= 0.5f;
        }

        this->delayL->setDelay(delayTimeL);
        this->delayR->setDelay(delayTimeR);
	}

    void setSync(bool sync)
    {
        this->sync = sync;
        this->setDelay(this->delayTime);
    }

    void setFactor2xL(bool factorL)
    {
        this->factorL = factorL;
        this->setDelay(this->delayTime);
    }

    void setFactor2xR(bool factorR)
    {
        this->factorR = factorR;
        this->setDelay(this->delayTime);
    }

	void setFeedback(float feedback)
	{
		this->delayL->setFeedback(feedback);
		this->delayR->setFeedback(feedback);
	}

	void setLowCut(float value)
	{
		this->delayL->setLowCut(value);
		this->delayR->setLowCut(value);
	}

	void setHighCut(float highCut)
	{
		this->delayL->setHighCut(highCut);
		this->delayR->setHighCut(highCut);
	}

    void setWet(float value)
    {
        this->wet = value;

        if (this->wet == 0.0f)
        {
            this->clearBuffer();
        }
    }

	void clearBuffer()
	{
		this->delayL->clearBuffer();
		this->delayR->clearBuffer();
	}

	inline void process(float *inputL, float *inputR) 
	{
        if (this->wet > 0.0f)
        {
            float inputInL = *inputL;
            float inputInR = *inputR;

		    inputInL = this->delayL->process(&inputInL);
		    inputInR = this->delayR->process(&inputInR);

            *inputL = *inputL + inputInL * wet;
            *inputR = *inputR + inputInR * wet;
        }
    }
};
#endif

