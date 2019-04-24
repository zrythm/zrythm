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

#ifndef __DROWAUDIO_PITCHDETECTOR_H__
#define __DROWAUDIO_PITCHDETECTOR_H__

#include "dRowAudio_Buffer.h"
#include "dRowAudio_FifoBuffer.h"

//==============================================================================
/**
    Auto correlation based pitch detector class.
 
    This class will calculate the pitch of a block of samples. Although this can
    be slower than an FFT approach it is much more accurate, especially at low
    frequencies (due to not having to lie on a bin boundry).
 
    The number of samples required to calculate a pitch will vary depending on
    the minimum frequency set. This will also determine the number of calculations
    required so set these to some sensible estimates first. Internally this uses a
    circular buffer to collect samples and will only calculate a pitch when a
    certain number of samples have been gathered. This number can be found with
    getNumSamplesNeededForDetection(). The last detected pitch can be returned
    with the getPitch() method. This does introduce some latency into the detection.
 
    Alternatively you can pass a whole block of samples with the detectPitch
    method and a smart average will be calculated.
 
    @see setSampleRate, setMinMaxFrequency, getPitch, processSamples, detectPitch
 */
class PitchDetector
{
public:
    //==============================================================================
    /** An enum to specify the detection algorithm used.
     */
    enum DetectionMethod
    {
        autoCorrelationFunction,
        squareDifferenceFunction,
    };
    
    //==============================================================================
    /** Creates a PitchDetector.
        Make sure you specify the sample rate and some sensible min/max frequencies
        to get the best out of this class.
        
        @see setSampleRate, setMinMaxFrequency
     */
    PitchDetector();
    
    /** Destructor. */
    ~PitchDetector();
    
    /** Process a block of samples.
        Because the number of samples required to find a pitch varies depending on
        the minimum frequency set this uses an internal buffer to store samples until
        enough have been gathered. This does have the side effect of introducing some
        latency.
     */
    void processSamples (const float* samples, int numSamples) noexcept;

    /** Returns the most recently detected pitch.
     */
    double getPitch() const noexcept                    {   return mostRecentPitch; }

    //==============================================================================
    /** Detects the average pitch in a block of samples.
        This alternative detection method uses some clever averaging techniques to
        compute the average pitch of a block of samples. This can be useful if used
        in conjunction with some other splicing techniques such as onset detection
        for finding the frequency of whole notes.
     
        Note that the sample array passed in is not marked const and will alter the
        samples. Be sure to pass in a copy if this is undesireable.
     */
    double detectPitch (float* samples, int numSamples) noexcept;

    //==============================================================================
    /** Sets the sample rate to base the detection and pitch calculation algorithms on.
        This needs to be set before any detection can be made.
        Note that this isn't thread safe so don't call it concurrently with any calls
        to the process methods.
     */
    void setSampleRate (double newSampleRate) noexcept;
    
    /** Sets the detection algorithm to use.
        By default this is autoCorrelationFunction.
     */
    void setDetectionMethod (DetectionMethod newMethod);
    
    /** Returns the detection method currently in use.
     */
    inline DetectionMethod getDetectionMethod() const noexcept  {   return detectionMethod; }
    
    /** Sets the minimum and maximum frequencies that can be detected.
        Because this uses an auto-correlation algorithm the lower the minimum
        frequency, the more cpu intensive the calculation will be. Therefore it is
        a good idea to set the values to some realistic expectations.
     */
    void setMinMaxFrequency (float newMinFrequency, float newMaxFrequency) noexcept;
    
    /** Returns the minimum frequency that can currently be detected.
     */
    inline float getMinFrequency() const noexcept               {   return minFrequency;    }

    /** Returns the maximum frequency that can currently be detected.
     */
    inline float getMaxFrequency() const noexcept               {   return maxFrequency;    }

    /** Returns the minimum number of samples required to detect a pitch based on
        the minimum frequency set.
     */
    inline int getNumSamplesNeededForDetection() const noexcept
    {
        return numSamplesNeededForDetection;
    }
    
    //==============================================================================
    /** This returns the tempoary Buffer objects used to calculate the pitch.
        This method isn't really intended for public use but could be used for
        debugging or display purposes.
     */
    Buffer* getBuffer (int stageIndex);

private:
    //==============================================================================
    DetectionMethod detectionMethod;
    double sampleRate;
    float minFrequency, maxFrequency;
    Buffer buffer1, buffer2;

    IIRFilter highFilter, lowFilter;
    int numSamplesNeededForDetection;
    Buffer currentBlockBuffer;
    FifoBuffer<float> inputFifoBuffer;
    double mostRecentPitch;

    //==============================================================================
    void updateFiltersAndBlockSizes();

    //==============================================================================
    double detectPitchForBlock (float* samples, int numSamples);
    double detectAcfPitchForBlock (float* samples, int numSamples);
    double detectSdfPitchForBlock (float* samples, int numSamples);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchDetector);
};


#endif  // __DROWAUDIO_PITCHDETECTOR_H__
