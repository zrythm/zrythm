#ifndef DSPSTUFF_H_INCLUDED
#define DSPSTUFF_H_INCLUDED

#include "JuceHeader.h"

namespace BiquadType
{
enum BiquadType
{
    kBypass,
    kHighPass6,
    kLowPass,
    kBandPass,

    kNumFilters
};
}

struct BiquadCoefficients
{
    BiquadCoefficients();

    void create (BiquadType::BiquadType type, double freq, double Q, double sampleRate);

    double b0;
    double b1;
    double b2;
    double a1;
    double a2;
};

class StaticBiquad
{
public:

    StaticBiquad();

    void clear();

    void setSampleRate (double newSampleRate);

    void setFilter (BiquadType::BiquadType type, double freq, double Q);

    float process (float in);

    void processBlock (float* in, int numSamples);

    void processBlock (float* inL, float* inR, int numSamples);

    float getLast (bool left);

private:

    double sampleRate;

    BiquadType::BiquadType curType;
    double curFreq;
    double curQ;

    BiquadCoefficients coeff;

    double xl[2];
    double yl[2];
    double xr[2];
    double yr[2];
};

class SimpleNoiseGen
{
public:

    SimpleNoiseGen (float levelDb);
    void processBlock (float* chL, float* chR, int numSamples);
    void processBlock (float* ch, int numSamples);

private:
    float level;
    juce::Random rnd;
};


#endif  // DSPSTUFF_H_INCLUDED
