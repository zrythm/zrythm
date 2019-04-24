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

//==============================================================================
#if DROWAUDIO_USE_FFTREAL

FFT::FFT (int fftSizeLog2)
    : properties (fftSizeLog2)
{
    config = new ffft::FFTReal<float> (properties.fftSize);
    
    buffer.malloc (properties.fftSize);
    bufferSplit.realp = buffer.getData();
    bufferSplit.imagp = bufferSplit.realp + properties.fftSizeHalved;
}

FFT::~FFT()
{
}

void FFT::setFFTSizeLog2 (int newFFTSizeLog2)
{
    if (newFFTSizeLog2 != properties.fftSizeLog2)
    {
        config = nullptr;
        
        properties = Properties (newFFTSizeLog2);
        buffer.malloc (properties.fftSize);
        bufferSplit.realp = buffer.getData();
        bufferSplit.imagp = bufferSplit.realp + properties.fftSizeHalved;
        
        config = new ffft::FFTReal<float> (properties.fftSize);
    }
}

void FFT::performFFT (float* samples)
{
    config->do_fft (buffer.getData(), samples);
}

void FFT::getPhase (float* phaseBuffer)
{
    const int numSamples = properties.fftSizeHalved;
    float* real = bufferSplit.realp;
    float* img = bufferSplit.imagp;
    
    for (int i = 1; i < numSamples; ++i)
        phaseBuffer[i] = std::atan2 (img[i], real[i]);

    phaseBuffer[0] = 0.0f;
}

void FFT::performIFFT (float* fftBuffer)
{
    config->do_ifft (fftBuffer, buffer.getData());
}

#elif JUCE_MAC || JUCE_IOS
    
FFT::FFT (int fftSizeLog2)
    : properties (fftSizeLog2)
{
	config = vDSP_create_fftsetup (properties.fftSizeLog2, 0);

	buffer.malloc (properties.fftSize);
	bufferSplit.realp = buffer.getData();
	bufferSplit.imagp = bufferSplit.realp + properties.fftSizeHalved;
}

FFT::~FFT()
{
	vDSP_destroy_fftsetup (config);
}

void FFT::setFFTSizeLog2 (int newFFTSizeLog2)
{
	if (newFFTSizeLog2 != properties.fftSizeLog2)
    {
		vDSP_destroy_fftsetup (config);
		
		properties = Properties (newFFTSizeLog2);
		buffer.malloc (properties.fftSize);
		bufferSplit.realp = buffer.getData();
		bufferSplit.imagp = bufferSplit.realp + properties.fftSizeHalved;
		
		config = vDSP_create_fftsetup (properties.fftSizeLog2, 0);
	}
}

void FFT::performFFT (float* samples)
{
	vDSP_ctoz ((COMPLEX*) samples, 2, &bufferSplit, 1, properties.fftSizeHalved);
	vDSP_fft_zrip (config, &bufferSplit, 1, properties.fftSizeLog2, FFT_FORWARD);
}

void FFT::getPhase (float* phaseBuffer)
{
    vDSP_zvphas (&bufferSplit, 1, phaseBuffer, 1, properties.fftSizeHalved);
    phaseBuffer[0] = 0.0f;
}

void FFT::performIFFT (float* fftBuffer)
{
    SplitComplex split;
    split.realp = fftBuffer;
	split.imagp = fftBuffer + properties.fftSizeHalved;

    jassert (split.realp != bufferSplit.realp); // These can't point to the same data!

	vDSP_fft_zrip (config, &split, 1, properties.fftSizeLog2, FFT_INVERSE);
    vDSP_ztoc (&split, 1, (COMPLEX*) buffer.getData(), 2, properties.fftSizeHalved);
}

#endif

void FFT::getMagnitudes (float* magnitudes)
{
    const float oneOverFFTSize = (float) properties.oneOverFFTSize;
    const int fftSizeHalved = properties.fftSizeHalved;
    const float oneOverWindowFactor = 1.0f;

    SplitComplex fftSplit;
    fftSplit.realp = buffer.getData();
    fftSplit.imagp = fftSplit.realp + fftSizeHalved;
    
    magnitudes[0] = magnitude (fftSplit.realp[0], 0.0f, oneOverFFTSize, oneOverWindowFactor);
    
    for (int i = 1; i < fftSizeHalved; ++i)
        magnitudes[i] = magnitude (fftSplit.realp[i], fftSplit.imagp[i], oneOverFFTSize, oneOverWindowFactor);
    
    magnitudes[fftSizeHalved] = magnitude (fftSplit.realp[0], 0.0f, oneOverFFTSize, oneOverWindowFactor);
}

//============================================================================
void FFTEngine::performFFT (float* samples)
{	
	// First apply the current window
	window.applyWindow (samples, getFFTProperties().fftSize);
	fft.performFFT (samples);
}

void FFTEngine::findMagnitues (float* magBuf, bool onlyIfBigger)
{
    const SplitComplex& fftSplit = fft.getFFTBuffer();
    const float oneOverFFTSize = (float) getFFTProperties().oneOverFFTSize;
    const int fftSizeHalved = getFFTProperties().fftSizeHalved;
    const float oneOverWindowFactor = window.getOneOverWindowFactor();
    
    // Find magnitudes
    {
        const float newMag = FFT::magnitude (fftSplit.realp[0], 0.0f, oneOverFFTSize, oneOverWindowFactor); // imag for DC is always zero
        
        if (! onlyIfBigger || (newMag > magBuf[0]))
            magBuf[0] = newMag;
    }
    
    for (int i = 1; i < fftSizeHalved; ++i)
    {
        const float newMag = FFT::magnitude (fftSplit.realp[i], fftSplit.imagp[i], oneOverFFTSize, oneOverWindowFactor);
        
        if (! onlyIfBigger || (newMag > magBuf[i]))
            magBuf[i] = newMag;
    }

    {
        const float newMag = FFT::magnitude (fftSplit.realp[0], 0.0f, oneOverFFTSize, oneOverWindowFactor); // imag for Nyquist is always zero

        if (! onlyIfBigger || (newMag > magBuf[fftSizeHalved]))
            magBuf[fftSizeHalved] = newMag;
    }
    
    magnitutes.updateListeners();
}
