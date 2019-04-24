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

#ifndef __FilterHandler_h_
#define __FilterHandler_h_

#include "Decimator.h"
#include "Interpolatorlinear.h"
#include "FilterLp24db.h"
#include "FilterLp18db.h"
#include "FilterLp12db.h"
#include "FilterLp06db.h"
#include "FilterHp24db.h"
#include "FilterBp24db.h"
#include "FilterN24db.h"

class FilterHandler
{
private:
	Decimator9 *decimator;
	Decimator9 *decimator2;
	Upsample *upsample;
	InterpolatorLinear *interpolatorLinear;

	FilterLp24db *filterLp24db;
	FilterLp18db *filterLp18db;
	FilterLp12db *filterLp12db;
	FilterLp06db *filterLp06db;

	FilterHp24db *filterHp24db;
	FilterBp24db *filterBp24db;
	FilterN24db *filterN24db;

	int filtertype;
    float *upsampledValues;
    

public:
	FilterHandler(float sampleRate) 
	{
		upsample = new Upsample();
		decimator = new Decimator9();
		decimator2 = new Decimator9();
		interpolatorLinear = new InterpolatorLinear();
        upsampledValues = new float[4];

		filterLp24db = new FilterLp24db(sampleRate * 4.0f);
		filterLp18db = new FilterLp18db(sampleRate * 4.0f);
		filterLp12db = new FilterLp12db(sampleRate * 4.0f);
		filterLp06db = new FilterLp06db(sampleRate * 4.0f);
		filterHp24db = new FilterHp24db(sampleRate * 4.0f);
		filterBp24db = new FilterBp24db(sampleRate * 4.0f);
		filterN24db = new FilterN24db(sampleRate * 4.0f);

        filtertype = 0;
    }

	~FilterHandler()
	{
		delete upsample;
		delete decimator;
		delete decimator2;
		delete interpolatorLinear;
		delete filterLp24db;
		delete filterLp18db;
		delete filterLp12db;
		delete filterLp06db;
		delete filterHp24db;
		delete filterBp24db;
		delete filterN24db;
		delete upsampledValues;
	}

	void setFiltertype(float value)
	{
        this->filtertype = (int)value;
	}

    void reset()
    {
        decimator->Initialize();
        decimator2->Initialize();
        filterLp24db->reset();
		filterLp18db->reset();
		filterLp12db->reset();
		filterLp06db->reset();
		filterHp24db->reset();
		filterBp24db->reset();
		filterN24db->reset();
    }

	inline void process(float *input, float cutoff, float resonance) 
	{
		interpolatorLinear->process4x(*input, upsampledValues);

		// Do oversampled stuff here
		switch (filtertype)
		{
		case 1: 
			filterLp24db->process(&upsampledValues[0], cutoff, resonance, true);
			filterLp24db->process(&upsampledValues[1], cutoff, resonance, false);
			filterLp24db->process(&upsampledValues[2], cutoff, resonance, false);
			filterLp24db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		case 2: 
			filterLp18db->process(&upsampledValues[0], cutoff, resonance, true);
			filterLp18db->process(&upsampledValues[1], cutoff, resonance, false);
			filterLp18db->process(&upsampledValues[2], cutoff, resonance, false);
			filterLp18db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		case 3: 
			filterLp12db->process(&upsampledValues[0], cutoff, resonance, true);
			filterLp12db->process(&upsampledValues[1], cutoff, resonance, false);
			filterLp12db->process(&upsampledValues[2], cutoff, resonance, false);
			filterLp12db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		case 4: 
			filterLp06db->process(&upsampledValues[0], cutoff, resonance, true);
			filterLp06db->process(&upsampledValues[1], cutoff, resonance, false);
			filterLp06db->process(&upsampledValues[2], cutoff, resonance, false);
			filterLp06db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		case 5: 
			filterHp24db->process(&upsampledValues[0], cutoff, resonance, true);
			filterHp24db->process(&upsampledValues[1], cutoff, resonance, false);
			filterHp24db->process(&upsampledValues[2], cutoff, resonance, false);
			filterHp24db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		case 6: 
            filterBp24db->process(&upsampledValues[0], cutoff, resonance, true);
			filterBp24db->process(&upsampledValues[1], cutoff, resonance, false);
			filterBp24db->process(&upsampledValues[2], cutoff, resonance, false);
			filterBp24db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		case 7: 
            filterN24db->process(&upsampledValues[0], cutoff, resonance, true);
			filterN24db->process(&upsampledValues[1], cutoff, resonance, false);
			filterN24db->process(&upsampledValues[2], cutoff, resonance, false);
			filterN24db->process(&upsampledValues[3], cutoff, resonance, false);
			break;
		}

		float decimated1 = decimator->Calc(upsampledValues[0], upsampledValues[1]);
		float decimated2 = decimator->Calc(upsampledValues[2], upsampledValues[3]);
		*input = decimator2->Calc(decimated1, decimated2);
	}
};
#endif

