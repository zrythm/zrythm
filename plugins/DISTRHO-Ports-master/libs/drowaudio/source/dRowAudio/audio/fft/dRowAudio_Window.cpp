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


Window::Window()
    : windowType (Window::Hann), windowFactor (1.0f), oneOverWindowFactor (1.0f),
      windowBuffer (1, 0)
{
}

Window::Window (int windowSize_)
    : windowType (Window::Hann), windowFactor (1.0f), oneOverWindowFactor (1.0f),
      windowBuffer (1, windowSize_)
{
	setUpWindowBuffer();
}

Window::Window (int windowSize_, WindowType type)
    : windowType (type), windowFactor (1.0f), oneOverWindowFactor (1.0f),
      windowBuffer (1, windowSize_)
{
	setUpWindowBuffer();
}

Window::~Window()
{
}

void Window::setWindowType (WindowType newType)
{
    if (windowType == newType)
        return;

	windowType = newType;
	setUpWindowBuffer();
}

void Window::setWindowSize (int newSize)
{
    if (windowBuffer.getNumSamples() == newSize)
        return;
    
    windowBuffer.setSize (1, newSize);
	setUpWindowBuffer();
}

void Window::applyWindow (float* samples, const int numSamples) const noexcept
{
    const float* window = windowBuffer.getReadPointer (0);
    const int windowSize = windowBuffer.getNumSamples();
    jassert (numSamples == windowSize); // Set your window size properly!

    FloatVectorOperations::multiply (samples, window, numSamples);

	if (numSamples > windowSize)
        FloatVectorOperations::clear (samples + windowSize, numSamples - windowSize);
}

void Window::setUpWindowBuffer()
{
    const int bufferSize = windowBuffer.getNumSamples();
    float* bufferSample = windowBuffer.getWritePointer (0);
    FloatVectorOperations::fill (bufferSample, 1.0f, bufferSize);
	
	switch (windowType)
	{
		case Rectangular:           applyRectangularWindow (bufferSample, bufferSize);          break;
		case Hann:                  applyHannWindow (bufferSample, bufferSize);                 break;
		case Hamming:               applyHammingWindow (bufferSample, bufferSize);              break;
		case Cosine:                applyCosineWindow (bufferSample, bufferSize);               break;
		case Lanczos:               applyLanczosWindow (bufferSample, bufferSize);              break;
		case ZeroEndTriangle:       applyZeroEndTriangleWindow (bufferSample, bufferSize);      break;
		case NonZeroEndTriangle:    applyNonZeroEndTriangleWindow (bufferSample, bufferSize);   break;
		case Gaussian:              applyGaussianWindow (bufferSample, bufferSize);             break;
		case BartlettHann:          applyBartlettHannWindow (bufferSample, bufferSize);         break;
		case Blackman:              applyBlackmanWindow (bufferSample, bufferSize);             break;
		case Nuttall:               applyNuttallWindow (bufferSample, bufferSize);              break;
		case BlackmanHarris:        applyBlackmanHarrisWindow (bufferSample, bufferSize);       break;
		case BlackmanNuttall:       applyBlackmanNuttallWindow (bufferSample, bufferSize);      break;
		case FlatTop:               applyFlatTopWindow (bufferSample, bufferSize);              break;
		default:                    applyRectangularWindow (bufferSample, bufferSize);          break;
	}

	oneOverWindowFactor = 1.0f / windowFactor;
}

//==============================================================================
void Window::applyRectangularWindow (float *samples, const int numSamples)
{
	const double oneOverSize = (1.0f / numSamples);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		float window = 1.0f;
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;
}	

void Window::applyHannWindow (float* samples, const int numSamples)
{
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Hann window equation
		float window = 0.5f * (1.0f - (float) (std::cos (i * oneOverSizeMinusOne * twoTimesPi)));
		samples[i] *= window;
		windowFactor += window;
	}
	
    windowFactor *= (float) oneOverSize;
}

void Window::applyHammingWindow (float *samples, const int numSamples)
{
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Hamming window equation
		float window = 0.54f - 0.46f * (float) (std::cos (twoTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;	
}

void Window::applyCosineWindow (float *samples,  const int numSamples)
{
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Cosine window equation
		float window = (float) std::sin (double_Pi * i * oneOverSizeMinusOne);
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;		
}

void Window::applyLanczosWindow (float *samples,  const int numSamples)
{
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Lanczos window equation
		float window = (float) sincPi ((2.0 * i * oneOverSizeMinusOne) - 1.0);
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;			
}

void Window::applyZeroEndTriangleWindow (float *samples,  const int numSamples)
{
	const int sizeMinusOne = numSamples - 1;
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Triangle window equation
		float window = (float) ((2.0 * oneOverSizeMinusOne) * ((sizeMinusOne * 0.5) - std::fabs (double (i) - (sizeMinusOne * 0.5))));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;			
}

void Window::applyNonZeroEndTriangleWindow(float *samples,  const int numSamples)
{
	const int sizeMinusOne = numSamples - 1;
	const double oneOverSize = (1.0 / numSamples);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Triangle window equation
		float window = (float) ((2.0 * oneOverSize) * ((numSamples * 0.5) - std::fabs (double (i) - (sizeMinusOne * 0.5))));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;			
}

void Window::applyGaussianWindow (float *samples,  const int numSamples)
{
	const float sigma = 0.4f;

	const int sizeMinusOne = numSamples - 1;
	const double oneOverSize = (1.0 / numSamples);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Gaussian window equation
		float window = (float) (std::exp (-0.5 * squareNumber ((i - sizeMinusOne * 0.5) / (sigma * sizeMinusOne * 0.5))));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;				
}

void Window::applyBartlettHannWindow (float *samples,  const int numSamples)
{
	const float a0 = 0.62f;
	const float a1 = 0.48f;
	const float a2 = 0.38f;
	
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Bartlett-Hann window equation
		float window = (float) (a0 - a1 * std::fabs (i * oneOverSizeMinusOne - 0.5) - a2 * std::cos (twoTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;			
}

void Window::applyBlackmanWindow (float *samples,  const int numSamples)
{
	const float alpha = 0.16f;
	const float a0 = (1.0f - alpha) * 0.5f;
	const float a1 = 0.5f;
	const float a2 = alpha * 0.5f;
	
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Blackman window equation
		float window = (float) (a0 - a1 * std::cos (twoTimesPi * i * oneOverSizeMinusOne) + a2 * std::cos (fourTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
	
    windowFactor *= (float) oneOverSize;			
}

void Window::applyNuttallWindow (float* samples,  const int numSamples)
{
	const float a0 = 0.355768f;
	const float a1 = 0.487396f;
	const float a2 = 0.144232f;
	const float a3 = 0.012604f;
	
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Nuttall window equation
		float window = a0 - a1 * (float) (std::cos (twoTimesPi * i * oneOverSizeMinusOne))
		+ a2 * (float) (std::cos (fourTimesPi * i * oneOverSizeMinusOne))
		- a3 * (float) (std::cos (sixTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;				
}

void Window::applyBlackmanHarrisWindow (float* samples, const int numSamples)
{
	const float a0 = 0.35875f;
	const float a1 = 0.48829f;
	const float a2 = 0.14128f;
	const float a3 = 0.01168f;
	
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Blackman-Harris window equation
		float window = a0 - a1 * (float) (std::cos (twoTimesPi * i * oneOverSizeMinusOne))
		+ a2 * (float) (std::cos (fourTimesPi * i * oneOverSizeMinusOne))
		- a3 * (float) (std::cos (sixTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;				
}

void Window::applyBlackmanNuttallWindow (float* samples, const int numSamples)
{
	const float a0 = 0.3635819f;
	const float a1 = 0.4891775f;
	const float a2 = 0.1365995f;
	const float a3 = 0.0106411f;
	
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Blackman-Nuttall window equation
		float window = a0 - a1 * (float) (std::cos (twoTimesPi * i * oneOverSizeMinusOne))
		+ a2 * (float) (std::cos (fourTimesPi * i * oneOverSizeMinusOne))
		- a3 * (float) (std::cos (sixTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;				
}

void Window::applyFlatTopWindow (float* samples, const int numSamples)
{
	const float a0 = 1.0f;
	const float a1 = 1.93f;
	const float a2 = 1.29f;
	const float a3 = 0.388f;
	const float a4 = 0.032f;
	
	const double oneOverSize = (1.0 / numSamples);
	const double oneOverSizeMinusOne = 1.0 / (numSamples - 1.0);
	windowFactor = 0.0f;
    
	for (int i = 0; i < numSamples; i++)
	{
		// Flat-Top window equation
		float window = a0 - a1 * (float) (std::cos (twoTimesPi * i * oneOverSizeMinusOne))
		+ a2 * (float) (std::cos (fourTimesPi * i * oneOverSizeMinusOne))
		- a3 * (float) (std::cos (sixTimesPi * i * oneOverSizeMinusOne))
		+ a4 * (float) (std::cos (2.0 * fourTimesPi * i * oneOverSizeMinusOne));
		samples[i] *= window;
		windowFactor += window;
	}
    
	windowFactor *= (float) oneOverSize;				
}

