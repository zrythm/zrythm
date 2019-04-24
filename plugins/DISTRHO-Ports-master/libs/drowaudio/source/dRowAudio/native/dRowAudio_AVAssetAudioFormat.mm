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

#if JUCE_IOS

} // namespace drow
#include <AudioToolbox/AudioToolbox.h>
#include <AVFoundation/AVFoundation.h>

namespace drow {
    
//==============================================================================
namespace
{
    const char* const AVAssetAudioFormatName = "AVAsset supported file";

    StringArray findFileExtensionsForCoreAudioCodecs()
    {
        StringArray extensionsArray;
        CFMutableArrayRef extensions = CFArrayCreateMutable (0, 0, 0);
        UInt32 sizeOfArray = sizeof (CFMutableArrayRef);

        if (AudioFileGetGlobalInfo (kAudioFileGlobalInfo_AllExtensions, 0, 0, &sizeOfArray, &extensions) == noErr)
        {
            const CFIndex numValues = CFArrayGetCount (extensions);

            for (CFIndex i = 0; i < numValues; ++i)
                extensionsArray.add ("." + String::fromCFString ((CFStringRef) CFArrayGetValueAtIndex (extensions, i)));
        }

        return extensionsArray;
    }
    
    String nsStringToJuce (NSString* s)
    {
        return CharPointer_UTF8 ([s UTF8String]);
    }
    
    NSString* juceStringToNS (const String& s)
    {
        return [NSString stringWithUTF8String: s.toUTF8()];
    }
}

//==============================================================================
class AVAssetAudioReader : public AudioFormatReader
{
public:
    AVAssetAudioReader (NSURL* assetURL)
        : AudioFormatReader (nullptr, TRANS (AVAssetAudioFormatName)),
          ok                    (false),
          lastReadPosition      (0),
          fifoBuffer            (2 * 8196 * 2),
          tempBlockSize         (256),
          tempInterleavedBlock  (tempBlockSize),
          tempDeinterleavedBlock(tempBlockSize)
    {
        @autoreleasepool {
        
        usesFloatingPointData = true;

        songAsset = [AVURLAsset URLAssetWithURL: assetURL options: nil];
        avAssetTrack = [songAsset.tracks objectAtIndex: 0];
        [songAsset retain];
        [avAssetTrack retain];
        
        NSError* status = nil;
        assetReader = [AVAssetReader assetReaderWithAsset: songAsset    // dont need to retain as a new one
                                                    error: &status];    // will be created in updateReadPosition()
        [assetReader retain];
        assetReaderOutput = nil;
        
        if (! status)
        {
            // fill in format information
            CMAudioFormatDescriptionRef formatDescription = (CMAudioFormatDescriptionRef) [avAssetTrack.formatDescriptions objectAtIndex: 0];
            const AudioStreamBasicDescription* audioDesc = CMAudioFormatDescriptionGetStreamBasicDescription (formatDescription);
            
            if (audioDesc != nullptr)
            {
                numChannels = audioDesc->mChannelsPerFrame;
                bitsPerSample = audioDesc->mBitsPerChannel;
                sampleRate = audioDesc->mSampleRate;
                lengthInSamples = avAssetTrack.timeRange.duration.value;

                outputSettings = [[NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithInt: kAudioFormatLinearPCM], AVFormatIDKey, 
//                                            [NSNumber numberWithFloat:44100.0], AVSampleRateKey,
//                                            [NSData dataWithBytes:&channelLayout length:sizeof(AudioChannelLayout)], AVChannelLayoutKey,
//                                            [NSNumber numberWithInt:16], AVLinearPCMBitDepthKey,
                                    [NSNumber numberWithBool: NO], AVLinearPCMIsNonInterleaved,
                                    [NSNumber numberWithBool: YES], AVLinearPCMIsFloatKey,
                                    [NSNumber numberWithInt: 32], AVLinearPCMBitDepthKey,
//                                            [NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
                                    nil]
                                  retain];

                ok = updateReadPosition (0);
            }
        }}
    }

    ~AVAssetAudioReader()
    {
        [songAsset release];
        [avAssetTrack release];
        [outputSettings release];
        
        [assetReader release];
        [assetReaderOutput release];
    }

    //==============================================================================
    bool readSamples (int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
                      int64 startSampleInFile, int numSamples)
    {
        jassert (destSamples != nullptr);
        jassert (numDestChannels == numChannels);
        
        @autoreleasepool // not sure if there is a better method than this
        {
            const int numBufferSamplesNeeded = numChannels * numSamples;

            // check if position has changed
            if (lastReadPosition != startSampleInFile)
            {
                updateReadPosition (startSampleInFile);
                lastReadPosition = startSampleInFile;
            }

            // get next block if we need to
            if (fifoBuffer.getNumAvailable() < numBufferSamplesNeeded
                && assetReader.status == AVAssetReaderStatusReading)
            {
                CMSampleBufferRef sampleRef = [assetReaderOutput copyNextSampleBuffer];

                if (sampleRef != NULL)
                {
                    CMBlockBufferRef bufferRef = CMSampleBufferGetDataBuffer (sampleRef);
                    size_t lengthAtOffset;
                    size_t totalLength;
                    char* dataPointer;
                    CMBlockBufferGetDataPointer (bufferRef, 
                                                 0, 
                                                 &lengthAtOffset, 
                                                 &totalLength, 
                                                 &dataPointer);
                    if (bufferRef != NULL)
                    {
                        const int samplesExpected = (int) CMSampleBufferGetNumSamples (sampleRef);
                              
                        const int numSamplesNeeded = fifoBuffer.getNumAvailable() + (samplesExpected * numChannels);
                        if (numSamplesNeeded > fifoBuffer.getSize()) //*** need to keep existing
                            fifoBuffer.setSize (numSamplesNeeded);
                        
                        fifoBuffer.writeSamples ((float*) dataPointer, samplesExpected * numChannels);
                    }

                    CFRelease (sampleRef);
                }
            }

            // deinterleave
            if (tempBlockSize < numBufferSamplesNeeded)
            {
                tempInterleavedBlock.malloc (numBufferSamplesNeeded);
                tempDeinterleavedBlock.malloc (numBufferSamplesNeeded);
                tempBlockSize = numBufferSamplesNeeded;
            }
                    
            fifoBuffer.readSamples (tempInterleavedBlock, numBufferSamplesNeeded);
            
            float* deinterleavedSamples[numChannels];
            for (int i = 0; i < numChannels; i++)
                deinterleavedSamples[i] = &tempDeinterleavedBlock[i * numSamples];

            AudioDataConverters::deinterleaveSamples (tempInterleavedBlock, deinterleavedSamples,
                                                      numSamples, numChannels);
                
            for (int i = 0; i < numChannels; i++)
                memcpy (destSamples[i] + startOffsetInDestBuffer, deinterleavedSamples[i], sizeof (float) * numSamples);
                        
            lastReadPosition += numSamples;
        }
        
        return true;
    }

    bool ok;

private:
    //==============================================================================
    NSAutoreleasePool* pool;
    
    AVURLAsset* songAsset;
    AVAssetTrack* avAssetTrack;
    NSDictionary* outputSettings;
    
    AVAssetReader* assetReader;
    AVAssetReaderTrackOutput* assetReaderOutput;
    CMTime startCMTime;
    CMTimeRange playbackCMTimeRange;
    int64 lastReadPosition;
    
    FifoBuffer<float> fifoBuffer;
    int tempBlockSize;
    HeapBlock<float> tempInterleavedBlock, tempDeinterleavedBlock;

    //==============================================================================
    /*  Annoyingly we can't just re-position the stream so we have to create an entire new one.
     */
    bool updateReadPosition (int64 startSample)
    {
        [assetReader cancelReading];
        [assetReader release];
        [assetReaderOutput release];
        
        NSError* error = nil;
        assetReader = [AVAssetReader assetReaderWithAsset: songAsset
                                                    error: &error];
        [assetReader retain];

        if (error == nil)
        {
            assetReaderOutput = [[AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack: avAssetTrack
                                                                            outputSettings: outputSettings]
                                 retain];
            assetReaderOutput.alwaysCopiesSampleData = NO;
                        
            if ([assetReader canAddOutput: assetReaderOutput])
            {
                [assetReader addOutput: assetReaderOutput];
                
                startCMTime = CMTimeMake (startSample, sampleRate);
                playbackCMTimeRange = CMTimeRangeMake (startCMTime, kCMTimePositiveInfinity);
                assetReader.timeRange = playbackCMTimeRange;                
                
                if ([assetReader startReading])
                {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVAssetAudioReader);
};

//==============================================================================
AVAssetAudioFormat::AVAssetAudioFormat()
    : AudioFormat (TRANS (AVAssetAudioFormatName), findFileExtensionsForCoreAudioCodecs())
{
}

AVAssetAudioFormat::~AVAssetAudioFormat() {}

MemoryInputStream* AVAssetAudioFormat::avAssetUrlStringToStream (const String& avAssetUrlString)
{
    const CharPointer_UTF8 urlUTF8 (avAssetUrlString.toUTF8());
    return new MemoryInputStream (urlUTF8.getAddress(), urlUTF8.sizeInBytes(), false);
}

Array<int> AVAssetAudioFormat::getPossibleSampleRates()    { return Array<int>(); }
Array<int> AVAssetAudioFormat::getPossibleBitDepths()      { return Array<int>(); }

bool AVAssetAudioFormat::canDoStereo()     { return true; }
bool AVAssetAudioFormat::canDoMono()       { return true; }

//==============================================================================
AudioFormatReader* AVAssetAudioFormat::createReaderFor (String assetNSURLAsString)
{
    NSString* assetNSString = [NSString stringWithUTF8String:assetNSURLAsString.toUTF8()];
    NSURL* assetNSURL = [NSURL URLWithString:assetNSString];
    
    ScopedPointer<AVAssetAudioReader> r (new AVAssetAudioReader (assetNSURL));

    [assetNSString release];
    [assetNSURL release];
    
    if (r->ok)
        return r.release();

    return nullptr;
}

AudioFormatReader* AVAssetAudioFormat::createReaderFor (InputStream* sourceStream,
                                                        bool deleteStreamIfOpeningFails)
{
    if (sourceStream != nullptr)
    {
        const String nsUrlString (sourceStream->readString());
        
        if (nsUrlString.startsWith ("ipod-library://"))
        {
            NSURL* sourceUrl = [NSURL URLWithString: juceStringToNS (nsUrlString)];
            
            ScopedPointer<AVAssetAudioReader> r (new AVAssetAudioReader (sourceUrl));
            
            if (r->ok)
                return r.release();
            
            if (! deleteStreamIfOpeningFails)
                r->input = nullptr;
        }
    }
    
    return nullptr;
        
    jassertfalse;
    /*  Can't read from a stream, has to be from an AVURLAsset compatible string.
        see createReaderFor (String assetNSURLAsString).
     */
    return nullptr;
}

AudioFormatWriter* AVAssetAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRateToUse,
                                                        unsigned int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex)
{
    jassertfalse; // not yet implemented!
    return nullptr;
}

#endif
