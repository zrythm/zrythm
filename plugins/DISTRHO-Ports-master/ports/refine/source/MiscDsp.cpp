#include "MiscDsp.h"

BiquadCoefficients::BiquadCoefficients() : b0 (1), b1 (0), b2 (0), a1 (0), a2 (0) {}

void BiquadCoefficients::create (BiquadType::BiquadType type, double freq, double Q, double sampleRate)
{
    const double w0 = 2 * juce::double_Pi * freq / sampleRate;
    const double cosw0 = cos(w0);
    const double alpha = sin(w0) / (2 * Q);

    double a0 = 1;

    switch (type)
    {
    case BiquadType::kBypass:
        b0 = 1; b1 = 0; b2 = 0; a1 = 0; a2 = 0;
        break;
    case BiquadType::kHighPass6:
        a1 = (tan(w0 / 2) - 1) / (tan(w0 / 2) + 1);
        b0 = (1 - a1) * 0.5;
        b1 = (a1 - 1) * 0.5;
        b2 = 0;
        a2 = 0;
        break;
    case BiquadType::kLowPass:
        a0 = 1 + alpha;
        b0 = (1 - cosw0) / 2;
        b1 = 1 - cosw0;
        b2 = (1 - cosw0) / 2;
        a1 = -2 * cosw0;
        a2 = 1 - alpha;
        break;
    case BiquadType::kBandPass:
        a0 = 1 + alpha;
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        a1 = -2 * cosw0;
        a2 = 1 - alpha;
        break;
    default:
        jassertfalse;
        break;
    }

    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
}


StaticBiquad::StaticBiquad()
    : sampleRate (44100)
{
    setFilter(BiquadType::kBypass, 1000, 0.7);
    clear();
}

void StaticBiquad::clear()
{
    xl[0] = 0; xl[1] = 0;
    yl[0] = 0; yl[1] = 0;
    xr[0] = 0; xr[1] = 0;
    yr[0] = 0; yr[1] = 0;
}

void StaticBiquad::setSampleRate (double newSampleRate)
{
    sampleRate = newSampleRate;
    setFilter(curType, curFreq, curQ);
}

void StaticBiquad::setFilter (BiquadType::BiquadType type, double freq, double Q)
{
    curType = type;
    curFreq = freq;
    curQ = Q;

    coeff.create(curType, curFreq, curQ, sampleRate);
}

float StaticBiquad::process (float in)
{
    const double x = (double) in;
    double y = x*coeff.b0 + xl[0] * coeff.b1 + xl[1] * coeff.b2 - yl[0] * coeff.a1 - yl[1] * coeff.a2;

    if (y > -1e-8 && y < 1e-8)
        y = 0;

    xl[1] = xl[0]; xl[0] = x;
    yl[1] = yl[0]; yl[0] = y;

    return (float) y;
}

void StaticBiquad::processBlock (float* in, int numSamples)
{
    for (int i = 0; i<numSamples; ++i)
        in[i] = process(in[i]);
}

void StaticBiquad::processBlock (float* inL, float* inR, int numSamples)
{
    for (int i = 0; i<numSamples; ++i)
    {
        const double xL = (double) inL[i];
        const double xR = (double) inR[i];
        double yL = xL*coeff.b0 + xl[0] * coeff.b1 + xl[1] * coeff.b2 - yl[0] * coeff.a1 - yl[1] * coeff.a2;
        double yR = xR*coeff.b0 + xr[0] * coeff.b1 + xr[1] * coeff.b2 - yr[0] * coeff.a1 - yr[1] * coeff.a2;

        if (yL > -1e-8 && yL < 1e-8)
            yL = 0;

        if (yR > -1e-8 && yR < 1e-8)
            yR = 0;

        xl[1] = xl[0]; xl[0] = xL;
        xr[1] = xr[0]; xr[0] = xR;
        yl[1] = yl[0]; yl[0] = yL;
        yr[1] = yr[0]; yr[0] = yR;

        inL[i] = (float) yL;
        inR[i] = (float) yR;
    }
}

float StaticBiquad::getLast (bool left)
{
    return float(left ? yl[0] : yr[0]);
}

SimpleNoiseGen::SimpleNoiseGen(float levelDb)
    : level (pow(10.f, 0.05f * levelDb))
{
}

void SimpleNoiseGen::processBlock (float* chL, float* chR, int numSamples)
{
    for (int i = 0; i<numSamples; ++i)
    {
        const float noise = (rnd.nextFloat() - 0.5f) * 2.f * level;
        chL[i] += noise;
        chR[i] += noise;
    }
}

void SimpleNoiseGen::processBlock (float* ch, int numSamples)
{
    for (int i = 0; i<numSamples; ++i)
    {
        const float noise = (rnd.nextFloat() - 0.5f) * 2.f * level;
        ch[i] += noise;
    }
}
