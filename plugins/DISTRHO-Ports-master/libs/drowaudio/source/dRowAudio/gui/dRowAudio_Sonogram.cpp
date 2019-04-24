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

#if JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL

Sonogram::Sonogram (int fftSizeLog2)
:	fftEngine       (fftSizeLog2),
	needsRepaint    (true),
	tempBlock       (fftEngine.getFFTSize()),
	circularBuffer  (fftEngine.getMagnitudesBuffer().getSize() * 4),
	logFrequency    (false),
    scopeLineW      (1.0f)
{
	setOpaque (true);

	fftEngine.setWindowType (Window::Hann);
	numBins = fftEngine.getFFTProperties().fftSizeHalved;
    
    circularBuffer.reset();
    
    scopeImage = Image (Image::RGB,
                        100, 100,
                        false);
    scopeImage.clear (scopeImage.getBounds(), Colours::black);
}

Sonogram::~Sonogram()
{
}

void Sonogram::resized()
{
    const ScopedLock sl (lock);
    scopeImage = scopeImage.rescaled (jmax (1, getWidth()), jmax (1, getHeight()));
}

void Sonogram::paint(Graphics &g)
{
    const ScopedLock sl (lock);
    g.drawImageAt (scopeImage, 0, 0, false);
}

//==============================================================================
void Sonogram::setLogFrequencyDisplay (bool shouldDisplayLog)
{
    logFrequency = shouldDisplayLog;
}

void Sonogram::setBlockWidth (int newBlockWidth)
{
    jassert (newBlockWidth > 0);
    scopeLineW = (float) newBlockWidth;
}

int Sonogram::getBlockWidth() const
{
    return (int) scopeLineW;
}

//==============================================================================
void Sonogram::copySamples (const float* samples, int numSamples)
{
	circularBuffer.writeSamples (samples, numSamples);
	needToProcess = true;
}

void Sonogram::timerCallback()
{
    if (needsRepaint)
        repaint();
}

void Sonogram::process()
{
    jassert (circularBuffer.getNumFree() != 0); // buffer is too small!
    
    while (circularBuffer.getNumAvailable() > fftEngine.getFFTSize())
	{
		circularBuffer.readSamples (tempBlock.getData(), fftEngine.getFFTSize());
		fftEngine.performFFT (tempBlock);
		fftEngine.findMagnitudes();

        renderScopeLine();
        
		needsRepaint = true;
	}
}

void Sonogram::flagForRepaint()
{	
    needsRepaint = true;
    repaint();
}

void Sonogram::renderScopeLine()
{
    const ScopedLock sl (lock);

    scopeImage.moveImageSection (0, 0, (int) scopeLineW, 0,
                                 scopeImage.getWidth(), scopeImage.getHeight());

    const int h = scopeImage.getHeight();
    
    Graphics g (scopeImage);
    const int x = scopeImage.getWidth() - (int) scopeLineW;
        
    const int numBins = fftEngine.getMagnitudesBuffer().getSize() - 1;
    const float yScale = (float) h / (numBins + 1);
    const float* data = fftEngine.getMagnitudesBuffer().getData();
    
    float amp = jlimit (0.0f, 1.0f, (float) (1 + (toDecibels (data[0]) / 100.0f)));
    float y2, y1 = 0;
    
    if (logFrequency)
    {
        for (int i = 0; i < numBins; ++i)
        {
            amp = jlimit (0.0f, 1.0f, (float) (1 + (toDecibels (data[i]) / 100.0f)));
            y2 = log10 (1 + 39 * ((i + 1.0f) / numBins)) / log10 (40.0f) * h;

            g.setColour (Colour::greyLevel (amp));
            g.fillRect ((float)x, h - y2, scopeLineW, y1 - y2);
            
            y1 = y2;
        }	
    }
    else
    {
        for (int i = 0; i < numBins; ++i)
        {
            amp = jlimit (0.0f, 1.0f, (float) (1 + (toDecibels (data[i]) / 100.0f)));
            y2 = (i + 1) * yScale;
            
            g.setColour (Colour::greyLevel (amp));
            g.fillRect ((float) x, h - y2, scopeLineW, y1 - y2);

            y1 = y2;
        }	
    }
}

#endif // JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL
