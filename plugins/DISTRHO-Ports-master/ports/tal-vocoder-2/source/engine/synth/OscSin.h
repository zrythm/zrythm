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

#ifndef OscSin_H
#define OscSin_H

#include "BlepData.h"
#include "math.h"

class OscSin
{
public:
	const float *minBlep;
	const float oversampling;
	const int n;

	float *buffer;
	int bufferPos;

	float pi;
	float pi2;

	float sampleRate;
	float sampleRateInv;
	float phaseInc;
	float x;

	float phaseFM;

	OscSin(float sampleRate, int oversampling) :
                                oversampling(64.0f / oversampling),
                                n(32 * (int)oversampling),
				sampleRate(sampleRate),
				sampleRateInv(1.0f / sampleRate)
	{
		this->sampleRate= sampleRate;
		pi      = 3.1415926535897932384626433832795f;
		pi2     = 2.0f*pi;
		x		= 0.0f;
		phaseFM = -pi;

		BlepData *blepData= new BlepData();
		minBlep = blepData->getBlep();
        delete blepData;

		buffer= new float[n];
		bufferPos = 0;

		resetOsc(0.0f);
	}

    ~OscSin()
    {
        delete[] buffer;
    }

	void resetOsc(float phase)
	{
		x = phase;
		phaseFM = 0.0f;
		bufferPos= 0;
		for (int i=0; i<n; i++)
		{
			buffer[i]= 0.0f;
		}
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

		float fs = freq * sampleRateInv;
		if (reset)
		{
			float tmp =  masterFreq / sampleRate;
			float fracMaster = (fs * resetFrac) / tmp;
			mixInBlep(resetFrac / tmp, calcCos(x) - calcCos(fracMaster));
			x = fracMaster;
		}
        float value = calcCos(x);
		x += fs;
		if(x >= 1.0f)
		{
			x -= 1.0f;
		}
		return getNextBlep() + value - 0.5f;
	}

	inline float calcCos(const float x)
	{
		return -0.5f * cosf(x * pi2) + 0.5f;
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
	inline void mixInBlep(float offset, float scale)
	{
		int lpIn = (int)(oversampling*offset);
		float frac = fmod(offset * oversampling, 1.0f);
		for (int i = 0; i < n; i++, lpIn += (int)oversampling)
		{
			buffer[(bufferPos + i) & (n - 1)] += scale - LERP(minBlep[lpIn], minBlep[lpIn+1], frac) * scale;
		}
	}
};
#endif
