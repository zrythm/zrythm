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

#include "TalReverb.h"
#include "AudioUtils.h"
#include "Params.h"
#include "NoiseGenerator.h"

class ReverbEngine 
{
public:
	TalReverb* reverb;
	NoiseGenerator *noiseGenerator;

	float dry;
	float wet;
	float stereoWidth;

	AudioUtils audioUtils;

	ReverbEngine(float sampleRate) 
	{
		initialize(sampleRate);
	}

	~ReverbEngine()
	{
		delete reverb;
		delete noiseGenerator;
	}

	void setDry(float dry)
	{
		this->dry = audioUtils.getLogScaledVolume(dry, 1.0f);
	}

	void setWet(float wet)
	{
		this->wet = audioUtils.getLogScaledVolume(wet, 1.0f);
	}

	void setDecayTime(float decayTime)
	{
		reverb->setDecayTime(decayTime);
	}

	void setPreDelay(float preDelay)
	{
		reverb->setPreDelay(preDelay);
	}

	void setLowCut(float value)
	{
		reverb->setLowCut(value);
	}

	void setHighCut(float value)
	{
		reverb->setHighCut(value);
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
		reverb = new TalReverb(sampleRate);
		noiseGenerator = new NoiseGenerator(sampleRate);

		dry = 1.0f;
		wet = 0.5f;
		stereoWidth = 1.0f;
	}

	void process(float *sampleL, float *sampleR) 
	{
        if (wet > 0.0f)
        {
		    float noise = noiseGenerator->tickNoise() * 0.000000001f;

		    *sampleL += noise;
		    *sampleR += noise;

		    float drysampleL = *sampleL;
		    float drysampleR = *sampleR;

		    reverb->process(sampleL, sampleR);

		    // Process Stereo
		    float wet1 = wet * (stereoWidth * 0.5f + 0.5f);
		    float wet2 = wet * ((1.0f - stereoWidth) * 0.5f);
		    float resultL = *sampleL * wet1 + *sampleR * wet2 + drysampleL * dry;
		    float resultR = *sampleR * wet1 + *sampleL * wet2 + drysampleR * dry;
		    *sampleL = resultL;
		    *sampleR = resultR;
        }
	}
};
#endif

