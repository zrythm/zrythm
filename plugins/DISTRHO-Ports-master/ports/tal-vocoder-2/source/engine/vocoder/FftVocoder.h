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

#if !defined(__FftVocoder_h)
#define __FftVocoder_h

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "Fft.h"
#include "EnvelopeManager.h"

#define M_PI 3.14159265358979323846
#define MAX_FRAME_LENGTH 8192

class FftVocoder 
{
public:
	static const int GAIN = 36; 

	static const int NUMBER_OF_BANDS = 11;
	float sampleRate;
	int oversampling;

	Fft *smsFft;

	int powerOfTwo;
	int fftFrameSize;
	int fftFrameSize2;
	int stepSize;
	int inFifoLatency;

	float normalizedFreq;

	float gInFIFOInput[MAX_FRAME_LENGTH];
	float gInFIFOCarrier[MAX_FRAME_LENGTH];
	float gOutFIFO[MAX_FRAME_LENGTH];

	float gFftInputRe[2*MAX_FRAME_LENGTH];
	float gFftInputIm[2*MAX_FRAME_LENGTH];

	float gFftCarrierRe[2*MAX_FRAME_LENGTH];
	float gFftCarrierIm[2*MAX_FRAME_LENGTH];

	float gFftResultRe[2*MAX_FRAME_LENGTH];
	float gFftResultIm[2*MAX_FRAME_LENGTH];

	float gOutputAccum[2*MAX_FRAME_LENGTH];

	float *windowTable;

	int gRover;

	EnvelopeManager *envelopeManager;

	// BufferSize must be power of two
	FftVocoder(long oversampling, float sampleRate, int bufferSize) 
	{
		this->oversampling	= oversampling;
		this->sampleRate	= sampleRate;

		smsFft = new Fft();

		// calculate power for FFT
		powerOfTwo = (int)(logf((float)bufferSize)/logf(2.0) + 0.5f);

		// set up some handy variables
		fftFrameSize	= bufferSize;
		fftFrameSize2	= fftFrameSize/2;
		stepSize		= fftFrameSize/oversampling;
		normalizedFreq	= sampleRate/(float)fftFrameSize;
		inFifoLatency	= fftFrameSize-stepSize;

		gRover = inFifoLatency;

		// initialize our static arrays 
		memset(gInFIFOInput, 0, MAX_FRAME_LENGTH*sizeof(float));
		memset(gInFIFOCarrier, 0, MAX_FRAME_LENGTH*sizeof(float));
		memset(gOutFIFO, 0, MAX_FRAME_LENGTH*sizeof(float));

		memset(gFftInputRe, 0, 2*MAX_FRAME_LENGTH*sizeof(float));
		memset(gFftInputIm, 0, 2*MAX_FRAME_LENGTH*sizeof(float));		
		
		memset(gFftCarrierRe, 0, 2*MAX_FRAME_LENGTH*sizeof(float));
		memset(gFftCarrierIm, 0, 2*MAX_FRAME_LENGTH*sizeof(float));

		memset(gFftResultRe, 0, 2*MAX_FRAME_LENGTH*sizeof(float));
		memset(gFftResultIm, 0, 2*MAX_FRAME_LENGTH*sizeof(float));

		memset(gOutputAccum, 0, 2*MAX_FRAME_LENGTH*sizeof(float));

		// Precalculate window
		windowTable = new float[fftFrameSize];
		for (int k = 0; k < fftFrameSize; k++)
		{
			windowTable[k] = 0.5f*(1.0f - cosf(2.0f* (float)M_PI*(float)k/(float)fftFrameSize))* (float)GAIN;
		}

		envelopeManager = new EnvelopeManager(sampleRate, fftFrameSize2);
	}

	~FftVocoder()
	{
		delete envelopeManager;
		delete windowTable;
	}

    EnvelopeManager* getEnvelopeManager()
    {
        return this->envelopeManager;
    }

	inline void process(float input, float carrier, float *out)
	{
		/* As long as we have not yet collected enough data just read in */
		gInFIFOInput[gRover] = input;
		gInFIFOCarrier[gRover] = carrier;
		*out = gOutFIFO[gRover - inFifoLatency];
		gRover++;

		/* Now we have enough data for processing */
		if (gRover >= fftFrameSize) 
		{
			gRover = inFifoLatency;

			// Do windowing and re,im interleave
			for (int k = 0; k < fftFrameSize; k++) 
			{
				gFftInputRe[k] = gInFIFOInput[k] * windowTable[k];
				gFftInputIm[k] = 0.0f;

				gFftCarrierRe[k] = gInFIFOCarrier[k] * windowTable[k];
				gFftCarrierIm[k] = 0.0f;
			}

			// Analyse 
			//*******************************************************************/
			// Do FFT transform
			smsFft->FFT2(1, powerOfTwo, gFftInputRe, gFftInputIm);
			smsFft->FFT2(1, powerOfTwo, gFftCarrierRe, gFftCarrierIm);

			/* Convolution */
			envelopeManager->process(gFftInputRe, gFftInputIm, gFftCarrierRe, gFftCarrierIm, gFftResultRe, gFftResultIm);

            // Easy vocoder
			//for (int k = 0; k <= fftFrameSize2; k++) 
			//{
			//	// Envelope input signal
			//	//magnitude = 2* alpha_beta_magnitude(real, imag);

			//	gFftResultRe[k] =  gFftCarrierRe[k] * gFftInputRe[k];
			//	gFftResultIm[k] =  gFftCarrierIm[k];
			//}

			/* zero negative frequencies */
			for (int k = fftFrameSize2+1; k < fftFrameSize; k++)
			{
				gFftResultRe[k] = 0.0f;
				gFftResultIm[k] = 0.0f;
			}

			/* Do inverse transform */
			smsFft->FFT2(-1, powerOfTwo, gFftResultRe, gFftResultIm);

			/* Do windowing and add to output accumulator */
			for(int k = 0; k < fftFrameSize; k++) 
			{
				gOutputAccum[k] += 2.0f*windowTable[k]*gFftResultRe[k]/(fftFrameSize2*oversampling);
			}
			for (int k = 0; k < stepSize; k++)
			{
				gOutFIFO[k] = gOutputAccum[k];
			}
			/* shift accumulator */
			memmove(gOutputAccum, gOutputAccum+stepSize, fftFrameSize*sizeof(float));
			
			/* move input FIFO */
			for (int k = 0; k < inFifoLatency; k++)
			{
				gInFIFOInput[k] = gInFIFOInput[k+stepSize];
				gInFIFOCarrier[k] = gInFIFOCarrier[k+stepSize];
			}
		}
	}
};

#endif

