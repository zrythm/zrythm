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

#ifndef __DROWAUDIO_SAMPLERATECONVERTER_H__
#define __DROWAUDIO_SAMPLERATECONVERTER_H__

//==============================================================================
/**
    Simple sample rate converter class.
 
    This converts a block of samples from one sample rate to another. It is
    based on a linear interpolation algorithm.
    To use it simply create one with the desired number of channels and then
    repeatedly call its process() method. The sample ratio is based on the
    difference in input and output buffer sizes so for example to convert a
    44.1KHz signal to a 22.05KHz one you could pass in buffers with sizes 512
    and 256 respectively.
 */
class SampleRateConverter
{
public:
    //==============================================================================
    /** Creates a SampleRateConverter with a given number of channels.
     */
    SampleRateConverter (const int numChannels = 1);

    /** Destructor.
     */
    ~SampleRateConverter();
    
    /** Performs the conversion.
        The minimum number of channels will be processed here so it is a good idea
        to make sure that the number of input channels is equal to the number of
        output channels. The input channel data is filtered during this process so
        if you don't want to lose it then make a copy before calling this method.
     */
    void process (float** inputChannelData, int numInputChannels, int numInputSamples,
                  float** outputChannelData, int numOutputChannels, int numOutputSamples);

private:
    //==============================================================================
    double ratio;
    double coefficients[6];
    const int numChannels;
    
    void setFilterCoefficients (double c1, double c2, double c3, double c4, double c5, double c6);
    void createLowPass (double proportionalRate);
    
    struct FilterState
    {
        double x1, x2, y1, y2;
    };
    
    HeapBlock<FilterState> filterStates;
    void resetFilters();
    
    void applyFilter (float* samples, int num, FilterState& fs);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleRateConverter);
};


#endif  // __SAMPLERATECONVERTER_H_65500721__
