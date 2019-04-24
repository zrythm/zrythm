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

#ifndef __DROWAUDIO_MATHSUTILITIES_H__
#define __DROWAUDIO_MATHSUTILITIES_H__

#if JUCE_MSVC
    #pragma warning (disable: 4505)
#endif

//==============================================================================
/** Contains a value and its reciprocal.
    This has some handy operator overloads to speed up multiplication and divisions.
 */
template <typename FloatingPointType>
class Reciprocal
{
public:
    /** Creates a 1/1 value/reciprocal pair. */
    Reciprocal()                                        { set (1.0); }
    
    /** Creates a 1/x value/reciprocal pair. */
    Reciprocal (FloatingPointType initalValue)          { set (initalValue); }

    /** Returns the value. */
    FloatingPointType get() const noexcept              { return value; }

    /** Returns the value. */
    FloatingPointType getValue() const noexcept         { return value; }
    
    /** Returns the reciprocal. */
    FloatingPointType getReciprocal() const noexcept    { return reciprocal; }

    /** Sets the value updating the reciprocal. */
    FloatingPointType set (FloatingPointType newValue) noexcept
    {
        jassert (newValue != 0);
        value = newValue;
        reciprocal = (FloatingPointType) (1.0 / value);
        
        return value;
    }

    /** Sets the value updating the reciprocal. */
    FloatingPointType operator= (FloatingPointType newValue) noexcept           { return set (newValue); }

    FloatingPointType operator+ (FloatingPointType operand) const noexcept      { return value + operand; }
    FloatingPointType operator+= (FloatingPointType operand) noexcept           { return set (value + operand); }

    FloatingPointType operator- (FloatingPointType operand) const noexcept      { return value - operand; }
    FloatingPointType operator-= (FloatingPointType operand) noexcept           { return set (value - operand); }

    FloatingPointType operator* (FloatingPointType operand) const noexcept      { return value * operand; }
    FloatingPointType operator/ (FloatingPointType operand) const noexcept      { return operand * reciprocal * value; }
    FloatingPointType operator*= (FloatingPointType operand) noexcept           { return set (value *= operand); }
    FloatingPointType operator/= (FloatingPointType operand) noexcept           { return set (operand * reciprocal * value); }

private:
    FloatingPointType value, reciprocal;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reciprocal)
};

//==============================================================================
/** Returns true if the given integer number is even. */
inline bool isEven (int number) noexcept
{
    return ! (number & 0x1);
}

/** Returns true if the given integer number is odd. */
inline bool isOdd (int number) noexcept
{
    return number & 0x1;
}

//==============================================================================
/** Finds the maximum value and location of this in a buffer regardless of sign. */
template <typename FloatingPointType>
inline void findAbsoluteMax (const FloatingPointType* samples, int numSamples,
                             int& maxSampleLocation, FloatingPointType& maxSampleValue) noexcept
{
    maxSampleValue = 0;
    
    for (int i = 0; i < numSamples; ++i)
    {
        const FloatingPointType absoluteSample = fabs (samples[i]);
        
        if (absoluteSample > maxSampleValue)
        {
            maxSampleValue = absoluteSample;
            maxSampleLocation = i;
        }
    }
}

/** Normalises a set of samples to the absolute maximum contained within the buffer. */
template <typename FloatingPointType>
inline void normalise (FloatingPointType* samples, int numSamples) noexcept
{
    FloatingPointType max = 0;
    int location;
    
    findAbsoluteMax (samples, numSamples, location, max);
    
    if (max != 0)
    {
        const FloatingPointType oneOverMax = 1 / max;
        
        for (int i = 0; i < numSamples; ++i)
            samples[i] *= oneOverMax;
    }
    else
    {
        zeromem (samples, numSamples * sizeof (float));
    }
}

/** Squares all the values in an array. */
template <typename FloatingPointType>
inline void square (FloatingPointType* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        samples[i] = juce::square<FloatingPointType> (samples[i]);
}

//==============================================================================
/** Finds the autocorrelation of a set of given samples.
 
    This will cross-correlate inputSamples with itself and put the result in
    output samples. Note that this uses a shrinking integration window, assuming
    values outside of numSamples are 0. This leads to an exponetially decreasing
    autocorrelation function.
 */
template <typename FloatingPointType>
inline void autocorrelate (const FloatingPointType* inputSamples, int numSamples, FloatingPointType* outputSamples) noexcept
{
    for (int i = 0; i < numSamples; i++)
    {
        FloatingPointType sum = 0;
        
        for (int j = 0; j < numSamples - i; j++)
            sum += inputSamples[j] * inputSamples[j + i];
        
        outputSamples[i] = sum * (static_cast<FloatingPointType> (1) / numSamples);
    }
}

/** Finds the autocorrelation of a set of given samples using a
    square-difference function.
 
    This will cross-correlate inputSamples with itself and put the result in
    output samples. Note that this uses a shrinking integration window, assuming
    values outside of numSamples are 0. This leads to an exponetially decreasing
    autocorrelation function.
 */
template <typename FloatingPointType>
inline void sdfAutocorrelate (const FloatingPointType* inputSamples, int numSamples, FloatingPointType* outputSamples) noexcept
{
    for (int i = 0; i < numSamples; i++)
    {
        FloatingPointType sum = 0;
        
        for (int j = 0; j < numSamples - i; j++)
            sum += squareNumber (inputSamples[j] - inputSamples[j + i]);
            
        outputSamples[i] = sum;
    }
}

/** Performs a first order differential on the set of given samples.
 
    This is the same as finding the difference between each sample and the previous.
    Note that outputSamples can point to the same location as inputSamples.
 */
template <typename FloatingPointType>
inline void differentiate (const FloatingPointType* inputSamples, int numSamples, FloatingPointType* outputSamples) noexcept
{
    FloatingPointType lastSample = 0.0;
    
    for (int i = 0; i < numSamples; ++i)
    {
        FloatingPointType currentSample = inputSamples[i];
        outputSamples[i] = currentSample - lastSample;
        lastSample = currentSample;
    }
}

/** Finds the mean of a set of samples. */
template <typename FloatingPointType>
inline FloatingPointType findMean (const FloatingPointType* samples, int numSamples) noexcept
{
    FloatingPointType total = 0;
    
    for (int i = 0; i < numSamples; ++i)
        total += samples[i];

    return total / numSamples;
}

/** Returns the median of a set of samples assuming they are sorted. */
template <typename FloatingPointType>
inline FloatingPointType findMedian (const FloatingPointType* samples, int numSamples) noexcept
{
    if (isEven (numSamples % 2))  // is even
    {
        return samples[numSamples / 2];
    }
    else
    {
        const int lowerIndex = int (numSamples / 2);
        const FloatingPointType lowerSample = samples[lowerIndex];
        const FloatingPointType upperSample = samples[lowerIndex + 1];
        
        return (lowerSample + upperSample) / 2;
    }
}

/** Finds the variance of a set of samples. */
template <typename FloatingPointType>
inline FloatingPointType findVariance (const FloatingPointType* samples, int numSamples) noexcept
{
    const FloatingPointType mean = findMean (samples, numSamples);

    FloatingPointType total = 0;
    for (int i = 0; i < numSamples; ++i)
        total += squareNumber (samples[i] - mean);

    return total / numSamples;
}

/** Finds the corrected variance for a set of samples suitable for a sample standard deviation.
    Note the N - 1 in the formula to correct for small data sets.
 */
template <typename FloatingPointType>
inline FloatingPointType findCorrectedVariance (const FloatingPointType* samples, int numSamples) noexcept
{
    const FloatingPointType mean = findMean (samples, numSamples);
    
    FloatingPointType total = 0;
    for (int i = 0; i < numSamples; ++i)
        total += squareNumber (samples[i] - mean);
    
    return total / (numSamples - 1);
}

/** Finds the sample standard deviation for a set of samples. */
template <typename FloatingPointType>
inline FloatingPointType findStandardDeviation (const FloatingPointType* samples, int numSamples) noexcept
{
    return sqrt (findCorrectedVariance (samples, numSamples));
}

/** Finds the RMS for a set of samples. */
template <typename FloatingPointType>
inline FloatingPointType findRMS (const FloatingPointType* samples, int numSamples) noexcept
{
    FloatingPointType sum = 0;
    
    for (int i = 0; i < numSamples; ++i)
        sum += juce::square (*samples++);
    
    return std::sqrt (sum / numSamples);
}

//==============================================================================
/**	Linear Interpolater.
	Performs a linear interpolation for a fractional buffer position.
	Note: For speed no bounds checking is performed on the buffer position so it is
	up to the caller to make sure it is less than the buffer size or you will be
	reading random memory and probably get an audio blow-up.
 */
template <typename FloatingPointType>
inline FloatingPointType linearInterpolate (const FloatingPointType* buffer, int bufferSize, FloatingPointType bufferPosition) noexcept
{
	int lower = (int) bufferPosition;
	int upper = lower + 1;
	if (upper == bufferSize)
		upper = 0;
	FloatingPointType difference = bufferPosition - lower;

	return (buffer[upper] * difference) + (buffer[lower] * (static_cast<FloatingPointType> (1) - difference));
}

/** Checks to see if two values are equal within a given precision.
 */
template <typename FloatingPointType>
inline bool almostEqual (FloatingPointType firstValue, FloatingPointType secondValue, FloatingPointType precision = 0.00001)
{
	if (fabs (firstValue - secondValue) < precision)
		return true;
	else
		return false;
}

/** Normalises a value to a range of 0-1 with a given minimum & maximum.
    This is just a quick function to make more readable code and desn't do any error checking.
    If your value is outside the range you will get a normalised value < 0 or > 1.
 */
template <typename FloatingPointType>
inline FloatingPointType normalise (const FloatingPointType valueToNormalise, const FloatingPointType minimum, const FloatingPointType maximum) noexcept
{
    return (valueToNormalise - minimum) / (maximum - minimum);
}

/** Scales a value to have a log range between a given minimum & maximum.
    This is just a quick function to make more readable code and desn't do any error checking.
    This is useful when scaling values for meters etc. A good starting point is a normalised
    input value, minimum of 1 and maximum of 40.
 */
template <typename FloatingPointType>
inline FloatingPointType logBase10Scale (const FloatingPointType valueToScale,
                                         const FloatingPointType minimum,
                                         const FloatingPointType maximum) noexcept
{
    return log10 (minimum + ((maximum - minimum) * valueToScale)) / log10 (maximum);
}

/** Converts a frequency in hertz to its Mel (or melody) based scale.
    The Mel scale is devised so that 1KHz represents a human perceived doubling in pitch.
 */
template <typename FloatingPointType>
inline FloatingPointType melScale (const FloatingPointType frequencyInHerts) noexcept
{
    return 2595 * log10 (1 + (frequencyInHerts / 700.0));
}

//==============================================================================
/** Checks to see if a number is NaN eg. sqrt (-1). */
template <typename Type>
inline static bool isnan (Type value)
{
#if JUCE_MAC
    return std::isnan (value);
#else
    volatile Type num = value;
    
    return num != num;
#endif
}

/** Checks to see if a number is Inf eg. 100.0 / 0.0. */
template <typename Type>
inline static bool isinf (Type value)
{
#if ! JUCE_WINDOWS
    return std::isinf (value);
#else
    return ! _finite (value);
#endif
}

//==============================================================================
/**	Sinc function. */
template <typename Type>
inline Type sinc (const Type x) noexcept
{
    if (x == 0)
        return static_cast<Type> (1);
    
    return sin (x) / x;
}

/**	Sinc function normalised with PI for audio applications.
    N.B. For accuracy this needs to use a double precision PI value internally
    so a cast will occur if floats are used.
 */
template<typename FloatingPointType>
inline FloatingPointType sincPi (const FloatingPointType x) noexcept
{
    if (x == 0)
        return static_cast<FloatingPointType> (1);
    
    return static_cast<FloatingPointType> (std::sin (double_Pi * x) / (double_Pi * x));
}

/**	Returns true if the argument is a power of 2.
    This will return false if 0 is passed.
 */
template <typename IntegerType>
inline bool isPowerOfTwo (IntegerType number) noexcept
{
	return (number) && ! (number & (number - 1));
}

/**	Returns the next power of 2 of the given number. */
inline int nextPowerOfTwo (int number) noexcept
{
	if (isPowerOfTwo (number))
		return number;
	else
		return (int) pow (2.0, ceil (log ((double) number) / log (2.0)));
}

/**	Returns the previous power of 2.
    This may return 0 if a number < 1 is passed.
 */
inline int prevPowerOfTwo (int number) noexcept
{
	if (isPowerOfTwo (number))
		return number;
	else
		return (int) (pow (2.0, ceil (log ((double) number) / log (2.0))) * 0.5);
}

/**	Returns the power which 2 has to be raised to to get the given number.
    If the given number is not an exact power of 2 the next nearest power will be given.
    E.g. 1024 will return 10 as will 1023.
 */
inline int findPowerForBaseTwo (int number) noexcept
{
	if (isPowerOfTwo (number))
		return (int) (log ((double) number) / log(2.0));
	else
		return (int) (log ((double) nextPowerOfTwo (number)) / log(2.0));
}

#if JUCE_MSVC || DOXYGEN
/** Log2 function for the MSVC compiler. */
inline double log2 (double number)
{
    return log (number) / log (2.0);
}

/** Log2f function for the MSVC compiler. */
inline float log2f (float number)
{
    return log (number) / log (2.0f);
}
#endif

#endif //__DROWAUDIO_MATHSUTILITIES_H__
