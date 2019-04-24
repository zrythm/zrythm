#include "Buffers.h"

RmsBuffer::RmsBuffer (int initSize)
    : buffer (initSize)
{
    clear();
}

void RmsBuffer::clear()
{
    rms = 0;
    buffer.clear();
}

double RmsBuffer::process (double in)
{
    const double x = in * in;
    const double tmp = buffer.pushAndGet(x);
    rms = rms + x - tmp;

    return rms / buffer.getSize();
}

void RmsBuffer::processBlock (const double* in, int numSamples)
{
    for (int i = 0; i<numSamples; ++i)
        process(in[i]);
}

void RmsBuffer::setSize (int newSize)
{
    if (buffer.setSize(newSize))
        rms = 0;
}

int RmsBuffer::getSize()
{
    return buffer.getSize();
}

double RmsBuffer::getRms()
{
    return rms / buffer.getSize();
}

RmsLevel::RmsLevel (double lengthMs)
    : ms (lengthMs), size (0), index (0)
{
    setSampleRate(44100);
}

void RmsLevel::setSampleRate (double sampleRate)
{
    const int newSize = int(sampleRate*ms*0.001);

    if (size != newSize)
    {
        size = newSize;
        data.realloc(size);
    }
    clear();
}

void RmsLevel::clear()
{
    zeromem(data, sizeof(double) * size);
    rms = 0;
    index = 0;
}

void RmsLevel::process (float in)
{
    const double x = in*in;
    rms += x - data[index];

    if (rms < 1e-8)
        rms = 0;

    data[index] = x;

    if (++index >= size)
        index = 0;
}

void RmsLevel::process (float inL, float inR)
{
    const double x = 0.5f * (inL*inL + inR*inR);
    rms += x - data[index];

    if (rms < 1e-8)
        rms = 0;

    data[index] = x;

    if (++index >= size)
        index = 0;
}

void RmsLevel::processBlock (const float* inL, const float* inR, int numSamples)
{
    if (inR == nullptr)
    {
        for (int i = 0; i<numSamples; ++i)
        {
            const double x = inL[i] * inL[i];
            rms += x - data[index];

            if (rms < 1e-8)
                rms = 0;

            data[index] = x;

            if (++index >= size)
                index = 0;
        }
    }
    else
    {
        for (int i = 0; i<numSamples; ++i)
        {
            const double x = 0.5f * (inL[i] * inL[i] + inR[i] * inR[i]);
            rms += x - data[index];

            if (rms < 1e-8)
                rms = 0;

            data[index] = x;

            if (++index >= size)
                index = 0;
        }
    }
}

float RmsLevel::getRms()
{
    return sqrt(float(rms / size));
}

double RmsLevel::getRmsLength()
{
    return ms;
}
