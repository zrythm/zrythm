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

#ifndef __FilterStateVariable12db_h_
#define __FilterStateVariable12db_h_

#include "OscNoise.h"

class FilterStateVariable12db 
{
public:
private:
	float pi;
    float d1, d2, d3, d4;
    float l, h, b, n;
    float f, r, q1, q2;
    float sampleRateFactor;
    int type;
    float currentResonance;

    float cutoff;

    OscNoise *oscNoise;
    AudioUtils audioUtils;

public:
	FilterStateVariable12db(float sampleRate, int type) 
	{
        this->type = type;

		pi= 3.1415926535f;
        f = 0.0f;
        r = 0.0f;
        q1 = 0.0f;
        q2 = 0.0f;

        currentResonance = -1.0f;

		sampleRateFactor= 44100.0f/sampleRate;
		if (sampleRateFactor > 1.0f) 
		{
			sampleRateFactor= 1.0f;
		}

		oscNoise = new OscNoise(sampleRate);
        reset();
	}

    ~FilterStateVariable12db()
    {
        delete oscNoise;
    }

public:
    void reset()
    {
        d1 = 0.4f;
        d2 = 0.4f;
        d3 = 0.4f;
        d4 = 0.4f;
    }

	inline void process(float *input, const float cutoffIn, const float resonance, const bool calcCeff) 
	{
        if (calcCeff)
        {
            // ideal tuning
            // f = sinf(pi * cutoffIn * sampleRateFactor);

            // optimized
            cutoff = cutoffIn;
            f = pi * cutoff * sampleRateFactor;

            if (currentResonance != resonance)
            {
                currentResonance = resonance;

                r = resonance * resonance;
                r = r * r * r;
                r = r * r * r * r;
                q1 = 1.0f / (r * 100000.0f + 0.5f);
                q2 = 1.0f / (r * 10.0f + 0.5f);
            }
        }
        
        this->calcFilterSaturation(d1, d2, *input, q2);
        
        // store delays
        d1 = b;
        d2 = l;

        switch (type)
        {
            case 1:
                this->calcFilterSaturation(d3, d4, h, q1);
                *input = h;
                break;
            case 2:
                this->calcFilterSaturation(d3, d4, b, q1);
                *input = b;
                break;
            default:
                this->calcFilterSaturation(d3, d4, l, q1);
                *input = l;
                //*input = tanhClipper(0.5f * l) * 2.0f;
        }

        d3 = b;
        d4 = l;

        //float l = d2 + f * d1;
        //float h = *input - l - q * d1;
        //float b = tanhClipper((f * h + d1) * 0.05f) * 20.0f;
        //float n = h + l;
        //
        //// store delays
        //d1 = b;
        //d2 = l;

        //switch (type)
        //{
        //    case 1:
        //        *input = h;
        //        break;
        //    case 2:
        //        *input = b;
        //        break;
        //    default:
        //        *input = l;
        //        //*input = tanhClipper(0.5f * l) * 2.0f;
        //}
	}

    inline void calcFilterSaturation(float localD1, float localD2, float input, float localQ)
    {
        float rnd1 = 0.001f * oscNoise->getNextSamplePositive() * (1.0f - cutoff);

        l = localD2 + (f + rnd1) * localD1;
        h = input - l - localQ * localD1;
        b = tanhClipper((f * h + localD1) * 0.1f) * 10.0f;
        n = h + l;
    }

    inline void calcFilter(float localD1, float localD2, float input, float localQ)
    {
        l = localD2 + f * localD1;
        h = input - l - localQ * localD1;
        b = f * h + localD1;
        n = h + l;
    }

	inline float tanhApp(const float x) 
	{
		return x;
	}

	inline float tanhClipper(float x) 
	{
		// return tanh(x);
		x *= 2.0f;
		float a = fabs(x);
		float b = 6.0f+a*(3.0f+a);
		return (x*b)/(a*b+12.0f);
	}
 };
#endif