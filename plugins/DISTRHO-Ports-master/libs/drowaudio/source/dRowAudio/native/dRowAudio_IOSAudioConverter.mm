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
using drow::IOSAudioConverter;

//==============================================================================
@interface JuceIOSAudioConverter : NSObject
{
@private
    IOSAudioConverter* owner;
}

@property (readonly) double progress;
@property bool cancelConverting;
@property (assign) NSString* exportName;

@end

//==============================================================================
@implementation JuceIOSAudioConverter

@synthesize progress;
@synthesize cancelConverting;
@synthesize exportName;

- (id) initWithOwner: (IOSAudioConverter*) owner_
{
    if ((self = [super init]) != nil)
    {
        owner = owner_;
    }
    
    return self;
}

- (void) dealloc
{
    [super dealloc];
}

//==============================================================================
- (void) convertAudioFile: (NSURL*) assetURL
{
    cancelConverting = false;
    progress = 0.0;
    
	// set up an AVAssetReader to read from the iPod Library
	AVURLAsset *songAsset = [AVURLAsset URLAssetWithURL: assetURL options: nil];
    
	NSError* assetError = nil;
	AVAssetReader* assetReader = [[AVAssetReader assetReaderWithAsset: songAsset
                                                                error: &assetError]
								  retain];
	if (assetError)
    {
		NSLog (@"error: %@", assetError);
		return;
	}
    
	AVAssetReaderOutput* assetReaderOutput = [[AVAssetReaderAudioMixOutput 
                                               assetReaderAudioMixOutputWithAudioTracks: songAsset.tracks
                                               audioSettings: nil]
                                              retain];
	if (! [assetReader canAddOutput: assetReaderOutput])
    {
		NSLog (@"can't add reader output... die!");
		return;
	}
	[assetReader addOutput: assetReaderOutput];
	
	NSArray* dirs = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES);
	NSString* documentsDirectoryPath = [dirs objectAtIndex: 0];
	NSString* exportPath = [[documentsDirectoryPath stringByAppendingPathComponent: exportName] retain];
	if ([[NSFileManager defaultManager] fileExistsAtPath: exportPath])
    {
		[[NSFileManager defaultManager] removeItemAtPath: exportPath error: nil];
	}
	NSURL* exportURL = [NSURL fileURLWithPath: exportPath];
	AVAssetWriter* assetWriter = [[AVAssetWriter assetWriterWithURL: exportURL
                                                           fileType: AVFileTypeCoreAudioFormat
                                                              error: &assetError]
								  retain];
	if (assetError) 
    {
		NSLog (@"error: %@", assetError);
		return;
	}
	AudioChannelLayout channelLayout;
	memset (&channelLayout, 0, sizeof (AudioChannelLayout));
	channelLayout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
    
    AVAssetTrack* avAssetTrack = [songAsset.tracks objectAtIndex: 0];
    CMAudioFormatDescriptionRef formatDescription = (CMAudioFormatDescriptionRef)[avAssetTrack.formatDescriptions objectAtIndex: 0];
    const AudioStreamBasicDescription* audioDesc = CMAudioFormatDescriptionGetStreamBasicDescription (formatDescription);
    
    NSDictionary *outputSettings = [NSDictionary dictionaryWithObjectsAndKeys:
									[NSNumber numberWithInt: kAudioFormatLinearPCM],        AVFormatIDKey, 
									[NSNumber numberWithFloat: 44100.0],                    AVSampleRateKey,
									[NSNumber numberWithInt: audioDesc->mChannelsPerFrame], AVNumberOfChannelsKey,
									[NSData dataWithBytes: &channelLayout length: sizeof (AudioChannelLayout)], AVChannelLayoutKey,
									[NSNumber numberWithInt: 16],                           AVLinearPCMBitDepthKey,
									[NSNumber numberWithBool: NO],                          AVLinearPCMIsNonInterleaved,
									[NSNumber numberWithBool: NO],                          AVLinearPCMIsFloatKey,
									[NSNumber numberWithBool: NO],                          AVLinearPCMIsBigEndianKey,
									nil];
	AVAssetWriterInput *assetWriterInput = [[AVAssetWriterInput assetWriterInputWithMediaType: AVMediaTypeAudio
                                                                               outputSettings: outputSettings]
											retain];
	if ([assetWriter canAddInput: assetWriterInput])
    {
		[assetWriter addInput: assetWriterInput];

        owner->sendConverstionStartedMessage (self);
	} 
    else
    {
		NSLog (@"can't add asset writer input... die!");
		return;
	}
	
	assetWriterInput.expectsMediaDataInRealTime = NO;
    
	[assetWriter startWriting];
	[assetReader startReading];
    
	AVAssetTrack *soundTrack = [songAsset.tracks objectAtIndex:0];
	CMTime startTime = CMTimeMake (0, soundTrack.naturalTimeScale);
	[assetWriter startSessionAtSourceTime: startTime];
	
    NSLog (@"duration: %f", CMTimeGetSeconds (soundTrack.timeRange.duration));
    double finalSizeByteCount = soundTrack.timeRange.duration.value * 2 * sizeof (SInt16);
    
	__block UInt64 convertedByteCount = 0;
	
    //==============================================================================
    // reading
    //==============================================================================
    dispatch_queue_t mediaInputQueue = dispatch_queue_create ("mediaInputQueue", NULL);
	[assetWriterInput requestMediaDataWhenReadyOnQueue: mediaInputQueue 
											usingBlock: ^ 
	 {
		 // NSLog (@"top of block");
		 while (assetWriterInput.readyForMoreMediaData)
         {
             CMSampleBufferRef nextBuffer = [assetReaderOutput copyNextSampleBuffer];
             
             if (nextBuffer && ! cancelConverting)
             {
                 // append buffer
                 [assetWriterInput appendSampleBuffer: nextBuffer];
                 
                 convertedByteCount += CMSampleBufferGetTotalSampleSize (nextBuffer);
                 progress = (double) convertedByteCount / finalSizeByteCount;
                 NSNumber* progressNumber = [NSNumber numberWithDouble: progress];
                 [self performSelectorOnMainThread: @selector (updateProgress:)
                                        withObject: progressNumber
                                     waitUntilDone: NO];
             }
             else
             {
                 // done!
                 [assetWriterInput markAsFinished];
                 [assetWriter finishWriting];
                 [assetReader cancelReading];
                 NSDictionary *outputFileAttributes = [[NSFileManager defaultManager]
                                                       attributesOfItemAtPath: exportPath
                                                       error: nil];
                 NSLog (@"done. file size is %llu", [outputFileAttributes fileSize]);

                 // release a lot of stuff
                 [assetReader release];
                 [assetReaderOutput release];
                 [assetWriter release];
                 [assetWriterInput release];
                 [exportPath release];
                 
                 [self performSelectorOnMainThread: @selector (finishedConverting:)
                                        withObject: exportURL
                                     waitUntilDone: NO];
                 
                 break;
             }
         }
	 }];
    //==============================================================================
}

- (void) cancel
{
    cancelConverting = true;
}

- (void) updateProgress: (NSNumber*) progressFloat
{
    owner->sendConverstionUpdatedMessage (self);
}

- (void) finishedConverting: (NSURL*) exportURL
{
    if (cancelConverting)
    {
        owner->sendConverstionFinishedMessage (self, exportURL);
    }
    else
    {
        owner->sendConverstionFinishedMessage (self, exportURL);
    }
}

@end

namespace drow {

//==============================================================================
IOSAudioConverter::IOSAudioConverter()
    : currentAudioConverter (nullptr)
{
}

IOSAudioConverter::~IOSAudioConverter()
{
}

void IOSAudioConverter::startConversion (const String& avAssetUrl, const String& convertedFileName)
{
    [(JuceIOSAudioConverter*) currentAudioConverter release];
    JuceIOSAudioConverter* audioConverter = [[JuceIOSAudioConverter alloc] initWithOwner: this];
    
    if (audioConverter != nil)
    {
        currentAudioConverter = audioConverter;
        [audioConverter retain];
        
        String fileName (convertedFileName);
        if (fileName.isEmpty())
            fileName = "convertedFile";
        
        fileName << ".caf";
        audioConverter.exportName = [NSString stringWithUTF8String: fileName.toUTF8()];
        
        NSURL* idUrl = [NSURL URLWithString: [NSString stringWithUTF8String: avAssetUrl.toUTF8()]];
        [audioConverter convertAudioFile: idUrl];
    }
}

void IOSAudioConverter::cancel()
{
    JuceIOSAudioConverter* audioConverter = (JuceIOSAudioConverter*) currentAudioConverter;
    if (audioConverter != nil)
        [audioConverter cancel];
}

//==============================================================================
void IOSAudioConverter::addListener (Listener* const newListener)
{
    listeners.add (newListener);
}

void IOSAudioConverter::removeListener (Listener* const listener)
{
    listeners.remove (listener);
}

//==============================================================================
void IOSAudioConverter::sendConverstionStartedMessage (void* juceIOSAudioConverter)
{
    listeners.call (&Listener::conversionStarted);
}

void IOSAudioConverter::sendConverstionUpdatedMessage (void* juceIOSAudioConverter)
{
    JuceIOSAudioConverter* converter = (JuceIOSAudioConverter*) juceIOSAudioConverter;
    progress = converter.progress;
    listeners.call (&Listener::conversionUpdated, converter.progress);
}

void IOSAudioConverter::sendConverstionFinishedMessage (void* juceIOSAudioConverter, void* convertedUrl)
{
    JuceIOSAudioConverter* converter = (JuceIOSAudioConverter*) juceIOSAudioConverter;
    currentAudioConverter = nullptr;
    [converter release];
    
    convertedFile = File (stripFileProtocolForLocal (String ([((NSURL*) convertedUrl).absoluteString UTF8String])));
    listeners.call (&Listener::conversionFinished, convertedFile);
}

#endif