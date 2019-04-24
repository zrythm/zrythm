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

#if !defined(__AudioUtils_h)
#define __AudioUtils_h

#include "math.h"
#include "Params.h"

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
	float getLogScaledValue(float inputValue, float maxValue)
	{
		return (expf(inputValue * maxValue * logf(20.0f)) - 1.0f) / 19.0f;
	}

	// max value unscalled
	float getLogScaledRate(float inputValue)
	{
		return (expf(0.2f + inputValue * 3.0f * logf(20.0f)) - 1.0f) / 19.0f;
	}

	// max value unscalled
	float getLogScaledValue(float inputValue)
	{
		// have to return a value between 0..1
		return (expf(inputValue * logf(20.0f)) - 1.0f) / 19.0f;
	}

	float getLogScaledValueFilter(float inputValue)
	{
		// have to return a value between 0..1
		return (expf(inputValue * logf(8.0f)) - 1.0f) / 7.0f;
	}

	// max value unscalled
	// have to return a value between -1..1
	float getLogScaledValueCentered(float inputValue)
	{
		float centerdValue = 2.0f * (inputValue - 0.5f);
		return centerdValue * centerdValue * centerdValue;
	}

    float getDelayFeedback(float feedback)
    {
        feedback *= 2.0f;
	    return 1.0f + (feedback - 1.0f) * (feedback - 1.0f) * (feedback - 1.0f);
    }

	float getLogScaledLinearValueCentered(float inputValue)
	{
		float centerdValue = 2.0f * (inputValue - 0.5f);
		return centerdValue;
	}

	float getLogScaledValueInverted(float inputValue)
	{
		// have to return a value between 0..1
		inputValue = 1.0f - inputValue;
		inputValue = (expf(inputValue * logf(20.0f)) - 1.0f) / 19.0f;
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

	float getMidiNoteInHertz (float noteNumber)
	{
		noteNumber -= 12 * 6 + 9; // now 0 = A440
		return 440.0f * powf(2.0f, (float)noteNumber / 12.0f);
	}

	float getOscFineTuneValue(float value)
	{
		return (value - 0.5f) * 2.0f;
	}

	float getOscTuneValue(float value)
	{
		return (float)(int)(value * 24.0f - 12.0f);
	}

    int getBitDepthDynamic(float value)
    {
        return (int)(1.0f + powf(value, 8.0f) * 65535.0f);
    }

    int getBitDepth(float value)
    {
        return (int)(1.0f + value * 15.0f);
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

    const char* getSyncedRateAndGetText(float *rate, float bpm)
    {
        const char* text = "";
        int rateSelector = (int)(*rate * 17.0f + 0.001f);

        switch (rateSelector)
        {
        case 0: *rate = bpm/60.0f*4.0f; text = "1/16"; break;
        case 1: *rate = bpm/60.0f*2.0f; text = "1/8"; break;
        case 2: *rate = bpm/60.0f*1.00f; text = "1/4"; break;
        case 3: *rate = bpm/60.0f*0.5f; text = "1/2"; break;
        case 4: *rate = bpm/60.0f*0.25f; text = "1/1"; break;
        case 5: *rate = bpm/60.0f*0.125f; text = "2/1"; break;

        case 6: *rate = bpm/60.0f*4.0f*1.5f; text = "1/16T"; break;
        case 7: *rate = bpm/60.0f*2.0f*1.5f; text = "1/8T"; break;
        case 8: *rate = bpm/60.0f*1.00f*1.5f; text = "1/4T"; break;
        case 9: *rate = bpm/60.0f*0.5f*1.5f; text = "1/2T"; break;
        case 10: *rate = bpm/60.0f*0.25f*1.5f; text = "1/1T"; break;
        case 11: *rate = bpm/60.0f*0.125f*1.5f; text = "2/1T"; break;

        case 12: *rate = bpm/60.0f*4.0f*1.33333333f; text = "1/16."; break;
        case 13: *rate = bpm/60.0f*2.0f*1.33333333f; text = "1/8."; break;
        case 14: *rate = bpm/60.0f*1.00f*1.33333333f; text = "1/4."; break;
        case 15: *rate = bpm/60.0f*0.5f*1.33333333f; text = "1/2."; break;
        case 16: *rate = bpm/60.0f*0.25f*1.33333333f; text = "1/1."; break;
        case 17: *rate = bpm/60.0f*0.125f*1.33333333f; text = "2/1."; break;
        }
        return text;
    }

    const char* getDelaySyncTimeAndText(float *value)
    {
        const char* text = "";
        int selector = (int)(*value * 17.0f);

        switch (selector)
        {
        case 0: *value =  0.5f*(60.0f/16.0f)*4.0f; text = "1/16"; break;
		case 1: *value =  0.5f*(60.0f/8.0f)*4.0f; text = "1/8"; break;
		case 2: *value =  0.5f*(60.0f/4.0f)*4.0f; text = "1/4"; break;
		case 3: *value =  0.5f*(60.0f/2.0f)*4.0f; text = "1/2"; break;
		case 4: *value =  0.5f*(60.0f/1.0f)*4.0f; text = "1/1"; break;
		case 5: *value =  0.5f*(60.0f/0.5f)*4.0f; text = "2/1"; break;
		case 6: *value =  0.5f*(60.0f/16.0f)*1.5f*4.0f; text = "1/16T"; break;
		case 7: *value =  0.5f*(60.0f/8.0f)*1.5f*4.0f; text = "1/8T"; break;
		case 8: *value =  0.5f*(60.0f/4.0f)*1.5f*4.0f; text = "1/4T"; break;
		case 9: *value =  0.5f*(60.0f/2.0f)*1.5f*4.0f; text = "1/2T"; break;
		case 10: *value = 0.5f*(60.0f/1.0f)*1.5f*4.0f; text = "1/1T"; break;
		case 11: *value = 0.5f*(60.0f/0.5f)*1.5f*4.0f; text = "2/1T"; break;
		case 12: *value = (60.0f/16.0f)* 1.3333333f; text = "1/16."; break;
		case 13: *value = (60.0f/8.0f)* 1.3333333f; text = "1/8."; break;
		case 14: *value = (60.0f/4.0f)* 1.3333333f; text = "1/4."; break;
		case 15: *value = (60.0f/2.0f)* 1.3333333f; text = "1/2."; break;
		case 16: *value = (60.0f/1.0f)* 1.3333333f; text = "1/1."; break;
		case 17: *value = (60.0f/0.5f)* 1.3333333f; text = "2/1."; break;
        }

        return text;
    }

    float getTranspose(float value)
    {
        float transpose = 0.0f;
        int valueInt = (int)(value * 3.00001f);
        switch (valueInt)
        {
        case 0: transpose = -12.0f; break;
        case 1: transpose = 0.0f; break;
        case 2: transpose = 12.0f; break;
        case 3: transpose = 24.0f; break;
        }
        return transpose;
    }

	float getLogScaledFrequency(float inputValue)
	{
		return 100.0f + getLogScaledValue(inputValue) * 9900.0f;
	}

    //// Converts [0;1] to [1,2,3...99,100]
    //int calcComboBoxValue(float value, int param)
    //{
    //    int numItems = this->getNumComboBoxItems(param);
    //    return (int)floorf(value * (numItems - 1.0f) + 1.0f + 0.5f);
    //}

    //float calcComboBoxValueNormalized(float value, int param)
    //{
    //    int numItems = this->getNumComboBoxItems(param);
    //    return (value - 1.0f) / (numItems - 1.0f);
    //}

    //int getNumComboBoxItems(int param)
    //{
    //    int numItems = 1;

    //    switch(param)
    //    {
    //    case VOICES: numItems = 6; break;
    //    case PORTAMENTOMODE: numItems = 3; break;
    //    case LFO1DESTINATION: numItems = 8; break;
    //    case LFO2DESTINATION: numItems = 8; break;
    //    case FREEADDESTINATION: numItems = 6; break;
    //    case FILTERTYPE: numItems = 10; break;
    //    case OSC1WAVEFORM: numItems = 3; break;
    //    case OSC2WAVEFORM: numItems = 5; break;
    //    case ENVELOPEEDITORDEST1: numItems = 8; break;
    //    case ENVELOPEEDITORSPEED: numItems = 6; break;
    //    }

    //    return numItems;
    //}
};
#endif

