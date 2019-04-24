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
#include "AudioUtils.h"
#include "Filter6dB.h"
#include "DCBlock.h"
#include "NoiseOsc.h"


class DelayFx
{
private:
	float *delayLineStart;
	float *delayLineEnd;
	float *readPointer, *writePointer;
	int delayLineLength;

	float z1;

	float delay;
	float currentDelay;
	float cutoff; 
	float resonance;
	float highCut;
	float feedback;

	AudioUtils audioUtils;
	Filter6dB *filter6dB;
	DCBlock *dcBlock;
	NoiseOsc *noiseOsc;

	float sampleRate;
	float peakReductionValue;

	float liveModeFadeValue;
	float liveModeFadeOffset;

	bool isClearingBuffer;

	// Delay time in seconds
	static const int MAX_DELAY_TIME = 4;

public:
	DelayFx(float sampleRate)
	{
		if (sampleRate == 0) sampleRate = 4100.0f;
		this->sampleRate = sampleRate;

		liveModeFadeValue = 0.0f;
		liveModeFadeOffset = 1.0f / sampleRate * 4.0f;

		peakReductionValue = 0.0f;
		isClearingBuffer = false;

		clearBufferAndInitValues(sampleRate);
	}

	~DelayFx()
	{
		if (delayLineStart) {
			delete[] delayLineStart;
		}
	}

	void setPeakReductionValue(float newLevel)
	{
		newLevel = fabsf (newLevel);

		if (newLevel > peakReductionValue)
		{
			if (newLevel > 1.0f) newLevel = 1.0f;

			peakReductionValue = newLevel;
		}
	}

	float getPeakReductionValue()
	{
		float value = peakReductionValue;
		peakReductionValue = 0.0f;
		return value;
	}

	// delay [0..1]
	inline void setDelay(float delay)
	{
		if (delay > 1.0f) delay = 1.0f;
		if (delay < 0.0f) delay = 0.0f;
		this->delay = delay;
	}

	inline float getDelay()
	{
		return delay;
	}

	void setFeedback(float feedback)
	{
		feedback *= 2.0f;
		this->feedback = 1.0f + (feedback - 1.0f) * (feedback - 1.0f) * (feedback - 1.0f);
	}

	void setCutoff(float cutoff)
	{
		this->cutoff = cutoff * cutoff;
		filter6dB->setCoefficients(this->cutoff);
	}

	void setResonance(float resonance)
	{
		this->resonance = resonance * resonance;
	}

	void setHighCut(float highCut)
	{
		this->highCut = 0.01f + highCut * highCut * highCut * 0.99f;
	}

	void clearBufferAndInitValues(float sampleRate)
	{
		float maxDelayInSeconds = MAX_DELAY_TIME;
		delayLineLength = (int)floorf(sampleRate * maxDelayInSeconds);
		delayLineStart = new float[delayLineLength];

		//set up pointers for delay line
		delayLineEnd = delayLineStart + delayLineLength;
		clearBuffer();

		delay = 1.0f;
		currentDelay = delay;
		cutoff = 1.0f;
		resonance = 1.0f;
		highCut = 0.0f;
		feedback = 0.5f;

		filter6dB = new Filter6dB(sampleRate);
		dcBlock = new DCBlock();
		noiseOsc = new NoiseOsc(sampleRate);
	}

	void clearBuffer()
	{
		if (!isClearingBuffer)
		{
			isClearingBuffer = true;
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

			isClearingBuffer = false;
		}
	}

	void setLiveModeDelayTimeChange()
	{
		liveModeFadeValue = 1.0f;
	}

	inline float getFadeOutValue()
	{
		float fadeOutFactor = 1.0f;
		if (liveModeFadeValue > 0.0f)
		{
			liveModeFadeValue -= liveModeFadeOffset;
			fadeOutFactor = liveModeFadeValue;
		}
		if (liveModeFadeValue < 0.0f)
		{
			liveModeFadeValue = 0.0f;
			fadeOutFactor = 0.0f;
			clearBuffer();
		}
		return fadeOutFactor;
	}

	inline bool isFadingOut()
	{
		return liveModeFadeValue > 0.0f;
	}

	inline float process(float *sample) 
	{
		if (!isClearingBuffer)
		{
			// supress delay changes while fading out
			if (!isFadingOut())
			{
				currentDelay = delay;
			}
			float offset = (float)delayLineLength * currentDelay;

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

			float currentFadeOutValue = getFadeOutValue();
			delayLineOutput *= currentFadeOutValue;

			// Filters
			float valueWithFeedback = *sample + delayLineOutput * feedback;
			dcBlock->tick(&valueWithFeedback, highCut);
			filter6dB->process(&valueWithFeedback);

			// Stauration
			float valueWithFeedbackNotShaped = valueWithFeedback;
			valueWithFeedback = audioUtils.tanhApp(valueWithFeedback);
			setPeakReductionValue(valueWithFeedbackNotShaped - valueWithFeedback);

			//write the input sample and any feedback to delayline
			*writePointer = valueWithFeedback * currentFadeOutValue;
	
			//increment buffer index and wrap if necesary
			if (++writePointer >= delayLineEnd)
			{ 
				writePointer = delayLineStart;
			}
			return delayLineOutput;
		}
		return 0.0f;
	}
};

#endif