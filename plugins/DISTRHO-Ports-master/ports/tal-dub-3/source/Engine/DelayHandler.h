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

#ifndef __DelayHandler_h_
#define __DelayHandler_h_

#include "Decimator.h"
#include "interpolatorlinear.h"
#include "Delay.h"
#include "AudioUtils.h"
#include "TapeSlider.h"

class DelayHandler
{
private:
	Decimator9 *decimator;
	InterpolatorLinear *interpolatorLinear;
	TapeSlider* tapeSlider;
	DelayFx* delay;

	float *upsampledValues;
	float delayTime;

	bool liveMode;

	AudioUtils audioUtils;

public:
	DelayHandler(float sampleRate) 
	{
		decimator = new Decimator9();
		interpolatorLinear = new InterpolatorLinear();
		upsampledValues = new float[2];

		// twice oversampled
		delay = new DelayFx(sampleRate * 2);
		tapeSlider = new TapeSlider(sampleRate * 2);

		delayTime = 0.0f;
	}

	~DelayHandler()
	{
		delete decimator;
		delete interpolatorLinear;
		delete delay;
	}

	void setDelay(float delayValue, bool liveMode)
	{
		this->liveMode = liveMode;
		this->delayTime = delayValue;
		delay->setLiveModeDelayTimeChange();
	}


	float getDelay()
	{
		return delay->getDelay();
	}

	void setFeedback(float feedback)
	{
		delay->setFeedback(feedback);
	}

	void setCutoff(float cutoff)
	{
		delay->setCutoff(cutoff);
	}

	void setResonance(float resonance)
	{

		delay->setResonance(resonance);
	}

	void setHighCut(float highCut)
	{
		delay->setHighCut(highCut);
	}

	void clearBuffer()
	{
		delay->clearBuffer();
	}

	float getPeakReductionValue()
	{
		return delay->getPeakReductionValue();
	}

	inline void process(float *input) 
	{
		if (!liveMode)
		{
			delay->setDelay(tapeSlider->tick(this->delayTime));
		}
		else
		{
			delay->setDelay(this->delayTime);
		}
		interpolatorLinear->process2x(*input, upsampledValues);
	
		// limit signal
		upsampledValues[0] = delay->process(&upsampledValues[0]);
		upsampledValues[1] = delay->process(&upsampledValues[1]);

		*input = decimator->Calc(upsampledValues[0], upsampledValues[1]);
	}
};
#endif

