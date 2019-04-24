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
using drow::AudioPicker;

#include <MediaPlayer/MediaPlayer.h>

//==============================================================================
@interface JuceUIAudioPicker : MPMediaPickerController <MPMediaPickerControllerDelegate,
                                                        UIPopoverControllerDelegate>
{
@private
    AudioPicker* owner;
    UIPopoverController* popover;
}

@property (nonatomic, assign) UIPopoverController* popover;

@end

@implementation JuceUIAudioPicker

@synthesize popover;

- (id) initWithOwner: (AudioPicker*) owner_
{
    if ((self = [super init]) != nil)
    {
        owner = owner_;

        self.delegate = self;
    }
    
    return self;
}

- (void) dealloc
{
    [super dealloc];
}

- (void) mediaPicker: (MPMediaPickerController*) mediaPicker didPickMediaItems: (MPMediaItemCollection*) mediaItemCollection
{
    owner->sendAudioPickerFinishedMessage (mediaPicker, mediaItemCollection);
}

- (void) mediaPickerDidCancel: (MPMediaPickerController*) mediaPicker
{
    self.delegate = nil;
    owner->sendAudioPickerCancelledMessage (mediaPicker);
}

- (void) setPopover: (UIPopoverController*) newPopover
{
    popover = newPopover;
    popover.delegate = self;
}

- (void) popoverControllerDidDismissPopover: (UIPopoverController*) popoverController
{
    [popoverController release];
}

+ (UIViewController*) topLevelViewController
{
    UIResponder* responder = ((UIView*) [[UIApplication sharedApplication].keyWindow.subviews objectAtIndex: 0]).nextResponder;
    
    if ([responder isKindOfClass: [UIViewController class]])
    {
        return (UIViewController*) responder;
    }
    
    return nil;
}
@end

namespace drow {

//==============================================================================
AudioPicker::AudioPicker()
{
}

AudioPicker::~AudioPicker()
{
}

//==============================================================================
void AudioPicker::show (bool allowMultipleSelection, Rectangle<int> areaToPointTo)
{
    UIViewController* controller = [JuceUIAudioPicker topLevelViewController];
    
    if (controller != nil)
    {
        JuceUIAudioPicker* audioPicker = [[JuceUIAudioPicker alloc] initWithOwner: this];
        
        if (audioPicker != nil)
        {
            [audioPicker retain];
            audioPicker.allowsPickingMultipleItems = allowMultipleSelection;
            
            if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
            {
                [controller presentViewController: audioPicker animated: YES completion: nil];
            }
            else if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
            {
                UIPopoverController* popover = [[UIPopoverController alloc] initWithContentViewController: audioPicker];
                popover.popoverContentSize = CGSizeMake (320.0f, 480.0f);
                
                audioPicker.popover = popover;
                
                CGRect fromFrame = CGRectMake (controller.view.center.x - 160.0f, controller.view.center.y, 320.0f, 480.0f);
                
                if (! areaToPointTo.isEmpty())
                {
                    fromFrame = CGRectMake (areaToPointTo.getX(), areaToPointTo.getY(), areaToPointTo.getWidth(), areaToPointTo.getHeight());
                }
                
                [popover presentPopoverFromRect: fromFrame
                                         inView: controller.view 
                       permittedArrowDirections: UIPopoverArrowDirectionAny 
                                       animated: YES];
            }
        }
        else
        {
            UIAlertView* alert = [[UIAlertView alloc] initWithTitle: @"Error"
                                                            message: @"Could not load iPod library" 
                                                           delegate: nil
                                                  cancelButtonTitle: @"Ok" 
                                                  otherButtonTitles: nil];
            [alert show];
        }
    }
}

//==============================================================================
String AudioPicker::mpMediaItemToAvassetUrl (void* mpMediaItem)
{
    MPMediaItem* item = (MPMediaItem*) mpMediaItem;
    NSURL* location = [item valueForProperty: MPMediaItemPropertyAssetURL];
    
    return [location.absoluteString UTF8String];
}

//==============================================================================
void AudioPicker::addListener (Listener* const newListener)
{
    listeners.add (newListener);
}

void AudioPicker::removeListener (Listener* const listener)
{
    listeners.remove (listener);
}

//==============================================================================
void AudioPicker::sendAudioPickerFinishedMessage (void* picker, void* info)
{
    if (picker != nil)
    {
        JuceUIAudioPicker* audioPicker = (JuceUIAudioPicker*) picker;
        [audioPicker dismissViewControllerAnimated: YES completion: nil];
        [audioPicker release];

        MPMediaItemCollection* mediaItemCollection = (MPMediaItemCollection*) info;

        Array<void*> pickedMPMediaItems;
        for (int i = 0; i < mediaItemCollection.count; ++i)
        {
            MPMediaItem* mediaItem = [mediaItemCollection.items objectAtIndex: i];
            pickedMPMediaItems.add (mediaItem);
        }
        
        listeners.call (&Listener::audioPickerFinished, pickedMPMediaItems);
    }
}

void AudioPicker::sendAudioPickerCancelledMessage (void* picker)
{
    if (picker != nil)
    {
        JuceUIAudioPicker* audioPicker = (JuceUIAudioPicker*) picker;
        [audioPicker dismissViewControllerAnimated: YES completion: nil];
        [audioPicker release];

        listeners.call (&Listener::audioPickerCancelled);
    }
}

#endif