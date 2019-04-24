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


#if !defined(__Engine_h)
#define __Engine_h

#include "../EnvelopeEditor/EnvelopeEditor.h"
#include "../Filter/FilterHandler.h"
#include "AudioUtils.h"
#include "../Filter/OscNoise.h"

class Engine 
{
private:
    EnvelopeEditor* envelopeEditor;
    FilterHandler* filterHandlerL;
    FilterHandler* filterHandlerR;
    OscNoise* oscNoise;

    float resonance;
    float volumeIn;
    float volumeOut;
    float depth;
    float invScaledDepth;

    int filterType; 

    AudioUtils audioUtils;

    void processPan(float *sampleL, float* sampleR, float value)
    {
        *sampleL *= sqrtf(1.0f - value);
        *sampleR *= sqrtf(value);
    }

public:
	Engine(float sampleRate) 
	{
        this->resonance = 0.0f;
        this->volumeIn = 1.0f;
        this->volumeOut = 1.0f;
        this->depth = 1.0f;
        this->invScaledDepth = 1.0f;
        this->filterType = 1;

        this->initialize(sampleRate);
	}

	~Engine()
	{
        delete envelopeEditor;
	}

    EnvelopeEditor* getEnvelopeEditor()
    {
        return envelopeEditor;
    }

	void setSampleRate(float sampleRate)
	{
		initialize(sampleRate);
	}

	void initialize(float sampleRate)
	{
        envelopeEditor = new EnvelopeEditor(sampleRate);
        this->filterHandlerL = new FilterHandler(sampleRate);
        this->filterHandlerR = new FilterHandler(sampleRate);

        this->filterHandlerL->reset();
        this->filterHandlerR->reset();

        this->oscNoise = new OscNoise(sampleRate);
	}

    void sync(float bpm, AudioPlayHead::CurrentPositionInfo pos)
    {
        this->envelopeEditor->sync(bpm, pos);
    }

    void setPoints(Array<SplinePoint*> splinePoints)
    {
        this->envelopeEditor->setPoints(splinePoints);
    }

    void setSpeedFactor(int index)
    {
        this->envelopeEditor->setSpeedFactor(index);
    }

    void setFilterType(int index)
    {
        this->filterType = index;

        if (this->filterType <= 7)
        {
            this->filterHandlerL->setFiltertype((float)index);
            this->filterHandlerR->setFiltertype((float)index);
        }
    }

    void setResonance(float value)
    {
        this->resonance = value;
    }

    void setVolumeIn(float value)
    {
        this->volumeIn = audioUtils.getLogScaledVolume(value, 2.0f);
    }

    void setDepth(float value)
    {
        this->depth = audioUtils.getLogScaledValue(value, 1.0f);
        this->invScaledDepth = 1.0f - audioUtils.getLogScaledValue(1.0f - value, 1.0f);
    }

    void setVolumeOut(float value)
    {
        this->volumeOut = audioUtils.getLogScaledVolume(value, 2.0f);
    }

	void process(float *sampleL, float *sampleR) 
	{
        float value = this->envelopeEditor->process();

        switch (this->filterType)
        {
        case 5:
            value = this->depth * value * value * value * value;
            break;
        case 9:
            value = (value - 0.5f) * this->depth + 0.5f;
            break;
        default:
            value = (1.0f - this->invScaledDepth) + this->invScaledDepth * value * value * value * value;
        }
        
        float denormalisationNoise = this->oscNoise->getNextSample() * 0.0000000000001f;

        *sampleL *= volumeIn + denormalisationNoise;
        *sampleR *= volumeIn + denormalisationNoise;

        switch (this->filterType)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            this->filterHandlerL->process(sampleL, value, resonance);
            this->filterHandlerR->process(sampleR, value, resonance);
            break;
        case 8:
            *sampleL *= value;
            *sampleR *= value;
            break;
        case 9:
            processPan(sampleL, sampleR, value);
            break;
        default:
            break;
        }
        *sampleL *= volumeOut;
        *sampleR *= volumeOut;
	}
};
#endif

