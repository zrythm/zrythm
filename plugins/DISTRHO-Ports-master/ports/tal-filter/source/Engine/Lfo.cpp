//============================================================================
/**
Implementation file for LFO.hpp

@author		Remy Muller
@date		20030822
*/
//============================================================================

#include <cmath>
#include "Lfo.h"

Lfo::Lfo(float samplerate_) : phase(0), inc(0), samplerate(samplerate_) {
	setWaveform(0);   
	setWaveform(1);   
	setWaveform(2);   
	setWaveform(3);   
	setWaveform(4);   
	setRate(1.0f); //1Hz

	noiseOsc= new NoiseOsc(samplerate);
	randomValue= 0.0f;
	randomValueOld= 0.0f;
	resultSmooth= 0.0f;
}

// static const float k1Div24lowerBits =1.0f/16777216.0f; //(float)(1<<24);

float Lfo::tick(int waveform)
{
	freqWrap= false;
	if (phase >= 256.0f) {
		phase -= 256.0f;
		freqWrap = true;
	}
	i= (int)floorf(phase);
	frac = phase-i;

	// increment the phase for the next tick
	phase += inc;

	if (waveform == 0)
		result = tableSin[i]*(1.0f-frac) + tableSin[i+1]*frac; // linear interpolation
	else if (waveform == 1)
		result = tableTri[i]*(1.0f-frac) + tableTri[i+1]*frac; // linear interpolation
	else if (waveform == 2)
		result = tableSaw[i]*(1.0f-frac) + tableSaw[i+1]*frac; // linear interpolation
	else if (waveform == 3)
		result = tableRec[i]*(1.0f-frac) + tableRec[i+1]*frac; // linear interpolation
	else if (waveform == 4) {
		// Random
		if (freqWrap) {
			randomValue = ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
		}
		result = randomValue;
	} else {
		result = noiseOsc->getNextSample();
	}
	resultSmooth= (resultSmooth*19+result)*0.05f;
	return resultSmooth;
}

void Lfo::resetPhase()
{
	phase= 0.0f;
	randomValue=  ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
}

void Lfo::setRate(float rate)
{
	// The rate in Hz is converted to a phase increment with the following formula
	inc =  256.0f*rate/samplerate;
}

void Lfo::setWaveform(int index)
{
	switch(index)
	{
	case 0:
		{
			float pi= 4.0f*atanf(1.0f);

			int i;
			for(i=0;i<=256;i++)
				tableSin[i]= sinf(2.0f*pi*(i/256.0f));

			break;
		}
	case 1:
		{
			int i;
			for(i=0;i<64;i++)
			{
				tableTri[i]     =        i / 64.0f;
				tableTri[i+64]  =   (64-i) / 64.0f;
				tableTri[i+128] =      - i / 64.0f;
				tableTri[i+192] = - (64-i) / 64.0f;
			}
			tableTri[256] = 0.0f;
			break;
		}
	case 2:
		{
			int i;
			for(i=0;i<256;i++)
			{
				tableSaw[i] = 2.0f*(i/255.0f) - 1.0f;
			}
			tableSaw[256] = -1.0f;
			break;
		}
	case 3:
		{
			int i;
			for(i=0;i<128;i++)
			{
				tableRec[i]     =  1.0f;
				tableRec[i+128] = -1.0f;
			}
			tableRec[256] = 1.0f;
			break;
		}
	case 4:
		{
			/* symetric exponent similar to triangle */
			int i;
			float e = (float)exp(1.0f);
			for(i=0;i<128;i++)
			{
				tableExp[i]				= 2.0f * ((exp(i/128.0f) - 1.0f) / (e - 1.0f)) - 1.0f  ;
				tableExp[i+128]		= 2.0f * ((exp((128-i)/128.0f) - 1.0f) / (e - 1.0f)) - 1.0f  ;
			}
			tableExp[256] = -1.0f;
			break;
		}
	default:
		{
			break;
		}
	} 
}
