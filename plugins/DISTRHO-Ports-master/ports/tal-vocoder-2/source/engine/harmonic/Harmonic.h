/*
	==============================================================================
	This file is part of Tal-Vocoder by Patrick Kunz.

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

#if !defined(__Harmonic_h)
#define __Harmonic_h

#include "OnePoleLP.h"
#include "math.h"

class Harmonic 
{
public:
	float *delayLineStart;
	float *delayLineEnd;
	float *writePtr;

	int delayLineLength;
	float rate;
	float delayLineOutput;

	float sampleRate;
	float delayTime;

	// Runtime variables
	float offset, diff, frac, *ptr, *ptr2;

	int readPos;

	OnePoleLP *lp;
	float z1, z2;
	float mult, sign;

	// lfo 
	float lfoPhase, lfoStepSize, lfoSign;

	Harmonic(float sampleRate, float phase, float rate, float delayTime)
	{
		this->rate= rate;
		this->sampleRate= sampleRate;
		this->delayTime= delayTime;
		z1= z2= 0.0f;
		sign= 0;
		lfoPhase= phase*2.0f-1.0f;
		lfoStepSize= (4.0f*rate/sampleRate);
		lfoSign= 1.0f;

		//compute required buffer size for desired d0elay and allocate for it
		//add extra point to aid in interpolation later
		delayLineLength= ((int)floorf(delayTime*sampleRate*0.001f) + 1 +2000);
		delayLineStart= new float[delayLineLength];

		//set up pointers for delay line
		delayLineEnd= delayLineStart + delayLineLength;
		writePtr= delayLineStart;

		//zero out the buffer (silence)
		do {
			*writePtr= 0.0f;
		}
		while (++writePtr < delayLineEnd);

		//set read pointer to end of delayline. Setting it to the end
		//ensures the interpolation below works correctly to produce
		//the first non-zero sample.
		writePtr= delayLineStart + delayLineLength -1;
		delayLineOutput= 0.0f;
		lp= new OnePoleLP();
	}

	float process(float *sample) 
	{
		// Get delay time
		offset= (nextLFO()*0.3f+0.4f)*delayTime*sampleRate*0.001f;

		//compute the largest read pointer based on the offset.  If ptr
		//is before the first delayline location, wrap around end point
		ptr = writePtr-(int)floorf(offset);
		if (ptr<delayLineStart)
			ptr+= delayLineLength;

		ptr2= ptr-1;
		if (ptr2<delayLineStart)
			ptr2+= delayLineLength;

		frac= offset-(int)floorf(offset);
		delayLineOutput= *ptr2+*ptr*(1-frac)-(1-frac)*z1;
		z1 = delayLineOutput;

		// Low pass
		//lp->tick(&delayLineOutput, 0.95f);

		//write the input sample and any feedback to delayline
		*writePtr= *sample;

		//increment buffer index and wrap if necesary
		if (++writePtr>=delayLineEnd) {
			writePtr= delayLineStart;
		}
		return delayLineOutput;
	}

	inline float nextLFO()
	{
		if (lfoPhase>=1.0f) 
		{
			lfoSign= -1.0f;
		} 
		else if (lfoPhase<=-1.0f) 
		{
			lfoSign= +1.0f;
		}
		lfoPhase+= lfoStepSize*lfoSign;
		return lfoPhase;
	}
};

#endif