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

#ifndef __FilterHandler_h_
#define __FilterHandler_h_

#include "Decimator.h"
#include "interpolatorlinear.h"
#include "FilterLp12dB.h"
#include "FilterHp12dB.h"
#include "FilterBp12dB.h"

class FilterHandler
{
private:
	Decimator9 *decimator;
	InterpolatorLinear *interpolatorLinear;
	FilterLp12dB *filterLp12dB;
	FilterHp12dB *filterHp12dB;
	FilterBp12dB *filterBp12dB;

	float *upsampledValues;

public:
	FilterHandler(float sampleRate) 
	{
		decimator = new Decimator9();
		interpolatorLinear = new InterpolatorLinear();
		upsampledValues = new float[2];

		filterLp12dB = new FilterLp12dB(sampleRate);
		filterHp12dB = new FilterHp12dB(sampleRate);
		filterBp12dB = new FilterBp12dB(sampleRate);
	}

	~FilterHandler()
	{
		delete decimator;
		delete interpolatorLinear;
		delete filterLp12dB;
		delete filterHp12dB;
		delete filterBp12dB;
	}

	inline void process(float *input, float cutoff, float resonance, float modulation, int filterType) 
	{
		cutoff += modulation;

		interpolatorLinear->process2x(*input, upsampledValues);
		float rndFactor = 1.0f; // - cutoff * (float)rand()/RAND_MAX * 0.02f;

		// Scale cutoff
		cutoff = cutoff * cutoff * cutoff * cutoff;
		resonance = 1.0f - resonance;
		resonance = resonance * resonance;
		resonance = 1.0f - resonance;

		if (cutoff > 1.0f) cutoff = 1.0f;
		if (cutoff < 0.0f) cutoff = 0.0f;

		////// Do oversampled stuff here
		switch (filterType)
		{
		case 1: 
			filterLp12dB->process(&upsampledValues[0], cutoff * rndFactor, resonance, true);
			filterLp12dB->process(&upsampledValues[1], cutoff, resonance, false);
			break;
		case 2: 
			filterHp12dB->process(&upsampledValues[0], cutoff * rndFactor, resonance, true);
			filterHp12dB->process(&upsampledValues[1], cutoff, resonance, false);
			break;
		case 3: 
			filterBp12dB->process(&upsampledValues[0], cutoff * rndFactor, resonance, true);
			filterBp12dB->process(&upsampledValues[1], cutoff, resonance, false);
			break;
		}
		*input = decimator->Calc(upsampledValues[0], upsampledValues[1]);
	}
};
#endif

