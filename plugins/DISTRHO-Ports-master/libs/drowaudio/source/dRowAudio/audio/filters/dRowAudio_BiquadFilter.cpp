/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

//==============================================================================
void BiquadFilter::processSamples (float* const samples,
                                   const int numSamples) noexcept
{
    IIRFilter::processSamples (samples, numSamples);
}

void BiquadFilter::processSamples (int* const samples,
								   const int numSamples) noexcept
{
    const SpinLock::ScopedLockType sl (processLock);
    
    if (active)
    {
        const float c0 = coefficients.coefficients[0];
        const float c1 = coefficients.coefficients[1];
        const float c2 = coefficients.coefficients[2];
        const float c3 = coefficients.coefficients[3];
        const float c4 = coefficients.coefficients[4];
        float lv1 = v1, lv2 = v2;

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = (float) samples[i];
            const float out = c0 * in + lv1;
            samples[i] = (int) out;
            
            lv1 = c1 * in - c3 * out + lv2;
            lv2 = c2 * in - c4 * out;
        }

        JUCE_SNAP_TO_ZERO (lv1);  v1 = lv1;
        JUCE_SNAP_TO_ZERO (lv2);  v2 = lv2;
    }
}

IIRCoefficients BiquadFilter::makeLowPass (const double sampleRate,
                                           const double frequency,
                                           const double Q) noexcept
{
	const double oneOverCurrentSampleRate = 1.0 / sampleRate;
	float w0 = (float) (2.0f * float_Pi * frequency * oneOverCurrentSampleRate);
	float cos_w0 = cos (w0);
	float sin_w0 = sin (w0);
	float alpha = sin_w0 / (2.0f * (float) Q);
    
	return IIRCoefficients ((1.0f - cos_w0) * 0.5f,
                            (1.0f - cos_w0),
                            (1.0f - cos_w0) * 0.5f,
                            (1.0f + alpha),
                            -2.0f * cos_w0,
                            (1.0f - alpha));
}

IIRCoefficients BiquadFilter::makeHighPass (const double sampleRate,
                                            const double frequency,
                                            const double Q) noexcept
{
	const double oneOverCurrentSampleRate = 1.0 / sampleRate;
	float w0 = (float) (2.0f * float_Pi * frequency * oneOverCurrentSampleRate);
	float cos_w0 = cos (w0);
	float sin_w0 = sin (w0);
	float alpha = sin_w0 / (2.0f * (float) Q);
    
	return IIRCoefficients ((1.0f + cos_w0) * 0.5f,
                            -(1.0f + cos_w0),
                            (1.0f + cos_w0) * 0.5f,
                            (1.0f + alpha),
                            -2.0f * cos_w0,
                            (1.0f - alpha));
}

IIRCoefficients BiquadFilter::makeBandPass (const double sampleRate,
                                            const double frequency,
                                            const double Q) noexcept
{
	const double qFactor = jlimit (0.00001, 1000.0, Q);
	const double oneOverCurrentSampleRate = 1.0 / sampleRate;
	
	float w0 = (float) (2.0f * float_Pi * frequency * oneOverCurrentSampleRate);
	float cos_w0 = cos (w0);
	float sin_w0 = sin (w0);
	float alpha = sin_w0 / (2.0f * (float) qFactor);
    //	float alpha = sin_w0 * sinh( (log(2.0)/2.0) * bandwidth * w0/sin_w0 );
	
	return IIRCoefficients (alpha,
                            0.0f,
                            -alpha,
                            1.0f + alpha,
                            -2.0f * cos_w0,
                            1.0f - alpha);
}

IIRCoefficients BiquadFilter::makeBandStop (const double sampleRate,
                                            const double frequency,
                                            const double Q) noexcept
{
	const double qFactor = jlimit(0.00001, 1000.0, Q);
	const double oneOverCurrentSampleRate = 1.0 / sampleRate;
	
	
	float w0 = (float) (2.0f * float_Pi * frequency * oneOverCurrentSampleRate);
	float cos_w0 = cos (w0);
	float sin_w0 = sin (w0);
	float alpha = (float) (sin_w0 / (2 * qFactor));
	
	return IIRCoefficients (1.0f,
                            -2*cos_w0,
                            1.0f,
                            1.0f + alpha,
                            -2.0f * cos_w0,
                            1.0f - alpha);
}

IIRCoefficients BiquadFilter::makePeakNotch (const double sampleRate,
                                             const double centreFrequency,
                                             const double Q,
                                             const float gainFactor) noexcept
{
    jassert (sampleRate > 0);
    jassert (Q > 0);
	
    const double A = jmax (0.0f, gainFactor);
    const double omega = (double_Pi * 2.0 * jmax (centreFrequency, 2.0)) / sampleRate;
    const double alpha = 0.5 * sin (omega) / Q;
    const double c2 = -2.0 * cos (omega);
    const double alphaTimesA = alpha * A;
    const double alphaOverA = alpha / A;
	
    return IIRCoefficients (1.0 + alphaTimesA,
                            c2,
                            1.0 - alphaTimesA,
                            1.0 + alphaOverA,
                            c2,
                            1.0 - alphaOverA);
}

IIRCoefficients BiquadFilter::makeAllpass (const double sampleRate,
                                           const double frequency,
                                           const double Q) noexcept
{
	const double qFactor = jlimit(0.00001, 1000.0, Q);
	const double oneOverCurrentSampleRate = 1.0 / sampleRate;
	
	float w0 = (float) (2.0f * float_Pi * frequency * oneOverCurrentSampleRate);
	float cos_w0 = cos(w0);
	float sin_w0 = sin(w0);
	float alpha = (float) (sin_w0 / (2 * qFactor));
	
	return IIRCoefficients (1.0f - alpha,
                            -2 * cos_w0,
                            1.0f + alpha,
                            1.0f + alpha,
                            -2.0f * cos_w0,
                            1.0f - alpha);
}

void BiquadFilter::copyOutputsFrom (const BiquadFilter& other) noexcept
{
	v1 = other.v1;
	v2 = other.v2;
}

#undef JUCE_SNAP_TO_ZERO
