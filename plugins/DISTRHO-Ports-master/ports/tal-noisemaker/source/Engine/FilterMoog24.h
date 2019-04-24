/**
"MoogFF" - Moog VCF digital implementation.
As described in the paper entitled
"Preserving the Digital Structure of the Moog VCF"
by Federico Fontana
appeared in the Proc. ICMC07, Copenhagen, 25-31 August 2007

Original Java code created by F. Fontana - August 2007
federico.fontana@univr.it

Ported to C++ for SuperCollider by Dan Stowell - August 2007
Ported to C++ for TAL by Patrick Kunz - November 2011
*/

#ifndef __FilterMoog24_h_
#define __FilterMoog24_h_

#include "OscNoise.h"

class FilterMoog24 
{
public:
private:
    float m_b0, m_a1; // Resonant freq and corresponding vals; stored because we need to compare against prev vals
    double m_wcD;

    double m_T; // 1/SAMPLEFREQ
    float m_s1, m_s2, m_s3, m_s4; // 1st order filter states

    float sampleRate;
    float sampleRateFactor;

    OscNoise *oscNoise;

public:
    FilterMoog24(float sampleRate)
    {
        this->sampleRate = sampleRate;

        // initialize the unit generator state variables.
        m_T = 1.0 / sampleRate;
        m_s1 = 0.0f;
        m_s2 = 0.0f;
        m_s3 = 0.0f;
        m_s4 = 0.0f;

		sampleRateFactor= 44100.0f/sampleRate;
		if (sampleRateFactor > 1.0f) 
		{
			sampleRateFactor= 1.0f;
		}

		oscNoise = new OscNoise(sampleRate);
    }

    void Process(float *input, float cutoffIn, float resonance, bool isHighPass, bool calcCeff)
    {
        bool hpMode = isHighPass;
        
        *input *= 1.0f + resonance * 2.0f;

        float inputWithoutRef = *input;
        float output = *input;
        float k = resonance * 4.0f; // 4 is distortion free

        if (cutoffIn <= 0.25)
        {
            float drive = (0.25f - cutoffIn) * 4.0f;
            k += drive * drive;
        }

        // Load state from the struct
        float s1 = m_s1;
        float s2 = m_s2;
        float s3 = m_s3;
        float s4 = m_s4;
        float freq =  sampleRateFactor * cutoffIn * this->sampleRate * 0.5f * (oscNoise->getNextSamplePositive() * 0.001f + 0.999f); ///
        double T = m_T;

        double wcD = m_wcD;
        float a1 = m_a1;
        float b0 = m_b0; // Filter coefficient parameters
        float o, u; // System's null response, loop input
        float past, future;

        // Update filter coefficients, but only if freq changes since it involves some expensive operations
        if (calcCeff)
        {
            //Print("Updated freq to %g\n", freq);
            wcD = 2.0 * tan(T * 3.141592653589793238462643383279502884197 * freq) * sampleRate;
            if (wcD < 0)
            {
                wcD = 0;
            }
            double TwcD = T * wcD;
            b0 = (float)(TwcD / (TwcD + 2.0f));
            a1 = (float)((TwcD - 2.0f) / (TwcD + 2.0f));
        }

        tanhClipper(inputWithoutRef * 2.0f);
        inputWithoutRef *= 0.5f;

        // compute loop values
        o = s4 + b0 * (s3 + b0 * (s2 + b0 * s1));
        output = (b0 * b0 * b0 * b0 * inputWithoutRef + o) / (1.0f + b0 * b0 * b0 * b0 * k);

        if (hpMode)
            u = inputWithoutRef + k * (*input - output);
        else
            u = inputWithoutRef - k * output;

        // update 1st order filter states
        past = u;
        future = b0 * past + s1;
        s1 = b0 * past - a1 * future;

        past = future;
        future = b0 * past + s2;
        s2 = b0 * past - a1 * future;

        past = future;
        future = b0 * past + s3;
        s3 = b0 * past - a1 * future;

        s4 = b0 * future - a1 * output;

        // Store state
        m_b0 = b0;
        m_a1 = a1;
        m_wcD = wcD;
        m_s1 = tanhClipper(s1);
        m_s2 = s2;
        m_s3 = tanhClipper(s3);
        m_s4 = s4;

        if (hpMode)
            *input = output - *input;
        else
            *input = output;
    }

private:
    float tanhClipper(float x)
    {
        if (x > 0.0f)
            x *= 0.99999f;
        else
            x *= 1.00001f;

        x *= 2.0f;
        float a = fabs(x);
        float b = 6.0f + a * (3.0f + a);
        return (x * b) / (a * b + 12.0f);
    }
};
#endif