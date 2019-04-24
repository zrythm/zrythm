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



GraphicalComponent::GraphicalComponent()
    : paused (false),
	  needToProcess (true),
      sleepTime (5),
	  numSamples (0)
{
	samples.malloc (numSamples);

	startTimer (30);
}

GraphicalComponent::~GraphicalComponent()
{
}

int GraphicalComponent::useTimeSlice()
{
	if (paused)
    {
		return sleepTime;
	}
	else
	{
		if (needToProcess)
        {
			process();
            needToProcess = false;
            
            return sleepTime;
        }
        
		return sleepTime;
	}
}

void GraphicalComponent::copySamples (const float *values, int numSamples_)
{
		// allocate new memory only if needed
		if (numSamples != numSamples_)
        {
			numSamples = numSamples_;
			samples.malloc (numSamples);
		}
		
		// lock whilst copying
		ScopedLock sl (lock);
		memcpy (samples, values, numSamples * sizeof (float));
		
		needToProcess = true;
}

void GraphicalComponent::copySamples (float **values, int numSamples_, int numChannels)
{
	// allocate new memory only if needed
	if (numSamples != numSamples_) 
    {
		numSamples = numSamples_;
		samples.malloc (numSamples);
	}
	
	// lock whilst copying
	ScopedLock sl (lock);

	if (numChannels == 1)
    {
		memcpy (samples, values[0], numSamples * sizeof (float));
	}
	// this is quicker than the generic method below
	else if (numChannels == 2) 
    {
		for (int i = 0; i < numSamples; i++)
        {
			samples[i] = (fabsf (values[0][i]) > fabsf (values[1][i])) ? values[0][i] : values[1][i];
		}
	}
	else
    {
		samples.clear (numSamples);
		for (int c = 0; c < numChannels; c++) 
        {
			for (int i = 0; i < numSamples; i++) 
            {
				if (fabsf (values[c][i]) > samples[i])
                {
					samples[i] = values[c][i];
				}
			}			
		}
	}
	
	needToProcess = true;
}

