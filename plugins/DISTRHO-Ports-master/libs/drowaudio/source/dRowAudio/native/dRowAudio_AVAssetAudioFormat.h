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

#ifndef __DROWAUDIO_AVASSETAUDIOFORMAT_H__
#define __DROWAUDIO_AVASSETAUDIOFORMAT_H__

#if JUCE_IOS || DOXYGEN

//==============================================================================
/**
    OSX and iOS only - This uses the AVFoundation framework to read any audio
    format that the system has a codec for.

    This should be able to understand formats such as mp3, m4a, etc.
 
    Because the underlying AVFoundation framework buffers the reading of the
    audio on a background thread already, it shouldn't be necessarry to use your
    own BufferingAudioSource (unless of course you need to). The other
    side-affect this has is that there can be a slight delay when changing the
    track's position. If you need instant acces try looking at the
    IOSAudioConverter class.

    The easiest way to use this is in conjunction with an AudioPicker.
    @code
        void MyClass::audioPickerFinished (const Array<void*> mpMediaItems)
        {
            const String url (AudioPicker::mpMediaItemToAvassetUrl (mpMediaItems[0]));
            AVAssetAudioFormat* reader = aVAssetAudioFormat
              .createReaderFor (AVAssetAudioFormat::avAssetUrlStringToStream (url);
        }
    @endcode

    @see AudioFormat, AudioPicker, IOSAudioConverter
 */
class AVAssetAudioFormat    : public AudioFormat
{
public:
    //==============================================================================
    /** Creates a format object. */
    AVAssetAudioFormat();

    /** Destructor. */
    ~AVAssetAudioFormat();

    //==============================================================================
    /** Converts an AVAssetURL String to a MemoryInputStream ready to pass to
        createReaderFor().
        
        Because we need a common interface to create the AudioFormatReader this
        helper method converts an AVAssetURL encoded as a String (such as that
        returned by an AudioPicker::mpMediaItemToAvassetUrl) to an InputStream.
        
        The caller is responsible for deleting the stream but this is usually taken
        care of by the createReaderFor method.
     
        @see AudioPicker
     */
    static MemoryInputStream* avAssetUrlStringToStream (const String& avAssetUrlString);

    //==============================================================================
    Array<int> getPossibleSampleRates();
    Array<int> getPossibleBitDepths();
    bool canDoStereo();
    bool canDoMono();

    //==============================================================================
    AudioFormatReader* createReaderFor (String assetNSURLAsString);

    AudioFormatReader* createReaderFor (InputStream* sourceStream,
                                        bool deleteStreamIfOpeningFails);

    AudioFormatWriter* createWriterFor (OutputStream* streamToWriteTo,
                                        double sampleRateToUse,
                                        unsigned int numberOfChannels,
                                        int bitsPerSample,
                                        const StringPairArray& metadataValues,
                                        int qualityOptionIndex);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVAssetAudioFormat);
};

#endif
#endif   // __DROWAUDIO_AVASSETAUDIOFORMAT_H__
