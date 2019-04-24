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

#if !defined(__AudioUtils_h)
#define __AudioUtils_h

#include "math.h"

class AudioUtils 
{
private:

public:
	AudioUtils() 
	{
	}

	~AudioUtils()
	{
	}

	float getLogScaledValueInDecibel(float inputValue, float maxValue)
	{
		float logScaled = getLogScaledVolume(inputValue, maxValue);
		return getValueInDecibel(logScaled);
	}

	float getLogScaledValueInDecibelFilter(float inputValue, float maxValue)
	{
		float logScaled = getLogScaledVolume(inputValue, maxValue);
		return getValueInDecibelFilter(logScaled);
	}

	// max value unscalled
	float getLogScaledVolume(float inputValue, float maxValue)
	{
		return (expf(inputValue * maxValue * logf(20.0f)) - 1.0f) / 19.0f;
	} 

	// max value unscalled
	float getLogScaledValue(float inputValue)
	{
		// have to return a value between 0..1
		return (expf(inputValue * logf(20.0f)) - 1.0f) / 19.0f;
	}

	float getLogScaledValue(float inputValue, float expValue)
	{
		// have to return a value between 0..1
		return (expf(inputValue * logf(expValue)) - 1.0f) / (expValue - 1.0f);
	}

	float getLogScaledFrequency(float inputValue)
	{
		return 100.0f + getLogScaledValue(inputValue) * 9900.0f;
	}

	float getLogScaledValueInverted(float inputValue)
	{
		// have to return a value between 0..1
		inputValue = 1.0f - inputValue;
		inputValue = (expf(inputValue * logf(20.0f)) - 1.0f) / 19.0f;
		return 1.0f - inputValue;
	}

	float getLogScaledValueInverted(float inputValue, int exponent)
	{
		// have to return a value between 0..1
		inputValue = 1.0f - inputValue;
		inputValue = (expf(inputValue * logf((float)exponent)) - 1.0f) / (exponent - 1.0f);
		return 1.0f - inputValue;
	}

	// input value >= 0
	float getValueInDecibel(float inputValue)
	{
		if (inputValue <= 0)
		{
			return -96.0f;
		}
		else
		{
			// 1.0 --> 0dB
			return 20.0f * log10f(inputValue / 1.0f);
		}
	}

	float getValueInDecibelFilter(float inputValue)
	{
		if (inputValue <= 0)
		{
			return -18.0f;
		}
		else
		{
			// 1.0 --> 0dB
			return 5.0f * log10f(inputValue / 1.0f);
		}
	}

	inline float tanhApp(float x) 
	{
		// Original
		//return tanh(x);

		// Approx
		x*= 2.0f;
		float a= fabs(x);
		float b= 6.0f + a*(3.0f + a); // 6, 3
		return (x * b)/(a * b + 12.0f);
	}

	inline int getNextNearPrime(int value)
	{
		while (!isPrime(value)) value++;
		return value;
	}

	inline bool isPrime(int value)
	{
		bool answer = true;
		if (value == 0) value = 1;
		for (int i = 2; i <= sqrtf((float)value) ; i++) 
		{
			if (value % i == 0)
			{
				answer = false;
				break;
			}
		}
		return answer;
	}
};
#endif

