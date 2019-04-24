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

#ifndef EnvelopeManager_H
#define EnvelopeManager_H

#include "EnvelopeFollower.h"
#include "Params.h"
#include "../synth/OscNoise.h"

class EnvelopeManager 
{
private:
	static const int NUMBER_OF_BANDS = 12;
    static const int NUMBER_OF_CORNER_BINS = NUMBER_OF_BANDS + 1;

	int binPositionPerFrequency[NUMBER_OF_CORNER_BINS];

	float frequencies[NUMBER_OF_BANDS];
	float volumeCorrection[NUMBER_OF_BANDS];
	float volumeUser[NUMBER_OF_BANDS];
	float sumModulator[NUMBER_OF_BANDS];
	float envelopeReleaseCorrecturFactor[NUMBER_OF_BANDS];

	float sampleRate;
	int frameSize;

    float esserIntensity; 

	float alpha, beta, abs_inphase, abs_quadrature;

	EnvelopeFollower **envelopeFollowerModulator;
	EnvelopeFollower **envelopeFollowerCarrier;

    OscNoise *oscNoise;

public:
	// frameSize = (frameSize of FFT) / 2
	EnvelopeManager(float sampleRate, int frameSize)
	{
		this->sampleRate = sampleRate;
		this->frameSize = frameSize;
 
		// Vocoder band frequencies
		//frequencies[0] = 80.0f;
		//frequencies[1] = 200.0f;
		//frequencies[2] = 330.0f;
		//frequencies[3] = 470.0f;
		//frequencies[4] = 730.0f; 
		//frequencies[5] = 1030.0f;
		//frequencies[6] = 1500.0f;
		//frequencies[7] = 2150.0f;
		//frequencies[8] = 3500.0f;
		//frequencies[9] = 5900.0f;
		//frequencies[10] = 9000.0f; // to the end of the spectrum

		frequencies[0] = 0.0f;   
		frequencies[1] = 150.0f;
		frequencies[2] = 230.0f;
		frequencies[3] = 330.0f;
		frequencies[4] = 520.0f; 
		frequencies[5] = 760.0f;
		frequencies[6] = 1170.0f;
		frequencies[7] = 1600.0f;
		frequencies[8] = 2400.0f;
		frequencies[9] = 4000.0f;
		frequencies[10] = 6600.0f;
		frequencies[11] = 11000.0f; // end of last band

		volumeCorrection[0] = 0.1f;
		volumeCorrection[1] = 0.3f;
		volumeCorrection[2] = 0.5f;
		volumeCorrection[3] = 0.7f;
		volumeCorrection[4] = 1.0f;
		volumeCorrection[5] = 1.0f;
		volumeCorrection[6] = 1.0f;
		volumeCorrection[7] = 1.0f;
		volumeCorrection[8] = 1.0f;
		volumeCorrection[9] = 1.0f;
		volumeCorrection[10] = 1.0f;
		volumeCorrection[11] = 0.0f;

		envelopeReleaseCorrecturFactor[0] = 2.0f;   
		envelopeReleaseCorrecturFactor[1] = 2.0f;
		envelopeReleaseCorrecturFactor[2] = 2.0f;
		envelopeReleaseCorrecturFactor[3] = 2.0f;
		envelopeReleaseCorrecturFactor[4] = 2.0f; 
		envelopeReleaseCorrecturFactor[5] = 1.0f;
		envelopeReleaseCorrecturFactor[6] = 1.0f;
		envelopeReleaseCorrecturFactor[7] = 1.0f;
		envelopeReleaseCorrecturFactor[8] = 1.0f;
		envelopeReleaseCorrecturFactor[9] = 1.5f;
		envelopeReleaseCorrecturFactor[10] = 2.0f;
		envelopeReleaseCorrecturFactor[11] = 1.0f; // end of last band

		// initialize
		envelopeFollowerModulator = new EnvelopeFollower *[NUMBER_OF_BANDS];
        envelopeFollowerCarrier = new EnvelopeFollower *[NUMBER_OF_BANDS];
		for (int i = 0; i < NUMBER_OF_BANDS; i++)
		{
			envelopeFollowerModulator[i] = new EnvelopeFollower(sampleRate, 1.0f);
			envelopeFollowerCarrier[i] = new EnvelopeFollower(sampleRate, 1.0f);

		    envelopeFollowerModulator[i]->setReleaseTime(0.2f); 
		    envelopeFollowerCarrier[i]->setReleaseTime(0.3f);

            volumeUser[i] = 1.0f;
		}

		// calculate bin positions of bands
		float frequencyPerBin = (0.5f * sampleRate) / frameSize;
		for (int i = 0; i < NUMBER_OF_BANDS; i++)
		{
			float realFrequency = frequencies[i];
			binPositionPerFrequency[i] = (int)floorf(realFrequency / frequencyPerBin + 0.5f);
		}

        // corner frequencies
        binPositionPerFrequency[0] = 0; // starting point
        binPositionPerFrequency[NUMBER_OF_CORNER_BINS - 1] = frameSize; // max frequency

        this->oscNoise = new OscNoise(sampleRate);

        this->esserIntensity = 0.5f;
	}

	~EnvelopeManager(void)
	{
		delete[] envelopeFollowerModulator;
        delete oscNoise;
	}

    void setVocoderBandVolume(float value, int band)
    {
        this->volumeUser[band] = value * 2.0f;
    }

    void setReleaseTime(float value)
    {
        value = 0.05f + value * 0.3f;
		for (int i = 0; i < NUMBER_OF_BANDS; i++)
		{
            this->envelopeFollowerModulator[i]->setReleaseTime(value * envelopeReleaseCorrecturFactor[i]); 
		}
    }

    void setEsserIntensity(float value)
    {
        this->esserIntensity = value;
    }

	inline void process(float *inputRe, float *inputIm, float *carrierRe, float *carrierIm, float *resultRe, float *resultIm)
	{
        // Sum up stuff
		for (int i = 0; i < NUMBER_OF_BANDS; i++)
		{
			sumModulator[i] = 0.0f;
		}

        // calculate modulation intensity per band
        for (int i = 0; i < NUMBER_OF_BANDS; i++)
        {
            int startBin = binPositionPerFrequency[i];
            int endBin = binPositionPerFrequency[i + 1];

            for (int j = startBin; j <= endBin; j++)
            {
                float actualMagnitude = alpha_beta_magnitude(inputRe[j], inputIm[j]) * this->volumeCorrection[i] * this->volumeUser[i];
                sumModulator[i] = 0.5f * (sumModulator[i] + actualMagnitude);
            }

            // envelope stuff
            envelopeFollowerModulator[i]->process(sumModulator[i]);
        }

        // convolution stuff / multiply carrier with modulation
        for (int i = 0; i < NUMBER_OF_BANDS; i++)
        {
            float volumePerBand = envelopeFollowerModulator[i]->getActualValue() * volumeCorrection[i];

            float summedCarrier = 0.0f;
            for (int j = binPositionPerFrequency[i]; j <= binPositionPerFrequency[i + 1]; j++)
            {
                // sum carrier intensity
                summedCarrier = 0.5f * (summedCarrier + alpha_beta_magnitude(carrierRe[j], carrierIm[j]));

                if (i > 8)
                {
                    // mix in some modulation signal at higher frequencies for s and z's
                    float currentModulationValueCarrier = envelopeFollowerCarrier[i]->getActualValue();
                    if (currentModulationValueCarrier > 1.0f)
                    {
                        currentModulationValueCarrier = 1.0f;
                    }

                    float zischRe = esserIntensity * 20.0f * inputRe[j] * currentModulationValueCarrier;
                    float zischIm = esserIntensity * 20.0f * inputIm[j] * currentModulationValueCarrier;

                    float carrrierIntensity = (1.0f - esserIntensity) * 0.8f + 0.2f;
                    carrierRe[j] = carrierRe[j] * carrrierIntensity + zischRe;
                    carrierIm[j] = carrierRe[j] * carrrierIntensity + zischIm;
                }

                // Convolve
			    resultRe[j] = carrierRe[j] * volumePerBand;
			    resultIm[j] = carrierIm[j] * volumePerBand;
            }

            envelopeFollowerCarrier[i]->process(summedCarrier);
        }
	}

	inline float alpha_beta_magnitude(float inphase, float quadrature)
	{
		// very fast approximation of sqrt(Re*Re + Im*Im)
		/* magnitude ~= alpha * max(|I|, |Q|) + beta * min(|I|, |Q|) */
		// take one of the following alpha and beta values
		/*
		{ "Min RMS Err",      0.947543636291, 0.3924854250920 },
		{ "Min Peak Err",     0.960433870103, 0.3978247347593 },
		{ "Min RMS w/ Avg=0", 0.948059448969, 0.3926990816987 }, 
		{ "1, Min RMS Err",              1.0,     0.323260990 },
		{ "1, Min Peak Err",             1.0,     0.335982538 },
		{ "1, 1/2",                      1.0,      1.0 / 2.0  },
		{ "1, 1/4",                      1.0,      1.0 / 4.0  },
		{ "Frerking",                    1.0,            0.4  },
		{ "1, 11/32",                    1.0,     11.0 / 32.0 },
		{ "1, 3/8",  
		1.0,      3.0 / 8.0  },
		{ "15/16, 15/32",        15.0 / 16.0,     15.0 / 32.0 },
		{ "15/16, 1/2",          15.0 / 16.0,      1.0 / 2.0  },
		{ "31/32, 11/32",        31.0 / 32.0,     11.0 / 32.0 },
		{ "31/32, 3/8",          31.0 / 32.0,      3.0 / 8.0  },
		{ "61/64, 3/8",          61.0 / 64.0,      3.0 / 8.0  },
		{ "61/64, 13/32",        61.0 / 64.0,     13.0 / 32.0 }
		*/

		alpha = 0.960433870103f;
		beta  = 0.3978247347593f;

		abs_inphase = fabs(inphase);
		abs_quadrature = fabs(quadrature);

		if (abs_inphase > abs_quadrature) {
			return alpha * abs_inphase + beta * abs_quadrature;
		} 
		else 
		{
			return alpha * abs_quadrature + beta * abs_inphase;
		}
	}
};
#endif
