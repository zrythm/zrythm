/*
  ==============================================================================

    SpectrumAnalyserHeader.h
    Created: 10 Jun 2014 8:19:00pm
    Author:  Samuel Gaehwiler

  ==============================================================================
*/

#ifndef SPECTRUM_ANALYSER_HEADER_H_INCLUDED
#define SPECTRUM_ANALYSER_HEADER_H_INCLUDED

//==============================================================================

#include "JuceHeader.h"

//==============================================================================

// Helper Functions
// ----------------

/**
 *  @return a value between 0 and 1.
 */
inline float logTransformInRange0to1 (const float between0and1)
{
    const float minimum = 1.0f;
    const float maximum = JUCE_LIVE_CONSTANT (1000.0f);
    // return drow::logBase10Scale (frequency, 1.0f, maximum);
    return log10 (minimum + ((maximum - minimum) * between0and1)) / log10 (maximum);
}

/**
 *  @return a value between 0 and 1.
 */
inline float expTransformInRange0to1 (const float between0and1)
{
    const float minimum = 1.0f;
    const float maximum = JUCE_LIVE_CONSTANT (1000.0f);
    return (pow(maximum, between0and1) - minimum) / (maximum - minimum);
}


#endif  // SPECTRUM_ANALYSER_HEADER_H_INCLUDED
