/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2007 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  Paul Nasca
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#include "jucetice_AnalogFilter.h"


AnalogFilter::AnalogFilter (uint8 Ftype, float Ffreq, float Fq, uint8 Fstages)
    : samplerate (44100),
      blocksize (512)
{
    ismp = 0;
    firsttime = 1;

    type = Ftype;
    freq = Ffreq;
    q = Fq;
    gain = 1.0;
    stages = jmin (Fstages, (uint8) MAX_ANALOG_FILTER_STAGES);
}

AnalogFilter::~AnalogFilter()
{
    if (ismp) delete[] ismp;
}

void AnalogFilter::cleanup()
{
    for (int i = 0; i < MAX_ANALOG_FILTER_STAGES + 1; i++)
    {
        x[i].c1 = 0.0;
        x[i].c2 = 0.0;
        y[i].c1 = 0.0;
        y[i].c2 = 0.0;
        oldx[i] = x[i];
        oldy[i] = y[i];
    }
    needsinterpolation = 0;
}

void AnalogFilter::prepareToPlay (float sampleRate, int samplesPerBlock)
{
    samplerate = (int) sampleRate;
    blocksize = samplesPerBlock;

    if (ismp) delete[] ismp;
    ismp = new float [blocksize * 2];

    for (int i = 0; i < 3; i++)
    {
        oldc[i] = 0.0;
        oldd[i] = 0.0;
        c[i] = 0.0;
        d[i] = 0.0;
    }

    cleanup();

    firsttime = 0;
    abovenq = 0;
    oldabovenq = 0;
    
    setFreqAndQ (freq, q);
    
    firsttime = 1;
    d[0] = 0; // this is not used
    
    computeFilterCoefs();
}

void AnalogFilter::releaseResources()
{
}

void AnalogFilter::computeFilterCoefs()
{
    float tmp;
    float omega, sn, cs, alpha, beta;
    int zerocoefs = 0; // this is used if the freq is too high

    // do not allow frequencies bigger than samplerate/2
    float freq = this->freq;
    if (freq > (samplerate / 2 - 500.0))
    {
        freq = samplerate / 2 - 500.0;
        zerocoefs = 1;
    }

    if (freq < 0.1) freq = 0.1;

    // do not allow bogus Q
    if (q < 0.0) q = 0.0;

    float tmpq, tmpgain;
    if (stages == 0)
    {
        tmpq = q;
        tmpgain = gain;
    }
    else
    {
        tmpq = (q > 1.0 ? pow (q, 1.0f / (stages + 1)) : q);
        tmpgain = pow (gain, 1.0f / (stages + 1));
    }

    // most of theese are implementations of
    // the "Cookbook formulae for audio EQ" by Robert Bristow-Johnson
    switch (type)
    {
    case 0: // LPF 1 pole
        if (zerocoefs == 0) tmp = exp (-2.0 * double_Pi * freq / samplerate);
        else                tmp = 0.0;
        c[0] = 1.0 - tmp;
        c[1] = 0.0;
        c[2] = 0.0;
        d[1] = tmp;
        d[2] = 0.0;
        order = 1;
        break;
    case 1: // HPF 1 pole
        if (zerocoefs == 0) tmp = exp (-2.0 * double_Pi * freq / samplerate);
        else                tmp = 0.0;
        c[0] = (1.0 + tmp) / 2.0;
        c[1] = -(1.0 + tmp) / 2.0;
        c[2] = 0.0;
        d[1] = tmp;
        d[2] = 0.0;
        order = 1;
        break;
    case 2: // LPF 2 poles
        if (zerocoefs == 0)
        {
            omega = 2.0 * double_Pi * freq / samplerate;
            sn = sin (omega);
            cs = cos (omega);
            alpha = sn / (2 * tmpq);
            tmp = 1 + alpha;
            c[0] = (1.0 - cs) / 2.0 / tmp;
            c[1] = (1.0 - cs) / tmp;
            c[2] = (1.0 - cs) / 2.0 / tmp;
            d[1] = -2 * cs / tmp * (-1);
            d[2] = (1 - alpha) / tmp * (-1);
        }
        else
        {
            c[0] = 1.0;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    case 3: // HPF 2 poles
        if (zerocoefs == 0)
        {
            omega = 2.0 * double_Pi * freq / samplerate;
            sn = sin (omega);
            cs = cos (omega);
            alpha = sn / (2 * tmpq);
            tmp = 1 + alpha;
            c[0] = (1.0 + cs) / 2.0 / tmp;
            c[1] = -(1.0 + cs) / tmp;
            c[2] = (1.0 + cs) / 2.0 / tmp;
            d[1] = -2 * cs / tmp * (-1);
            d[2] = (1 - alpha) / tmp * (-1);
        }
        else
        {
            c[0] = 0.0;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    case 4: // BPF 2 poles
        if (zerocoefs == 0)
        {
            omega = 2 * double_Pi * freq / samplerate;
            sn = sin(omega);
            cs = cos(omega);
            alpha = sn / (2 * tmpq);
            tmp = 1 + alpha;
            c[0] = alpha / tmp * sqrt (tmpq + 1);
            c[1] = 0;
            c[2] = -alpha / tmp * sqrt (tmpq + 1);
            d[1] = -2 * cs / tmp * (-1);
            d[2] = (1 - alpha) / tmp * (-1);
        }
        else
        {
            c[0] = 0.0;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    case 5: // NOTCH 2 poles
        if (zerocoefs == 0)
        {
            omega = 2.0 * double_Pi * freq / samplerate;
            sn = sin(omega);
            cs = cos(omega);
            alpha = sn / (2 * sqrt (tmpq));
            tmp = 1 + alpha;
            c[0] = 1 / tmp;
            c[1] = -2 * cs / tmp;
            c[2] = 1 / tmp;
            d[1] = -2 * cs / tmp * (-1);
            d[2] = (1 - alpha) / tmp * (-1);
        }
        else
        {
            c[0] = 1.0;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    case 6: // PEAK (2 poles)
        if (zerocoefs == 0)
        {
            omega = 2.0 * double_Pi * freq / samplerate;
            sn = sin(omega);
            cs = cos(omega);
            tmpq *= 3.0;
            alpha = sn / (2 * tmpq);
            tmp = 1 + alpha / tmpgain;
            c[0] = (1.0 + alpha * tmpgain) / tmp;
            c[1] = (-2.0 * cs) / tmp;
            c[2] = (1.0 - alpha * tmpgain) / tmp;
            d[1] = -2 * cs / tmp * (-1);
            d[2] = (1 - alpha / tmpgain) / tmp * (-1);
        }
        else
        {
            c[0] = 1.0;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    case 7: // Low Shelf - 2 poles
        if (zerocoefs == 0)
        {
            omega = 2.0 * double_Pi * freq / samplerate;
            sn = sin (omega);
            cs = cos (omega);
            tmpq = sqrt (tmpq);
            alpha = sn / (2 * tmpq);
            beta = sqrt (tmpgain) / tmpq;
            tmp = (tmpgain + 1.0) + (tmpgain - 1.0) * cs + beta * sn;
            c[0] = tmpgain * ((tmpgain + 1.0) - (tmpgain - 1.0) * cs + beta * sn) / tmp;
            c[1] = 2.0 * tmpgain * ((tmpgain - 1.0) - (tmpgain + 1.0) * cs) / tmp;
            c[2] = tmpgain * ((tmpgain + 1.0) - (tmpgain - 1.0) * cs - beta * sn) / tmp;
            d[1] = -2.0 * ((tmpgain - 1.0) + (tmpgain + 1.0) * cs) / tmp * (-1);
            d[2] = ((tmpgain + 1.0) + (tmpgain - 1.0) * cs - beta * sn) / tmp * (-1);
        }
        else
        {
            c[0] = tmpgain;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    case 8://High Shelf - 2 poles
        if (zerocoefs==0)
        {
            omega = 2.0 * double_Pi * freq / samplerate;
            sn = sin(omega);
            cs = cos(omega);
            tmpq = sqrt(tmpq);
            alpha = sn / (2 * tmpq);
            beta = sqrt (tmpgain) / tmpq;
            tmp = (tmpgain + 1.0) - (tmpgain - 1.0) * cs + beta * sn;
            c[0] = tmpgain * ((tmpgain + 1.0) + (tmpgain - 1.0) * cs + beta * sn) / tmp;
            c[1] = -2.0 * tmpgain * ((tmpgain - 1.0) + (tmpgain + 1.0) * cs) / tmp;
            c[2] = tmpgain * ((tmpgain + 1.0) + (tmpgain - 1.0) * cs - beta * sn) / tmp;
            d[1] = 2.0 * ((tmpgain - 1.0) - (tmpgain + 1.0) * cs) / tmp * (-1);
            d[2] = ((tmpgain + 1.0) - (tmpgain - 1.0) * cs - beta * sn) / tmp * (-1);
        }
        else
        {
            c[0] = 1.0;
            c[1] = 0.0;
            c[2] = 0.0;
            d[1] = 0.0;
            d[2] = 0.0;
        }
        order = 2;
        break;
    default: // wrong type
        type = 0;
        computeFilterCoefs();
        break;
    }
}

void AnalogFilter::setFreq (float frequency)
{
    if (frequency < 0.1) frequency = 0.1;

    float rap = freq / frequency;
    if (rap < 1.0) rap = 1.0 / rap;

    oldabovenq = abovenq;
    abovenq = frequency > (samplerate / 2 - 500.0);

    int nyquistthresh = (abovenq ^ oldabovenq);

    if ((rap > 3.0) || (nyquistthresh != 0))
    {
        // if the frequency is changed fast, it needs interpolation
        // (now, filter and coeficients backup)
        for (int i = 0; i < 3; i++)
        {
            oldc[i] = c[i];
            oldd[i] = d[i];
        }

        for (int i = 0; i < MAX_ANALOG_FILTER_STAGES + 1; i++)
        {
            oldx[i] = x[i];
            oldy[i] = y[i];
        }

        if (firsttime == 0)
            needsinterpolation = 1;
    }

    freq = frequency;
    computeFilterCoefs();
    firsttime = 0;
    
#if 0
    DBG ("AnalogFilter: " + String (freq));
#endif
}

void AnalogFilter::setFreqAndQ (float frequency, float q_)
{
    q = q_;
    setFreq (frequency);
}

void AnalogFilter::setQ (float q_)
{
    q = q_;
    computeFilterCoefs();
}

void AnalogFilter::setType (int type_)
{
    type = type_;
    computeFilterCoefs();
}

void AnalogFilter::setGain (float dBgain)
{
    gain = exp (dBgain * double_Log10 / 20.0);
    computeFilterCoefs();
}

void AnalogFilter::setStages (int stages_)
{
    stages = jmin (stages_, MAX_ANALOG_FILTER_STAGES);

    cleanup();
    computeFilterCoefs();
}

void AnalogFilter::singleFilterOut (float *smp,
                                    AnalogFilterStage &x,
                                    AnalogFilterStage &y,
                                    float *c,
                                    float *d,
                                    int numSamples)
{
    float y0;

    if (order == 1)
    {
        // First order filter
        for (int i = 0; i < numSamples; i++)
        {
            y0 = smp[i] * c[0] + x.c1 * c[1] + y.c1 * d[1];
            y.c1 = y0;
            x.c1 = smp[i];
            // output
            smp[i] = y0;
        }
    }
    else if (order == 2)
    {
        // Second order filter
        for (int i = 0; i < numSamples; i++)
        {
            y0 = smp[i] * c[0] + x.c1 * c[1] + x.c2 * c[2] + y.c1 * d[1] + y.c2 * d[2];
            y.c2 = y.c1;
            y.c1 = y0;
            x.c2 = x.c1;
            x.c1 = smp[i];
            // output
            smp[i] = y0;
        }
    }
}

void AnalogFilter::filterOut (float *smp, int numSamples)
{
    if (needsinterpolation != 0)
    {
        for (int i = 0; i < numSamples; i++)
            ismp[i] = smp[i];
        for (int i = 0; i < stages + 1; i++)
            singleFilterOut (ismp, oldx[i], oldy[i], oldc, oldd, numSamples);
    }

    for (int i = 0; i < stages + 1; i++)
        singleFilterOut (smp, x[i], y[i], c, d, numSamples);

    if (needsinterpolation != 0)
    {
        float samplesInv = 1.0f / (float) numSamples;
        
        for (int i = 0; i < numSamples; i++)
        {
            float x = i * samplesInv;
            smp[i] = ismp[i] * (1.0 - x) + smp[i] * x;
        }

        needsinterpolation = 0;
    }
}

float AnalogFilter::H (float freq)
{
    float fr = freq / samplerate * double_Pi * 2.0;

    float x = c[0], y = 0.0;
    for (int n = 1; n < 3; n++)
    {
        x += cos (n * fr) * c[n];
        y -= sin (n * fr) * c[n];
    }

    float h = x * x + y * y;
    x = 1.0;
    y = 0.0;
    for (int n = 1; n < 3; n++)
    {
        x -= cos (n * fr) * d[n];
        y += sin (n * fr) * d[n];
    }
    
    h = h / (x * x + y * y);

    return pow (h, (stages + 1.0f) / 2.0f);
}

