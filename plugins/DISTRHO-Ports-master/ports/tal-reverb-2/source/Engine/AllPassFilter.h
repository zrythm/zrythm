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


#if !defined(__AllPassFilter_h)
#define __AllPassFilter_h

#include "math.h"
#include "AudioUtils.h"

class AllPassFilter
{
private:
	float delay, gain;
	float* buffer;
	int bufferLength, writePtr, readPtr1, readPtr2;
	float z1;

	AudioUtils audioUtils;

public:
	// delay times in milliseconds
	AllPassFilter(float delayTime, float feedbackGain, long samplingRate)
	{
		//OutputDebugString("start init allPass");
		gain = feedbackGain;
		delay = delayTime;

		bufferLength = (int)(delay * samplingRate / 1000.0f);
		bufferLength = audioUtils.getNextNearPrime(bufferLength);
		buffer = new float[bufferLength];

		//zero out the buffer (create silence)
		for (int i = 0; i < bufferLength; i++)
			buffer[i] = 0.0f;

		writePtr = readPtr1 = readPtr2 = 0;
		z1 = 0.0f;
		//OutputDebugString("end init allPass");
	}

	~AllPassFilter()
	{
		delete buffer;
	}

	// all values [0..1]
	inline float processInterpolated(float input, float delayLength, float diffuse, bool negative)
	{
		// dynamic interpolated
		float offset = (bufferLength - 2.0f) * delayLength + 1.0f;
		readPtr1 = writePtr - (int)floorf(offset);

		if (readPtr1 < 0)
			readPtr1 += bufferLength;

		readPtr2 = readPtr1 - 1;
		if (readPtr2 < 0)
			readPtr2 += bufferLength;

		// interpolate, see paper: http://www.stanford.edu/~dattorro/EffectDesignPart2.pdf
		float frac = offset - (int)floorf(offset);
		float temp = buffer[readPtr2] + buffer[readPtr1] * (1-frac) - (1-frac) * z1;
		z1 = temp;

		float output;
		if (!negative)
		{
			buffer[writePtr] = diffuse * gain * temp + input;
			output = temp - diffuse * gain * buffer[writePtr];
		}
		else
		{
			buffer[writePtr] = diffuse * gain * temp - input;
			output = temp + diffuse * gain * buffer[writePtr];
		}

		if (++writePtr >= bufferLength)
			writePtr = 0;

		return output;
	}

	inline float process(float input, float diffuse)
	{
		float temp = buffer[readPtr1];
		buffer[readPtr1] = diffuse * gain * temp + input;
		float output = temp - diffuse * gain * buffer[readPtr1];

		if (++readPtr1 >= bufferLength)
			readPtr1 = 0;

		return output;
	}

	inline float process(float input)
	{
		float temp = buffer[readPtr1];
		buffer[readPtr1] = gain * temp + input;
		float output = temp - gain * buffer[readPtr1];

		if (++readPtr1 >= bufferLength)
			readPtr1 = 0;

		return output;
	}
};
#endif