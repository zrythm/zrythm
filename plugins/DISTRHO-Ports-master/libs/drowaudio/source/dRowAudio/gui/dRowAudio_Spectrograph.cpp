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

Spectrograph::Spectrograph (int fftSizeLog2)
    : fftEngine         (fftSizeLog2),
      circularBuffer    (fftEngine.getFFTSize() + 1),
      fftMagnitudesData (128),
      tempBlock         (fftEngine.getFFTSize()),
      logFrequency      (false),
      binSize           (0.0f, 0.0f, 1.0f, 1.0f)
{
	fftEngine.setWindowType (Window::Hann);
	numBins = fftEngine.getFFTProperties().fftSizeHalved;
    
    reset();
}

Spectrograph::~Spectrograph()
{
}

//==============================================================================
Image Spectrograph::generateImage (const float* samples, int numSamples)
{
    reset();
    ensureStorageAllocated (numSamples);
    processSamples (samples, numSamples);
    
    return createImage();
}

void Spectrograph::reset() noexcept
{
    circularBuffer.reset();
    fftMagnitudesData.reset();
    fftMagnitudesBlocks.clear();
}

void Spectrograph::ensureStorageAllocated (int numSamples)
{
    const int numBlocks = numSamples / fftEngine.getFFTSize();
    const int totalNumBins = numBlocks * numBins;
    
    fftMagnitudesData.setSize (totalNumBins);
    fftMagnitudesBlocks.ensureStorageAllocated (numBlocks);
}

void Spectrograph::processSamples (const float* samples, int numSamples)
{
    int numLeft = numSamples;
    const float* data = samples;
    
    while (numLeft > 0)
    {
        const int numThisTime = jmin (numLeft, fftEngine.getFFTSize());
        
        circularBuffer.writeSamples (data, numThisTime);

        if (circularBuffer.getNumAvailable() >= fftEngine.getFFTSize())
        {
            circularBuffer.readSamples (tempBlock.getData(), fftEngine.getFFTSize());
            fftEngine.performFFT (tempBlock);
            fftEngine.findMagnitudes();
            
            addMagnitudesBlock (fftEngine.getMagnitudesBuffer().getData(),
                                fftEngine.getMagnitudesBuffer().getSize() - 1);
        }
        
        data += numThisTime;
        numLeft -= numThisTime;
    }
}

Image Spectrograph::createImage() const
{
    if (fftMagnitudesBlocks.size() == 0 || numBins == 0)
    {
        jassertfalse;
        return Image();
    }
    
    const float bW = binSize.getWidth();
    const float bH = binSize.getHeight();
    const int w = (int) std::ceil (bW * fftMagnitudesBlocks.size());
    const int h = (int) std::ceil (bH * numBins);
    
    Image image (Image::RGB, w, h, false);
    Graphics g (image);
    g.fillAll (Colours::black);

    float x1 = 0.0f;
    
    for (int i = 0; i < fftMagnitudesBlocks.size(); ++i)
    {
        float x2 = x1 + bW;
        const float yScale = (float) h / (numBins + 1);
        const float* data = fftMagnitudesBlocks.getUnchecked (i);
        
        float amp = jlimit (0.0f, 1.0f, (float) (1 + (toDecibels (data[0]) / 100.0f)));
        float y2, y1 = 0.0f;
        
        if (logFrequency)
        {
            for (int i = 0; i < numBins; ++i)
            {
                amp = jlimit (0.0f, 1.0f, (float) (1 + (toDecibels (data[i]) / 100.0f)));
                y2 = log10 (1 + 39 * ((i + 1.0f) / numBins)) / log10 (40.0f) * h;
                
                g.setColour (Colour::greyLevel (amp));
                g.fillRect (x1, h - y2, bW, y1 - y2);
                
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
                g.fillRect (x1, h - y2, bW, y1 - y2);
                
                y1 = y2;
            }	
        }
        
        x1 = x2;
    }
    
    return image;
}

//==============================================================================
void Spectrograph::setLogFrequencyDisplay (bool shouldDisplayLog)
{
    logFrequency = shouldDisplayLog;
}

void Spectrograph::setBinSize (const Rectangle<float>& size) noexcept
{
    jassert (size.getWidth() > 0.0);
    jassert (size.getHeight() > 0.0);
    binSize = size;
}

void Spectrograph::addMagnitudesBlock (const float* data, int size)
{
    if (fftMagnitudesData.getNumFree() < size)
        fftMagnitudesData.setSizeKeepingExisting (fftMagnitudesData.getSize() - size);

    float* startOfData = fftMagnitudesData.getData() + fftMagnitudesData.getNumAvailable();
    fftMagnitudesBlocks.add (startOfData);
    
    fftMagnitudesData.writeSamples (data, size);
}

void Spectrograph::renderScopeLine()
{

}

#endif // JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL
