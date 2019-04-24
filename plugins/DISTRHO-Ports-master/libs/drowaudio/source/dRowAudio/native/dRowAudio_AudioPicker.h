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

#ifndef __DROWAUDIO_AUDIOPICKER__
#define __DROWAUDIO_AUDIOPICKER__

#if JUCE_IOS || DOXYGEN

//==============================================================================
/**
    AudioPicker class
 
    An iOS-specific class that can be used for choosing audio files on 
    supported devices.
 */
class AudioPicker
{    
public:
    /** Creates a default AudioPicker.
     
        Register yourself as a listener to this by inheriting from
        AudioPicker::Listener and then call the show method to display the picker.
     */
    AudioPicker();
    
    /** Destructor.
     */
    ~AudioPicker();
    
    //==============================================================================
    /**	Shows the audio picker user interface.
     
        @param allowMultipleSelection   If true multiple items can be selected.
        @param areaToPointTo            On the iPad the picker is shown as a
                                        pop-over, this rectangle represents an area
                                        that the arrow of the pop-over should point to.
     */
    void show (bool allowMultipleSelection = false, Rectangle<int> areaToPointTo = Rectangle<int>());
    
    //==============================================================================
    /** This helper method returns the MPMediaItemPropertyAssetURL for a MPMediaItem
        such as those passed to Listener::audioPickerFinished.
     
        To keep the obj-C code behind the scenes the argument is a void* but should
        be a valid MPMediaItem. A String is returned for ease which can then be
        passed to an IOSAudioConverter.
     
        @returns AVAssetUrl String
     
        @see IOSAudioConverter
     */
    static String mpMediaItemToAvassetUrl (void* mpMediaItem);
    
    //==============================================================================
    /** A class for receiving callbacks from a AudioPicker.
     
        To be told when an audio picker changes, you can register a 
        AudioPicker::Listener object using AudioPicker::addListener().
     
        @see AudioPicker::addListener, AudioPicker::removeListener
     */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() {}
        
        //==============================================================================
        /** Called when the audio picker has finished loading the selected track.
         
            This callback recieves an array of MPMediaItems. They are presented as
            void*'s to avoid Obj-C clashes.
         
            @see IOSAudioConverter, mpMediaItemToAvassetUrl, AudioPicker::audioPickerCancelled
         */
        virtual void audioPickerFinished (const Array<void*> mpMediaItems) = 0;
        
        /** Called when the audio picker has been canceled by the user.
         
            @see AudioPicker::audioPickerFinished
         */
        virtual void audioPickerCancelled()  {}
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
    /**	@internal */
    void sendAudioPickerFinishedMessage (void* picker, void* info);
    
    /**	@internal */
    void sendAudioPickerCancelledMessage (void* picker);
    
private:
    //==============================================================================
    ListenerList<Listener> listeners;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPicker);
};

#endif
#endif   // __DROWAUDIO_AUDIOPICKER__