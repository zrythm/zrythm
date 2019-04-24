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

#ifndef DROWAUDIO_FFT_H_INCLUDED
#define DROWAUDIO_FFT_H_INCLUDED

#if (JUCE_MAC || JUCE_IOS) && ! DROWAUDIO_USE_FFTREAL

typedef FFTSetup FFTConfig;
typedef DSPSplitComplex SplitComplex;

#elif DROWAUDIO_USE_FFTREAL

typedef ScopedPointer< ffft::FFTReal<float> > FFTConfig;

struct SplitComplex
{
    float* realp;
    float* imagp;
};

#endif

#if JUCE_MAC || JUCE_IOS || DROWAUDIO_USE_FFTREAL

//==============================================================================
/** Low-level FFT class for performing single FFT calculations.
 
    This wraps together all the various set-up steps to perform an FFT so you can just 
    create one of these and call its performFFT method and retrieve the result with getFFTBuffer().
 
    If you are doing lots of FFT operations take a look at FFTEngine which is better suited to
    easily finding magnitude bins.
 */
class FFT
{
public:
    //==============================================================================
    /** Holds the properties for an FFT operation.
        Essentially pre-calculates some commonly used values for a given FFT size.
     */
    class Properties
    {
    public:
        //==============================================================================
        /** Creates a set of proeprties for a given FFT size. */
        Properties (int fftSizeLog2_) noexcept
            : fftSizeLog2 (fftSizeLog2_),
              fftSize (1 << fftSizeLog2),
              fftSizeMinus1 (fftSize - 1),
              fftSizeHalved (fftSize >> 1),
              oneOverFFTSizeMinus1 (1.0 / fftSizeMinus1),
              oneOverFFTSize (1.0 / fftSize)
        {}
        
        /** Creates a copy of another set of Properties. */
        Properties (const Properties& other) noexcept
        {
            *this = other;
        }
        
        /** Creates a copy of another set of Properties. */
        Properties& operator= (const Properties& other) noexcept
        {
            fftSizeLog2             = other.fftSizeLog2;
            fftSize                 = other.fftSize;
            fftSizeMinus1           = other.fftSizeMinus1;
            fftSizeHalved           = other.fftSizeHalved;
            oneOverFFTSizeMinus1    = other.oneOverFFTSizeMinus1;
            oneOverFFTSize          = other.oneOverFFTSize;
            
            return *this;
        }

        //==============================================================================
        int fftSizeLog2;
        int fftSize;
        int fftSizeMinus1;
        int fftSizeHalved;
        
        double oneOverFFTSizeMinus1;
        double oneOverFFTSize;
        
    private:
        //==============================================================================
        Properties() JUCE_DELETED_FUNCTION;
        JUCE_LEAK_DETECTOR (Properties)
    };
    
    //==============================================================================
    /** Creates an FFT class that can perform various FFT operations on blocks of data.
        The internals will vary depending on platform e.g. one the Mac Accelerate is used, on Windows FFTReal.
     */
    FFT (int fftSizeLog2);
    
    /** Destructor. */
    ~FFT();

    /** Changes the FFT size. */
    void setFFTSizeLog2 (int newFFTSize);
    
    /** Returns the Properties in use. */
    Properties getProperties() const noexcept           { return properties; }

    /** Returns the internal buffer.
        The contents of this will vary depending on whether you've just performed an FFT or IFFT but
        will be the result of the operation either way..
     */
    float* getBuffer()                                  { return buffer.getData(); }

    /** Returns the SplitComplex of the buffer.
        This is basically just a pair of pointers to the real and imag parts of the buffer.
     */
    SplitComplex& getFFTBuffer()                        { return bufferSplit; }
    
    /** Performs an FFT operation on a set of samples.
        N.B. samples must be an array the same size as the FFT. After processing you can retrive the 
        buffer using getBuffer or getFFTBuffer.
     */
    void performFFT (float* samples);

    /** Calculates and returns the magnitudes of the previous buffer.
        N.B. magnitudes should be as at least half the FFT size.
     */
    void getMagnitudes (float* magnitudes);

    /** Calculates and returns the phase of the previous buffer.
        N.B. phaseBuffer should be as at least half the FFT size.
     */
    void getPhase (float* phaseBuffer);

    /** Performs an inverse FFT.
        fftBuffer should be in SplitComplex format where [0] = realp & [fftSize / 2] = imagp.
        N.B. fftBuffer must be the same size as the FFTProperties fftSize and the buffer must not be 
        the same as that retrieved from getBuffer.
     */
    void performIFFT (float* fftBuffer);

    /** Calculates the magnitude of an FFT bin. */
    static float magnitude (const float real, const float imag,
                            const float oneOverFFTSize,  const float oneOverWindowFactor)
    {
        const float rawMagnitude = hypotf (real, imag);
        const float magnitudeForFFTSize = rawMagnitude * oneOverFFTSize;
        const float magnitudeForWindowFactor = magnitudeForFFTSize * oneOverWindowFactor;
        
        return magnitudeForWindowFactor;
    }

private:
    //==============================================================================
    Properties properties;
    HeapBlock<float> buffer;
    
    FFTConfig config;
    SplitComplex bufferSplit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFT)
};

//==============================================================================
/**
    Engine to continuously perform FFT operations and easily calculate the resultant magnitudes.
 */
class FFTEngine
{
public:
    //==============================================================================
    /** Creates an FFTEngine with a given size. */
    FFTEngine (int fftSizeLog2)
        : fft (fftSizeLog2),
          window (getFFTProperties().fftSize),
          magnitutes (getFFTProperties().fftSizeHalved + 1)
    {
    }
    
    /** Destructor. */
    ~FFTEngine() {}
    
    /** Performs an FFT operation.
        The number of samples must be equal to the fftSize.
     */
    void performFFT (float* samples);
    
    /**	This will fill the internal buffer with the magnitudes of the last performed FFT.
        You can then get this buffer using getMagnitudesBuffer(). Remember that
        the size of the buffer is the fftSizeHalved + 1 to incorporate the Nyquist.
     */
    void findMagnitudes()                               { findMagnitues (magnitutes.getData(), false); }

    /**	Fills a provided buffer with the magnitudes of the last performed FFT. */
    void findMagnitudes (Buffer& bufferToFill)          { findMagnitues (bufferToFill.getData(), false); }

    /**	This will fill the buffer with the magnitudes of the last performed FFT if they are bigger.
        You can then get this buffer using getMagnitudesBuffer(). Remember that
        the size of the buffer is the fftSizeHalved + 1 to incorporate the Nyquist.
     */
    void updateMagnitudesIfBigger()                     { findMagnitues (magnitutes.getData(), true); }
    
    /** Changes the Window type. */
    void setWindowType (Window::WindowType type)        { window.setWindowType (type); }
    
    /** Returns the FFT size. */
    int getFFTSize()const noexcept                      { return getFFTProperties().fftSize; }
    
    /** Returns the magnitutes buffer. */
    Buffer& getMagnitudesBuffer()                       { return magnitutes; }
    
    /** Returns the Window buffer. */
    Window& getWindow()                                 { return window; }
    
    /** Returns a copy of the FFT Properties. */
    FFT::Properties getFFTProperties() const noexcept   { return fft.getProperties(); }
    
private:
    //==============================================================================
    FFT fft;
    Window window;
    Buffer magnitutes;
    
    void findMagnitues (float* magBuf, bool onlyIfBigger);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTEngine)
};

#endif
#endif  // DROWAUDIO_FFT_H_INCLUDED
