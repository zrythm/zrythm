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

#ifndef OscTriangle_H
#define OscTriangle_H

#include "BlepData.h"

class OscTriangle
{
public:
	const float *sincBlep;
	const float *minBlep;
	const float sampleRate;
	float sampleRateInv;
	const float oversampling;
	const int n;

	float *buffer;
	float *bufferSync;
	int bufferPos;
	int bufferPosSync;

	float x;

	float phaseFM;
	float pi;
	float pi2;

	float sign;

	float currentValue;

    int countDelayBlep;

	float delay1;
	float delay2;
	float delay3;
	float delay4;
	float delay5;
	float delay6;
	float delay7;
	float delay8;
	float delay9;
	float delay10;
	float delay11;
	float delay12;
	float delay13;
	float delay14;
	float delay15;

	OscTriangle(float sampleRate, int oversampling):
		sampleRate(sampleRate),
		sampleRateInv(1.0f / sampleRate),
		oversampling(128.0f / oversampling), 
		n(32 * (int)oversampling)
	{
		buffer= new float[n];
		bufferSync= new float[n];
		BlepData *blepData= new BlepData();
		//minBlep = blepData->getBlep();
		sincBlep = blepData->getSinc();
		minBlep = blepData->getBlep();
        delete blepData;

		resetOsc(0.0f);

		pi= 3.1415926535897932384626433832795f;
		pi2= 2.0f * pi;

		sign = 1.0f;
		currentValue = 0.0f;
        countDelayBlep = 100; 

        resetOsc(0.0f);
	}

	~OscTriangle() 
	{
		delete[] buffer;
        delete[] bufferSync;
	}

	void resetOsc(float phase) 
	{
		x = phase;
		phaseFM = 0.0f;
		bufferPos = 0;
        countDelayBlep = 0;
        sign = 1.0f;
		for (int i = 0; i < n; i++) 
		{
			buffer[i]= 0.0f;
		}

		bufferPosSync = 0;
		for (int i = 0; i < n; i++) 
		{
			bufferSync[i]= 0.0f;
		}

		delay1 = 0.0f;
		delay2 = 0.0f;
		delay3 = 0.0f;
		delay4 = 0.0f;
		delay5 = 0.0f;
		delay6 = 0.0f;
		delay7 = 0.0f;
		delay8 = 0.0f;
		delay9 = 0.0f;
		delay10 = 0.0f;
		delay11 = 0.0f;
		delay12 = 0.0f;
		delay13 = 0.0f;
		delay14 = 0.0f;
		delay15 = 0.0f;
	}

	inline float getNextSample(float freq, float fm, float fmFreq, bool reset, float resetFrac, float masterFreq) 
	{
		if (fm > 0.0f) 
		{
			phaseFM += fmFreq / sampleRate;
			freq +=  fm * 10.0f * fmFreq * (1.0f + sinf(phaseFM * pi2));
			if (phaseFM > 1.0f) phaseFM -= 1.0f;
		}
		if (freq > 22000.0f) freq = 22000.0f;

		if (freq > sampleRate * 0.45f) freq = sampleRate * 0.45f;

		float fs = freq * sampleRateInv * 2.0f;
		x += fs;

        countDelayBlep++;

        if (x >= 1.0f) 
		{
			if (sign == 1.0f)
			{
				sign = -1.0f;
			}
			else
			{
				sign = 1.0f;
			}
			x -= 1.0f;
			// mixInSinc(x / fs, sign * n / (1.0f / fs));
			mixInSinc(x / fs, sign * n * fs);
            countDelayBlep = 0;
		}

		if (reset)
		{
			float tmp =  masterFreq / sampleRate;
			float fracMaster = (fs * resetFrac) / tmp;
            if (countDelayBlep != 16)
            {
			    if (sign == 1.0f)
			    {
				    sign = 1.0f;
				    mixInBlepSync(resetFrac / tmp, x - fracMaster);
                }
			    else
			    {
				    //sign = 1.0f;
				    //mixInBlepSync(resetFrac / tmp, 1.0f - (x - fracMaster));
				    sign = -1.0f;
				    mixInBlepSync(resetFrac / tmp, sign * (x - fracMaster));
                }
            }
			x = fracMaster;
		}

		if (sign == 1.0f)
		{
			currentValue = x;
		}
		else
		{
			currentValue = 1.0f - x;
		}

		float value = delay15;
		delay15 = delay14;
		delay14 = delay13;
		delay13 = delay12;
		delay12 = delay11;
		delay11 = delay10;
		delay10 = delay9;
		delay9 = delay8;
		delay8 = delay7;
		delay7 = delay6;
		delay6 = delay5;
		delay5 = delay4;
		delay4 = delay3;
		delay3 = delay2;
		delay2 = delay1;
		delay1 = currentValue - 0.5f + getNextBlepSync();
	
		return  getNextBlep() + value;
	}

	inline float getNextBlep() 
	{
		buffer[bufferPos]= 0.0f;
		bufferPos++;

		// Wrap pos
		if (bufferPos>=n) 
		{
			bufferPos -= n;
		}
		return buffer[bufferPos];
	}
	
	#define LERP(A,B,F) ((B-A)*F+A)
	inline void mixInSinc(float offset, float scale) 
	{
		int lpIn = (int)(oversampling * offset);
		float frac = fmod(oversampling * offset, 1.0f);

		for (int i = 0; i < n; i++, lpIn += (int)oversampling) 
		{
			buffer[(bufferPos + i)&(n-1)] +=  LERP(sincBlep[lpIn], sincBlep[lpIn+1], frac) * scale;
		}
	}

	inline float getNextBlepSync() 
	{
		bufferSync[bufferPosSync] = 0.0f;
		bufferPosSync++;

		// Wrap pos
		if (bufferPosSync >= n) 
		{
			bufferPosSync -= n;
		}
		return  bufferSync[bufferPosSync];
	}

	inline void mixInBlepSync(float offset, float scale) 
	{
		int lpIn = (int)(oversampling * 0.5f * offset);
		float frac = fmod(offset * oversampling * 0.5f, 1.0f);
		for (int i = 0; i < n; i++, lpIn += (int)(oversampling * 0.5f)) 
		{
			bufferSync[(bufferPosSync + i)&(n-1)] += scale - LERP(minBlep[lpIn], minBlep[lpIn+1], frac) * scale;
		}
	}

};
#endif