#ifndef BUFFERS_H_INCLUDED
#define BUFFERS_H_INCLUDED

#include "JuceHeader.h"

template <class DataType>
class CircularBuffer
{
public:
    CircularBuffer (int initSize)
        : size (0), index (0)
    {
        setSize(initSize);
        clear();
    }

    bool setSize (int newSize)
    {
        jassert(newSize > 0);

        if (size != newSize && newSize > 0)
        {
            size = newSize;
            data.realloc(size);
            clear();

            return true;
        }

        return false;
    }

    int getSize() const
    {
        return size;
    }

    void clear()
    {
        juce::zeromem(data, sizeof(DataType) * size);
        index = 0;
    }

    void push (DataType x)
    {
        data[index] = x;

        if (++index >= size)
            index = 0;
    }

    DataType pushAndGet (DataType x)
    {
        DataType ret = data[index];
        push(x);

        return ret;
    }

    DataType operator[] (int offset) const
    {
        int idx = index - offset - 1;

        if (idx < 0)
            idx += size;

        jassert(idx >= 0 && idx < size);
        return data[idx];
    }

    void processBlock (DataType* proc, int numSamples)
    {
        for (int i = 0; i<numSamples; ++i)
            proc[i] = pushAndGet(proc[i]);
    }

private:

    int size;
    juce::HeapBlock<DataType> data;
    int index;
};

class RmsBuffer
{
public:
    RmsBuffer (int initSize);

    void clear();

    double process (double in);

    void processBlock (const double* in, int numSamples);

    void setSize (int newSize);

    int getSize();

    double getRms();


private:

    double rms;
    CircularBuffer<double> buffer;
};


class RmsLevel
{
public:
    RmsLevel (double lengthMs);
    void setSampleRate (double sampleRate);

    void clear();
    void process (float in);
    void process (float inL, float inR);
    void processBlock (const float* inL, const float* inR, int numSamples);
    float getRms();
    double getRmsLength();


private:

    const double ms;
    int size;
    HeapBlock<double> data;
    double rms;

    int index;

    JUCE_DECLARE_NON_COPYABLE(RmsLevel)
};


#endif  // BUFFERS_H_INCLUDED
