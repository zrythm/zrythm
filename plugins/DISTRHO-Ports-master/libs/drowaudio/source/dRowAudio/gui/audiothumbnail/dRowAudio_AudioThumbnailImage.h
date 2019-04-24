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

#ifndef __DROWAUDIO_AUDIOTHUMBNAILIMAGE_H__
#define __DROWAUDIO_AUDIOTHUMBNAILIMAGE_H__

//==============================================================================	
/** A class to display the waveform of an audio file.
	
	This will load an audio file and display its waveform. All waveform rendering
    happens on a background thread. This will listen to changes in the
    AudioFilePlayer passed in and update the thumbnail accordingly.
    
    You can either get the whole image using getImage() or you can get a scaled
    section using getImageAtTime().
 
    You can also register as a listener to recive update when the sourc changes
    or new data has been generated.
 */
class AudioThumbnailImage : public Timer,
                            public TimeSliceClient,
                            public AudioFilePlayer::Listener
{
public:
    //==============================================================================	
    /** Creates the AudioThumbnailImage.

		The file player associated with the display must be passed in.
		To save on the number of threads in your program you can optionally pass in your own
		AudioThumbnailCache. If you pass in your own the caller is responsible for deleting it,
		if not the PositionableWaveform will create and delete its own when not needed.	 
	 */
	explicit AudioThumbnailImage (AudioFilePlayer& sourceToBeUsed,
                                  TimeSliceThread& backgroundThread,
                                  AudioThumbnailBase& thumbnailToUse,
                                  int sourceSamplesPerThumbnailSample);
	
	/** Destructor. */
	~AudioThumbnailImage();
	
    /** Sets the colour to use for the background.
     */
    void setBackgroundColour (const Colour& newBackgroundColour);
    
    /** Sets the colour to use for the waveform.
     */
    void setWaveformColour (const Colour& newWaveformColour);
    
    /** Sets the image resolution in lines per pixel.
        This will cause the waveform to be re-generated from the source.
     */
    void setResolution (double newResolution);

	//====================================================================================
    /** Returns the whole waveform image.
     */
    const Image getImage()                          {   return waveformImage;   }
    
    /** Returns a section of the image at a given time for a given duration.
     */
    const Image getImageAtTime (double startTime, double duration);
    
    /** Returns the AudioFilePlayer currently being used.
     */
    AudioFilePlayer& getAudioFilePlayer()           {   return filePlayer;      }
    
    /** Retuns the ammount of time that has been rendered.
     */
    double getTimeRendered()                        {   return lastTimeDrawn;   }
    
    /** Returns true if the Image has finished rendering;
     */
    bool hasFinishedLoading()                       {   return renderComplete;  }

    /** Returns the number of sourceSamplesPerThumbnailSample if set in the constructor.
     */
    int getNumSourceSamplesPerThumbnailSamples()    {   return sourceSamplesPerThumbnailSample; }

	//====================================================================================
	/** @internal */
    void resized ();
	
	/** @internal */
	void paint (Graphics &g);

	//====================================================================================
	/** @internal */
	void timerCallback ();
    
	/** @internal */
    int useTimeSlice();
    
	/** @internal */
	void fileChanged (AudioFilePlayer *player);
    
    //==============================================================================
    /** A class for receiving callbacks from an AudioThumbnailImage.
        These are called from the internal buffering thread so be sure to lock the
        message manager if you intend to do any graphical related stuff with the Image.
	 
        @see AudioThumbnailImage::addListener, AudioThumbnailImage::removeListener
	 */
    class  Listener
    {
    public:
        //==============================================================================
        /** Destructor. */
        virtual ~Listener() {}
		
        //==============================================================================
        /** Called when the source file changes and the image starts rendering.
		 */
        virtual void imageChanged (AudioThumbnailImage* /*audioThumbnailImage*/) {}
		
        /** Called when the the image is updated.
            This will be continuously called while the waveform is being generated.
		 */
        virtual void imageUpdated (AudioThumbnailImage* /*audioThumbnailImage*/) {}
        
        /** Called when the the image has finished rendering.
            If you are using a scaled version of the Image it might be worth caching 
            your own copy to avoid having to rescale it each time.
         */
        virtual void imageFinished (AudioThumbnailImage* /*audioThumbnailImage*/) {}
    };
	
    /** Adds a listener to be notified of changes to the AudioThumbnailImage. */
    void addListener (Listener* listener);
	
    /** Removes a previously-registered listener. */
    void removeListener (Listener* listener);
	    
private:
	//==============================================================================
    AudioFilePlayer& filePlayer;
    TimeSliceThread& backgroundThread;
    AudioThumbnailBase& audioThumbnail;
    int sourceSamplesPerThumbnailSample;
    
    ReadWriteLock imageLock;
	Image waveformImage, tempSectionImage;

    Colour backgroundColour, waveformColour;
    
    bool sourceLoaded, renderComplete;
	double fileLength, oneOverFileLength, currentSampleRate, oneOverSampleRate;
    double lastTimeDrawn, resolution;
    
    ListenerList <Listener> listeners;

    //==============================================================================
    void refreshFromFilePlayer();
    void triggerWaveformRefresh();
	void refreshWaveform();

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThumbnailImage);
};


#endif  // __DROWAUDIO_AUDIOTHUMBNAILIMAGE_H__
