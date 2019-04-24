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

#ifndef OscSaw_H
#define OscSaw_H

#include "BlepData.h"

class OscSaw
{
public:
	const float *minBlep;
	const float sampleRate;
	float sampleRateInv;
	const float oversampling;
	const int n;

	float *buffer;
	float x, tmp;
	float value;
	int bufferPos;

	bool phaseReset;

	float phaseFM;
	float pi;
	float pi2;

	OscSaw(float sampleRate, int oversampling):
		sampleRate(sampleRate),
		sampleRateInv(1.0f / sampleRate),
		oversampling(64.0f / oversampling),
		n(32 * (int)oversampling)
	{
		buffer= new float[n];
		BlepData *blepData= new BlepData();
		minBlep= blepData->getBlep();
        delete blepData;

		resetOsc(0.0f);

		pi= 3.1415926535897932384626433832795f;
		pi2= 2.0f*pi;
	}

	~OscSaw() 
	{
		delete[] buffer;
	}

	void resetOsc(float phase) 
	{
		x = phase;
		phaseFM = 0.0f;
		bufferPos = 0;
		for (int i = 0; i < n; i++) 
		{
			buffer[i] = 0.0f;
		}
	}

	inline float getNextSample(float freq, float fm, float fmFreq, bool reset, float resetFrac, float masterFreq) 
	{
		phaseReset = false;
		if (fm > 0.0f) 
		{
			phaseFM += fmFreq / sampleRate;
			freq +=  fm * 10.0f * fmFreq * (1.0f + sinf(phaseFM * pi2));
			if (phaseFM > 1.0f) phaseFM -= 1.0f;
		}
		if (freq > 22000.0f) freq = 22000.0f;

		float fs = freq * sampleRateInv;
		x += fs;
		if (reset)
		{ 
			tmp =  masterFreq / sampleRate;
			float fracMaster = (fs * resetFrac)/tmp;
			mixInBlep(resetFrac/tmp, x - fracMaster);
			x = fracMaster;
		}

		if (x >= 1.0f) 
		{
			x -= 1.0f;
			mixInBlep(x/fs, 1.0f);
			phaseReset = true;
		}
		return getNextBlep() + x - 0.5f;
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
		value= buffer[bufferPos];
		return value;
	}
	
	#define LERP(A,B,F) ((B-A)*F+A)
	inline void mixInBlep(float offset, float scale) 
	{
		// Mix blep into buffer
		//for (int i=0; i<n; i++) 
		//{
		//	buffer[(bufferPos+i)&(n-1)]+= 1.0f - minBlep[(int)((i+offset)*oversampling)];
		//}
		int lpIn = (int)(oversampling*offset);
		float frac = fmod(offset * oversampling, 1.0f);
		for (int i = 0; i < n; i++, lpIn += (int)oversampling) 
		{
			buffer[(bufferPos + i) & (n - 1)] += scale - LERP(minBlep[lpIn], minBlep[lpIn+1], frac) * scale;
		}
	}

	inline float getNextSampleWithPhase(const float phase, const float freq) 
	{
		x= phase;
		if (x>=1.0f) 
		{
			x-= 1.0f;
			mixInBlep(x*(sampleRate/freq), 1.0f);
		}
		return getNextBlep()+x-0.5f;
	}
};
#endif