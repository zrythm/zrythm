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

#ifndef __DROWAUDIO_IOSAUDIOCONVERTER__
#define __DROWAUDIO_IOSAUDIOCONVERTER__

#if JUCE_IOS || DOXYGEN

//==============================================================================
/**
    This class converts an iPod Library track to a PCM Wav format suitable for
    reading using a CoreAudioFormatReader.
    
    Because of iOS app sandboxing, we don't have direct access to the source
    file. This class will take an AVAssetUrl string and convert the source to an
    application-local file which can be freely modified.
 
    The easiest way to use this is to show an AudioPicker and convert a returned
    item to an AVAssetUrl using AudioPicker::mpMediaItemToAvassetUrl and pass
    this to the start conversion method. Either then continuosly check the
    progress and get the converted file or register youself as a Listener to be
    updated about the conversion process.
 
    @see AudioPicker, AudioPicker::mpMediaItemToAvassetUrl
 */
class IOSAudioConverter
{
public:
    //==============================================================================
    /** Creates an IOSAudioConverter.
     
        Initially this does nothing, use the startConversion method to convert a
        file.
     */
    IOSAudioConverter();
    
    /** Destructor.
     */
    ~IOSAudioConverter();

    /** Starts the conversion process.
     
        Register yourself as a listener to be notified of updates during the
        conversion process.

        @param  avAssetUrl          The AVAssetUrl in String format to convert. This
                                    is usually obtained from an AudioPicker and its
                                    AudioPicker::mpMediaItemToAvassetUrl method.
        @param  convertedFileName   A name for the converted file. This will have
                                    the extension .caf and if left blank will be
                                    called "convertedFile".
     */
    void startConversion (const String& avAssetUrl, const String& convertedFileName = String());
    
    /** Cancels the current conversion.
        A file may be partially converted and can be obtained using the
        getConvertedFile method.
     */
    void cancel();
    
    /** Returns the most recently converted file.
     */
    File getConvertedFile()     {   return convertedFile;   }
    
    /** Returns the progress of the current conversion.
        @returns The current progress, a value between 0 and 1.
     */
    double getProgress()        {   return progress;    }
    
    //==============================================================================
    /** A class for receiving callbacks from a IOSAudioConverter.
     
        To be notified about updates during the conversion process, you can register
        an IOSAudioConverter::Listener object using IOSAudioConverter::addListener().
     
        @see IOSAudioConverter::addListener, IOSAudioConverter::removeListener
     */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() {}
        
        //==============================================================================
        /** Called when a conversion has been started using the startConversion method.
         */
        virtual void conversionStarted() {}
        
        /** Called repeatedly during the conversion process.
         
            @param progress The current progress between 1 and 0.
         */
        virtual void conversionUpdated (double progress) {}

        /** Called when the conversion process has finished.
            
            @param convertedFile    The fully converted .caf file.
         */
        virtual void conversionFinished (const File& convertedFile)  {}
    };
    
    /**	Description
     
        @see removeListener
     */
    void addListener (Listener* newListener);
    
    /**	Description
     
        @see addListener
     */
    void removeListener (Listener* listener);

    //==============================================================================
    /** @internal */
    void sendConverstionStartedMessage (void* juceIOSAudioConverter);
    
    /** @internal */
    void sendConverstionUpdatedMessage (void* juceIOSAudioConverter);

    /** @internal */
    void sendConverstionFinishedMessage (void* juceIOSAudioConverter, void* convertedUrl);

private:
    //==============================================================================
    ListenerList<Listener> listeners;
    
    File convertedFile;
    double progress;

    void* currentAudioConverter;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IOSAudioConverter);
};


#endif
#endif // __DROWAUDIO_IOSAUDIOCONVERTER__