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
#include "AudioUtils.h"
#include "Params.h"
#include "ParamChangeUtil.h"
#include "NoiseGenerator.h"

class ReverbEngine 
{
public:
	float *param;
	TalReverb* reverb;

	ParamChangeUtil* dryParamChange;
	ParamChangeUtil* wetParamChange;

	NoiseGenerator *noiseGenerator;

	float dry;
	float wet;
	float stereoWidth;

	AudioUtils audioUtils;

	ReverbEngine(float sampleRate) 
	{
		Params *params= new Params();
		this->param= params->parameters;
		initialize(sampleRate);
	}

	~ReverbEngine()
	{
		delete reverb;

		delete noiseGenerator;
	}

	void setDry(float dry)
	{
		this->dry = audioUtils.getLogScaledVolume(dry, 2.0f);
	}

	void setWet(float wet)
	{
		this->wet = audioUtils.getLogScaledVolume(wet, 2.0f);
	}

	void setDecayTime(float decayTime)
	{
		reverb->setDecayTime(decayTime);
	}

	void setPreDelay(float preDelay)
	{
		reverb->setPreDelay(preDelay);
	}

	void setLowShelfGain(float lowShelfGain)
	{
		reverb->setLowShelfGain(lowShelfGain);
	}

	void setHighShelfGain(float highShelfGain)
	{
		reverb->setHighShelfGain(highShelfGain);
	}

	void setLowShelfFrequency(float lowShelfFrequency)
	{
		reverb->setLowShelfFrequency(lowShelfFrequency);
	}

	void setHighShelfFrequency(float highShelfFrequency)
	{
		reverb->setHighShelfFrequency(highShelfFrequency);
	}

	void setPeakFrequency(float peakFrequency)
	{
		reverb->setPeakFrequency(peakFrequency);
	}

	void setPeakGain(float peakGain)
	{
		reverb->setPeakGain(peakGain);
	}

	void setStereoWidth(float stereoWidth)
	{
		this->stereoWidth = stereoWidth;
	}

	void setStereoMode(float stereoMode)
	{
		reverb->setStereoMode(stereoMode > 0.0f ? true : false);
	}

	void setSampleRate(float sampleRate)
	{
		initialize(sampleRate);
	}

	void initialize(float sampleRate)
	{
        if (sampleRate <= 0)
        {
            sampleRate = 44100.0f;
        }

		reverb = new TalReverb((int)sampleRate);

		dryParamChange = new ParamChangeUtil(sampleRate, 300.0f);
		wetParamChange = new ParamChangeUtil(sampleRate, 300.0f);

		noiseGenerator = new NoiseGenerator(sampleRate);

		dry = 1.0f;
		wet = 0.5f;
		stereoWidth = 1.0f;
	}

	void process(float *sampleL, float *sampleR) 
	{
		// avoid cpu spikes
		float noise = noiseGenerator->tickNoise() * 0.000000001f;

		*sampleL += noise;
		*sampleR += noise;

		float drysampleL = *sampleL;
		float drysampleR = *sampleR;

		reverb->process(sampleL, sampleR);

		// Process Stereo
		float actualDryValue = dryParamChange->tick(dry);
		float wet1 = wet * (stereoWidth * 0.5f + 0.5f);
		float wet2 = wet * ((1.0f - stereoWidth) * 0.5f);
		float resultL = *sampleL * wet1 + *sampleR * wet2 + drysampleL * actualDryValue;
		float resultR = *sampleR * wet1 + *sampleL * wet2 + drysampleR * actualDryValue;
		*sampleL = resultL;
		*sampleR = resultR;
	}
};
#endif

