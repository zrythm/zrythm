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

#ifndef __DROWAUDIO_LTAS_H__
#define __DROWAUDIO_LTAS_H__

#if JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL || defined (DOXYGEN)

class CumulativeMovingAverage;

//==============================================================================
/** Calculates the Long Term Average Spectrum of a set of samples.
    
    This is a simple LTAS calculator that uses finds the spectrum of a block of
    audio samples and then computes the average weights across the number of
    FFTs performed.
 */
class LTAS
{
public:
    //==============================================================================
    /** Creates an LTAS.
        
        This will use a given FFT size, remember this is the log2 of the FFT size
        so 11 will be a 2048 point FFT.
     
        @see FFTEngine
     */
    LTAS (int fftSizeLog2);
    
    /** Destructor. */
    ~LTAS();
    
    /** Calculates the LTAS based on a set of samples.
        
        For this to work the number of samples must be at least as many as the size
        of the FFT.
     */
    void updateLTAS (float* input, int numSamples);
    
    /** Returns the computed LTAS buffer.
     
        This can be used to find pitch, tone information etc.
        
        @see Buffer
     */
    Buffer& getLTASBuffer()                    {   return ltasBuffer;  }
        
private:
    //==============================================================================
    FFTEngine fftEngine;
    Buffer ltasBuffer;
    const int fftSize, numBins;
    HeapBlock<float> tempBuffer;
    Array<CumulativeMovingAverage> ltasAvg;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LTAS);
};

#endif
#endif  // __DROWAUDIO_LTAS_H__