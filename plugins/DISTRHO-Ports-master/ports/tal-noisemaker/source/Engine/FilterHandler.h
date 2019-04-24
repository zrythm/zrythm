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
#include "FilterStateVariable12db.h"

#include "FilterMoog24.h"

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

    FilterStateVariable12db *filterStateVariableLp12db;
    FilterStateVariable12db *filterStateVariableHp12db;
    FilterStateVariable12db *filterStateVariableBp12db;

    FilterMoog24 *filterMoog24;

	int filtertype;
    float filterDrive;

    float *upsampledValues;
    

public:
	FilterHandler(float sampleRate) 
	{
		this->upsample = new Upsample();
		this->decimator = new Decimator9();
		this->decimator2 = new Decimator9();
		this->interpolatorLinear = new InterpolatorLinear();
        this->upsampledValues = new float[4];

		this->filterLp24db = new FilterLp24db(sampleRate * 4.0f);
		this->filterLp18db = new FilterLp18db(sampleRate * 4.0f);
		this->filterLp12db = new FilterLp12db(sampleRate * 4.0f);
		this->filterLp06db = new FilterLp06db(sampleRate * 4.0f);
		this->filterHp24db = new FilterHp24db(sampleRate * 4.0f);
		this->filterBp24db = new FilterBp24db(sampleRate * 4.0f);
		this->filterN24db = new FilterN24db(sampleRate * 4.0f);

        filterStateVariableLp12db = new FilterStateVariable12db(sampleRate * 4.0f, 0);
        filterStateVariableHp12db = new FilterStateVariable12db(sampleRate * 4.0f, 1);
        filterStateVariableBp12db = new FilterStateVariable12db(sampleRate * 4.0f, 2);

        this->filterMoog24 = new FilterMoog24(sampleRate);

        this->filtertype = 0;
        this->filterDrive = 0.0f;
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
        delete filterStateVariableLp12db;
        delete filterStateVariableHp12db;
        delete filterStateVariableBp12db;
		delete[] upsampledValues;
	}

	void setFiltertype(float value)
	{
        this->filtertype = (int)value;
	}

    void setFilterDrive(float value)
    {
        this->filterDrive = value * 10.0f;
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
        filterStateVariableLp12db->reset();
        filterStateVariableHp12db->reset();
        filterStateVariableBp12db->reset();
    }

	inline void process(float *input, float cutoff, float resonance) 
	{
        *input *= filterDrive + 1.0f;
        
        if (filtertype > 10)
        {
            *input /= 4.0f;

		    switch (filtertype)
		    {
		    case 11: 
                this->filterMoog24->Process(input, cutoff, resonance, false, true);
                break;
		    case 12: 
                // this->filterMoog24->Process(input, cutoff, resonance, true, true);
                break;
            }
            *input *= 4.0f;
            *input *= 3.0f;
        }
        else
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
		    case 8: 
                filterStateVariableLp12db->process(&upsampledValues[0], cutoff, resonance, true);
			    filterStateVariableLp12db->process(&upsampledValues[1], cutoff, resonance, false);
			    filterStateVariableLp12db->process(&upsampledValues[2], cutoff, resonance, false);
			    filterStateVariableLp12db->process(&upsampledValues[3], cutoff, resonance, false);
			    break;
		    case 9: 
                filterStateVariableHp12db->process(&upsampledValues[0], cutoff, resonance, true);
			    filterStateVariableHp12db->process(&upsampledValues[1], cutoff, resonance, false);
			    filterStateVariableHp12db->process(&upsampledValues[2], cutoff, resonance, false);
			    filterStateVariableHp12db->process(&upsampledValues[3], cutoff, resonance, false);
			    break;
		    case 10: 
                filterStateVariableBp12db->process(&upsampledValues[0], cutoff, resonance, true);
			    filterStateVariableBp12db->process(&upsampledValues[1], cutoff, resonance, false);
			    filterStateVariableBp12db->process(&upsampledValues[2], cutoff, resonance, false);
			    filterStateVariableBp12db->process(&upsampledValues[3], cutoff, resonance, false);
			    break;
		    }

		    float decimated1 = decimator->Calc(upsampledValues[0], upsampledValues[1]);
		    float decimated2 = decimator->Calc(upsampledValues[2], upsampledValues[3]);
		    *input = decimator2->Calc(decimated1, decimated2);
        
        }
        *input /= filterDrive * 0.5f + 1.0f;
	}
};
#endif

