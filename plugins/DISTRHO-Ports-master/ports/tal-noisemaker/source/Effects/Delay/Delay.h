/*
	==============================================================================
	This file is part of Tal-Dub-III by Patrick Kunz.

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

#if !defined(__Delay_h)
#define __Delay_h

#include "math.h"
#include "TalEq.h"
#include "AudioUtils.h"


class DelayFx
{
private:
	float *delayLineStart;
	float *delayLineEnd;
	float *readPointer, *writePointer;
	int delayLineLength;

    TalEq *talEq;

	float z1;

	float delay;
	float feedback;

	AudioUtils audioUtils;

	float sampleRate;

	// Delay time in seconds
	static const int MAX_DELAY_TIME = 4;

public:
	DelayFx(float sampleRate)
	{
		if (sampleRate == 0) sampleRate = 4100.0f;
		this->sampleRate = sampleRate;

		this->talEq = new TalEq((float)sampleRate);

		this->clearBufferAndInitValues(sampleRate);
	}

	~DelayFx()
	{
		if (delayLineStart) 
        {
			delete[] delayLineStart;
		}

        delete talEq;
	}

	// delay [0..1]
	inline void setDelay(float delay)
	{
		if (delay > 1.0f) delay = 1.0f;
		if (delay < 0.01f) delay = 0.01f;
		this->delay = delay;
	}

	inline float getDelay()
	{
		return delay;
	}

	void setFeedback(float feedback)
	{
        this->feedback = audioUtils.getDelayFeedback(feedback);
		//feedback *= 2.0f;
		//this->feedback = 1.0f + (feedback - 1.0f) * (feedback - 1.0f) * (feedback - 1.0f);
	}

	void setLowCut(float value)
	{
        this->talEq->setHighPassFrequency(value * 0.95f + 0.05f);
	}

	void setHighCut(float value)
	{
		//this->highCut = 0.01f + highCut * highCut * highCut * 0.99f;
        this->talEq->setLowPassFrequency(1.0f - value);
	}

	void clearBufferAndInitValues(float sampleRate)
	{
		float maxDelayInSeconds = MAX_DELAY_TIME;
		delayLineLength = (int)floorf(sampleRate * maxDelayInSeconds + 1);
		delayLineStart = new float[delayLineLength];

		//set up pointers for delay line
		delayLineEnd = delayLineStart + delayLineLength;
		this->clearBuffer();

		delay = 1.0f;
		feedback = 0.5f;
	}

	void clearBuffer()
	{
		writePointer = delayLineStart;

		//zero out the buffer (silence)
		do 
		{
			*writePointer = 0.0f;
		}
		while (++writePointer < delayLineStart + delayLineLength);

		//set read pointer to end of delayline. Setting it to the end
		//ensures the interpolation below works correctly to produce
		//the first non-zero sample.
		writePointer = delayLineStart + delayLineLength -1;

		// Zero out all pass interpolation buffer
		z1 = 0.0f;
    }

	inline float process(float *sample) 
	{
		float offset = (float)(delayLineLength - 1.0f) * delay;

		//compute the largest read pointer based on the offset.  If ptr
		//is before the first delayline location, wrap around end point
		float *ptr  = writePointer - (int)floorf(offset);
		if (ptr < delayLineStart)
			ptr += delayLineLength;

		float *ptr2 = ptr - 1;
		if (ptr2 < delayLineStart)
			ptr2 += delayLineLength;

		//jassert(ptr >= delayLineStart);
		//jassert(ptr < delayLineEnd);

		//jassert(ptr2 >= delayLineStart);
		//jassert(ptr2 < delayLineEnd);

		float frac = offset - (int)floorf(offset);
		float delayLineOutput= *ptr2 + *ptr * (1 - frac) - (1 - frac) * z1;
		z1 = delayLineOutput;

		// Filters
		float valueWithFeedback = *sample + delayLineOutput * feedback;
		talEq->process(&valueWithFeedback);
        valueWithFeedback = audioUtils.tanhApp(valueWithFeedback);

		//write the input sample and any feedback to delayline
		*writePointer = valueWithFeedback;

		//increment buffer index and wrap if necesary
		if (++writePointer >= delayLineEnd)
		{ 
			writePointer = delayLineStart;
		}
		return delayLineOutput;
	}
};

#endif