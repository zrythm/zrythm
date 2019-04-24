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



OnePoleFilter::OnePoleFilter() noexcept
    : y1 (0.0f), b0 (1.0f), a1 (0.0f)
{
}

OnePoleFilter::~OnePoleFilter() noexcept
{
}

void OnePoleFilter::processSamples (float* const samples,
                                    const int numSamples) noexcept
{
    // make sure sample values are locked
    const ScopedLock sl (lock);
    
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = (b0 * samples[i]) + (a1 * y1);
        y1 = samples[i];
    }
}


void OnePoleFilter::makeLowPass (const double sampleRate,
                                 const double frequency) noexcept
{
    const double w0 = 2.0 * double_Pi * (frequency / sampleRate);
    const double cos_w0 = cos (w0);

    const double alpha = (2.0f - cos_w0) - sqrt ((2.0 - cos_w0) * (2.0 - cos_w0) - 1.0);

    const ScopedLock sl (lock);
    
    b0 = 1.0f - (float) alpha;
    a1 = (float) alpha;
}

void OnePoleFilter::makeHighPass (const double sampleRate,
                                  const double frequency) noexcept
{
    const double w0 = 2.0 * double_Pi * (frequency / sampleRate);
    const double cos_w0 = cos (w0);
    
    const double alpha = (2.0 + cos_w0) - sqrt ((2.0 + cos_w0) * (2.0 + cos_w0) - 1.0);

    const ScopedLock sl (lock);

    b0 = (float) alpha - 1.0f;
    a1 = (float) -alpha;
}

