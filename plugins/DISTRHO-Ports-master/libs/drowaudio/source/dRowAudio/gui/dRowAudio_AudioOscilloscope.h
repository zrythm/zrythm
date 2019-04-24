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

#ifndef __DROWAUDIO_AUDIOOSCILLOSCOPE_H__
#define __DROWAUDIO_AUDIOOSCILLOSCOPE_H__

//==============================================================================
/** An oscilliscope class for displaying audio waveforms.
 
	This is a high-res version of the SimpleAudioScope class and as such is
    slightly more CPU intensive.
	Use this when you need detailed images of a waveform instead of a general
    idea of what is passing through it.
 */
class AudioOscilloscope  :	public Component,
							public Timer
{
public:
	//==============================================================================
    /** Creates an AudioOscilloscope.
     
        Once created simply push data to display by repeatedly calling the
        processBlock method.
     */
    AudioOscilloscope();
	
	/** Destructor. */
    ~AudioOscilloscope();
	
    //==============================================================================
    /**	Processes a number of samples displaying them on the scope.
        The resolution will depend on how zoomed in you are @see setHorizontalZoom.
	 */
    void processBlock (const float* inputChannelData,
                       int numSamples);
	
	/**	Clears the internal buffers.
        It is a good idea to call this when an audio device is started or stopped
        to avoid clicks on the scope.
	 */
	void clear();
	
	/** Adjusts the vertical zoom of the scope.
        0 = nothing, 1 = normal, > 1 = zoomed.
	 */
	void setVerticalZoom (float newVerticalZoomFactor)      {   verticalZoomFactor = newVerticalZoomFactor;     }
    
	/**	Adjusts the horizontal zoom of the scope.
        The lower this value is the more zoomed in and detailed the image will be.
        However, as more information is passing the scope will move quicker and be
        difficult to see. Consider using Component::createComponentSnapshot() to
        capture an image of the scope.
	 */
	void setHorizontalZoom (float newHorizontalZoomFactor)  {	horizontalZoomFactor = newHorizontalZoomFactor; }
    
	/** Sets the background colour of the scope. */
	void setBackgroundColour (Colour newBackgroundColour)	{	backgroundColour = newBackgroundColour;         }
	
	/** Sets the trace colour of the scope. */
	void setTraceColour (Colour newTraceColour)             {	traceColour = newTraceColour;                   }
    
    //==============================================================================
    /** @internal. */
    void resized();
    
	/** @internal */
    void paint (Graphics& g);

	/** @internal */
    void timerCallback();
	
	/** @internal. Used to add a sample to the internal buffer. */
    void addSample (const float sample);
	
private:
    //==============================================================================
    HeapBlock<float> circularBufferMax, circularBufferMin;
	int bufferSizeMask;
    float currentMax, currentMin;
    int volatile bufferPos, lastBufferPos, bufferSize, numSamplesIn;
	float bufferLastMax, bufferLastMin;
	
    Image waveformImage;
    
	float verticalZoomFactor, horizontalZoomFactor;
	Colour backgroundColour, traceColour;
	
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioOscilloscope);
};

#endif //__DROWAUDIO_AUDIOOSCILLOSCOPE_H__