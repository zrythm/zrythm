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


#if !defined(__ReverbEngine_h)
#define __ReverbEngine_h

#include "Reverb.h"
#include "Params.h"
#include "ParamChangeUtil.h"

class ReverbEngine 
{
public:
	float *param;
	TalReverb* reverbL;
	TalReverb* reverbR;

	float drysampleL;
	float drysampleR;

	ParamChangeUtil* roomSizeParamChange;
	ParamChangeUtil* wetParamChange;

	ReverbEngine(float sampleRate) 
	{
		Params *params= new Params();
		this->param= params->parameters;
		initialize(sampleRate);
	}

	~ReverbEngine()
	{
		delete reverbL;
		delete reverbR;
	}

	void setSampleRate(float sampleRate)
	{
		initialize(sampleRate);
	}

	void initialize(float sampleRate)
	{
		reverbL = new TalReverb((int)sampleRate);
		reverbR = new TalReverb((int)sampleRate);

		roomSizeParamChange = new ParamChangeUtil(sampleRate, 300.0f);
		wetParamChange = new ParamChangeUtil(sampleRate, 300.0f);
	}

	void process(float *sampleL, float *sampleR) 
	{
		drysampleL = *sampleL;
		drysampleR = *sampleR;
		reverbL->process(sampleL, sampleR, 
			wetParamChange->tick(param[WET]), 
			roomSizeParamChange->tick(param[ROOMSIZE]), 
			param[PREDELAY], param[LOWCUT], param[DAMP], param[HIGHCUT], param[STEREO]);
		float gainValue = param[DRY] * param[DRY] * 1.5f;
		*sampleL += drysampleL * gainValue;
		*sampleR += drysampleR * gainValue;
	}
};
#endif

