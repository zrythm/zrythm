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

#ifndef __DROWAUDIO_FILTERINGAUDIOSOURCE_H__
#define __DROWAUDIO_FILTERINGAUDIOSOURCE_H__

//==============================================================================
/**	An AudioSource that contains three settable filters to EQ the audio stream.
 */
class FilteringAudioSource : public AudioSource
{

public:
	//==============================================================================
    enum FilterType
    {
        Low = 0,
        Mid,
        High,
        numFilters
    };
    
    enum FilterSetting
    {
        CF = 0,
        Q,
        numFilterSettings
    };
    
	//==============================================================================
    /** Creates an FilteringAudioTransportSource.
	 */
    FilteringAudioSource (AudioSource* inputSource,
                          bool deleteInputWhenDeleted);
	
    /** Destructor. */
    ~FilteringAudioSource();
    
    //==============================================================================
    /** Changes one of the filter gains.
     */
    void setGain (FilterType setting, float newGain);

	/** Toggles the filtering of the transport source.
	 */
	void setFilterSource (bool shouldFilter);

	/** Returns whether the source is being filtered or not.
	 */
	bool getFilterSource()				{ return filterSource; }

    //==============================================================================
    /** Implementation of the AudioSource method. */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
	
    /** Implementation of the AudioSource method. */
    void releaseResources();
	
    /** Implementation of the AudioSource method. */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill);
		
private:
    //==============================================================================
    OptionalScopedPointer<AudioSource> input;
    float gains[numFilters];
	IIRFilter filter[2][numFilters];
	
    double sampleRate;
	bool filterSource;

    //==============================================================================
    void resetFilters();

    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilteringAudioSource);
};

#endif //__DROWAUDIO_FILTERINGAUDIOSOURCE_H__