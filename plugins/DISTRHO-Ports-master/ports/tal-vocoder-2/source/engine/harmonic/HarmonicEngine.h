/*-----------------------------------------------------------------------------

Kunz Patrick 30.04.2007

Chorus-60 FX

-----------------------------------------------------------------------------*/

#if !defined(__HarmonicEngine_h)
#define __HarmonicEngine_h

#include "Harmonic.h"
#include "Params.h"
#include "DCBlock.h"
//#include "Tools.h"


class HarmonicEngine 
{
public:
	Harmonic *harmonic1L;
	Harmonic *harmonic1R;

	DCBlock *dcBlock1L;
	DCBlock *dcBlock1R;

	float resultL, resultR;

    float intensity; 

	HarmonicEngine(float sampleRate) 
	{
		this->setUpHarmonic(sampleRate);

		dcBlock1L= new DCBlock();
		dcBlock1R= new DCBlock();

        intensity = 0.0f;
	}

    void setIntensity(float value)
    {
        this->intensity = value;
    }

	void setSampleRate(float sampleRate)
	{
		setUpHarmonic(sampleRate);
	}

	void setUpHarmonic(float sampleRate)
	{
		// Phase 126
		harmonic1L= new Harmonic(sampleRate, 1.0f, 0.5f, 7.0f);
		harmonic1R= new Harmonic(sampleRate, 0.0f, 0.5f, 7.0f);
	}

	inline void process(float *sampleL) 
	{
        if (this->intensity > 0.0f)
        {
		    resultR= 0.0f;
		    resultL= 0.0f;

		    resultL+= harmonic1L->process(sampleL);
		    resultR+= harmonic1R->process(sampleL);
		    dcBlock1L->tick(&resultL, 0.5f);
		    dcBlock1R->tick(&resultR, 0.5f);

		    resultL*= 1.4f;
		    resultR*= 1.4f;
		    *sampleL= *sampleL + (resultL + resultR) * this->intensity;
        }
	}
};

#endif

